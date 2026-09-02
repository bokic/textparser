#include <textparser-json.h>
#include "adv_regex.h"

#include <json.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>

#include <os.h>

#define json_object_defer(var) struct json_object * var __attribute__((cleanup(json_object_cleanup))) = nullptr

static void json_object_cleanup(struct json_object **handle)
{
    if (handle && *handle)
    {
        json_object_put(*handle);
        *handle = nullptr;
    }
}

#include "string_pool.h"

static uint32_t get_color_or_flag_value(struct json_object *obj, uint32_t default_val)
{
    if (obj == nullptr) {
        return default_val;
    }
    if (json_object_get_type(obj) == json_type_string) {
        const char *str = json_object_get_string(obj);
        if (str) {
            return (uint32_t)strtoul(str, nullptr, 0);
        }
    }
    return (uint32_t)json_object_get_int64(obj);
}

static int json_get_token_id_by_name(const char *name, const textparser_token *tokens, size_t tokens_cnt)
{
    if (!name || !tokens) return -1;
    for (size_t j = 0; j < tokens_cnt; j++) {
        if (tokens[j].name && strcmp(tokens[j].name, name) == 0) {
            return (int)j;
        }
    }
    return -1;
}

static const char **json_parse_string_array(struct json_object *arr_obj, textparser_string_pool *pool)
{
    if (!arr_obj || !json_object_is_type(arr_obj, json_type_array)) return nullptr;
    int len = json_object_array_length(arr_obj);
    if (len <= 0) return nullptr;
    const char **res = calloc(len + 1, sizeof(char *));
    if (res == nullptr) return nullptr;
    for (int i = 0; i < len; i++) {
        struct json_object *item = json_object_array_get_idx(arr_obj, i);
        if (item && json_object_is_type(item, json_type_string)) {
            const char *val = json_object_get_string(item);
            if (val) {
                res[i] = textparser_string_pool_strdup(pool, val);
            }
        }
    }
    return res;
}

static int *json_parse_token_id_array(struct json_object *arr, const textparser_token *tokens, size_t tokens_cnt)
{
    if (!arr || !json_object_is_type(arr, json_type_array)) return nullptr;

    int len = json_object_array_length(arr);
    int *list = calloc(len + 1, sizeof(int));
    if (!list) return nullptr;

    for (int i = 0; i < len; i++) {
        json_object *item = json_object_array_get_idx(arr, i);
        list[i] = TextParser_END;
        if (item && json_object_is_type(item, json_type_string)) {
            const char *name = json_object_get_string(item);
            if (name && tokens) {
                for (size_t j = 0; j < tokens_cnt; j++) {
                    if (tokens[j].name && strcmp(tokens[j].name, name) == 0) {
                        list[i] = (int)j;
                        break;
                    }
                }
            }
        }
    }
    list[len] = TextParser_END;
    return list;
}

typedef struct {
    textparser_production *items;
    size_t count;
    size_t capacity;
    size_t named_count;
    const textparser_token *tokens;
    size_t token_count;
    textparser_string_pool *pool;
    size_t global_sync_token_count;
} json_grammar_builder;

static void json_grammar_builder_free(json_grammar_builder *builder)
{
    if (builder == nullptr || builder->items == nullptr) return;
    for (size_t i = 0; i < builder->count; i++) {
        free((void *)builder->items[i].children);
        free((void *)builder->items[i].recovery_sync_tokens);
    }
    free(builder->items);
    builder->items = nullptr;
    builder->count = 0;
    builder->capacity = 0;
}

static int json_grammar_reserve(json_grammar_builder *builder, size_t required)
{
    if (required <= builder->capacity) return 0;
    size_t capacity = builder->capacity == 0 ? 16 : builder->capacity;
    while (capacity < required) capacity *= 2;
    textparser_production *items = realloc(builder->items, capacity * sizeof(*items));
    if (items == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
    memset(items + builder->capacity, 0, (capacity - builder->capacity) * sizeof(*items));
    builder->items = items;
    builder->capacity = capacity;
    return 0;
}

static int json_grammar_append_anonymous(json_grammar_builder *builder, int *out_id)
{
    int ret = json_grammar_reserve(builder, builder->count + 1);
    if (ret != 0) return ret;
    size_t index = builder->count++;
    memset(&builder->items[index], 0, sizeof(builder->items[index]));
    builder->items[index].id = (int)index;
    builder->items[index].token_id = -1;
    builder->items[index].referenced_production = -1;
    builder->items[index].recovery_insert_token = -1;
    *out_id = (int)index;
    return 0;
}

static int json_grammar_named_id(const json_grammar_builder *builder, const char *name)
{
    if (name == nullptr) return -1;
    for (size_t i = 0; i < builder->named_count; i++) {
        if (builder->items[i].name != nullptr && strcmp(builder->items[i].name, name) == 0) return (int)i;
    }
    return -1;
}

static int json_parse_grammar_construct(
    json_grammar_builder *builder,
    json_object *construct,
    int production_id);

static int json_parse_grammar_construct_core(
    json_grammar_builder *builder,
    json_object *construct,
    int production_id)
{
    if (construct == nullptr || !json_object_is_type(construct, json_type_object) ||
        production_id < 0 || (size_t)production_id >= builder->count) {
        return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
    }

    const char *keys[] = {
        "token", "ref", "sequence", "choice", "optional", "repeat",
        "lookahead", "not", "when", "withContext", "commit", "pratt", "withGoal",
        "capture", "matchCapture"
    };
    json_object *values[15] = {0};
    size_t present = 0;
    int selected = -1;
    for (int i = 0; i < 15; i++) {
        if (json_object_object_get_ex(construct, keys[i], &values[i])) {
            present++;
            selected = i;
        }
    }
    if (present != 1) return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
    json_object_iter member;
    json_object_object_foreachC(construct, member) {
        bool supported = strcmp(member.key, "astKind") == 0;
        if (supported && !json_object_is_type(member.val, json_type_string)) {
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        }
        for (int i = 0; i < 15 && !supported; i++) supported = strcmp(member.key, keys[i]) == 0;
        if (!supported) return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
    }

    textparser_production *production = &builder->items[production_id];
    production->token_id = -1;
    production->referenced_production = -1;
    if (selected == 0) {
        if (!json_object_is_type(values[0], json_type_string)) return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        int token_id = json_get_token_id_by_name(json_object_get_string(values[0]), builder->tokens, builder->token_count);
        if (token_id < 0) return TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN;
        production->kind = TEXTPARSER_PROD_TOKEN;
        production->token_id = token_id;
        return 0;
    }
    if (selected == 1) {
        if (!json_object_is_type(values[1], json_type_string)) return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        int reference = json_grammar_named_id(builder, json_object_get_string(values[1]));
        if (reference < 0) return TEXTPARSER_JSON_GRAMMAR_UNDEFINED_REFERENCE;
        production->kind = TEXTPARSER_PROD_REF;
        production->referenced_production = reference;
        return 0;
    }

    if (selected == 2 || selected == 3) {
        if (!json_object_is_type(values[selected], json_type_array)) return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        size_t child_count = (size_t)json_object_array_length(values[selected]);
        int *children = child_count ? calloc(child_count, sizeof(*children)) : nullptr;
        if (child_count != 0 && children == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
        production = &builder->items[production_id];
        production->kind = selected == 2 ? TEXTPARSER_PROD_SEQUENCE : TEXTPARSER_PROD_CHOICE;
        production->children = children;
        production->child_count = child_count;
        for (size_t i = 0; i < child_count; i++) {
            int child_id = -1;
            int ret = json_grammar_append_anonymous(builder, &child_id);
            if (ret != 0) return ret;
            children[i] = child_id;
            ret = json_parse_grammar_construct(
                builder, json_object_array_get_idx(values[selected], (int)i), child_id);
            if (ret != 0) return ret;
        }
        return 0;
    }

    if (selected >= 4 && selected <= 7) {
        if (!json_object_is_type(values[selected], json_type_object)) {
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        }
        int child_id = -1;
        int ret = json_grammar_append_anonymous(builder, &child_id);
        if (ret != 0) return ret;
        production = &builder->items[production_id];
        switch (selected) {
        case 4: production->kind = TEXTPARSER_PROD_OPTIONAL; break;
        case 5: production->kind = TEXTPARSER_PROD_REPEAT; break;
        case 6: production->kind = TEXTPARSER_PROD_LOOKAHEAD; break;
        default: production->kind = TEXTPARSER_PROD_NOT; break;
        }
        int *children = malloc(sizeof(*children));
        if (children == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
        children[0] = child_id;
        production->children = children;
        production->child_count = 1;
        return json_parse_grammar_construct(builder, values[selected], child_id);
    }

    if (selected == 8) {
        if (!json_object_is_type(values[selected], json_type_object) ||
            json_object_object_length(values[selected]) != 1) {
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        }
        json_object *native = nullptr;
        if (!json_object_object_get_ex(values[selected], "native", &native) ||
            !json_object_is_type(native, json_type_string) ||
            json_object_get_string_len(native) == 0) {
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        }
        production->kind = TEXTPARSER_PROD_PREDICATE;
        production->predicate_name = textparser_string_pool_strdup(
            builder->pool, json_object_get_string(native));
        return production->predicate_name == nullptr ? TEXTPARSER_JSON_OUT_OF_MEMORY : 0;
    }

    if (selected == 10) {
        if (!json_object_is_type(values[selected], json_type_boolean) ||
            !json_object_get_boolean(values[selected])) {
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        }
        production->kind = TEXTPARSER_PROD_COMMIT;
        return 0;
    }

    if (selected == 11) {
        if (!json_object_is_type(values[selected], json_type_object))
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        json_object *primary = nullptr;
        json_object *minimum = nullptr;
        json_object *postfix = nullptr;
        if (!json_object_object_get_ex(values[selected], "primary", &primary) ||
            json_object_object_length(values[selected]) > 3)
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        if (json_object_object_get_ex(values[selected], "minimumPrecedence", &minimum) &&
            !json_object_is_type(minimum, json_type_int))
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        if (json_object_object_get_ex(values[selected], "postfix", &postfix) &&
            !json_object_is_type(postfix, json_type_object))
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        size_t child_count = postfix != nullptr ? 2 : 1;
        int *children = calloc(child_count, sizeof(*children));
        if (children == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
        production = &builder->items[production_id];
        production->kind = TEXTPARSER_PROD_PRATT;
        production->children = children;
        production->child_count = child_count;
        production->minimum_precedence = minimum ? json_object_get_int(minimum) : 0;
        for (size_t i = 0; i < child_count; i++) {
            int child_id = -1;
            int ret = json_grammar_append_anonymous(builder, &child_id);
            if (ret != 0) return ret;
            children[i] = child_id;
            ret = json_parse_grammar_construct(builder, i == 0 ? primary : postfix, child_id);
            if (ret != 0) return ret;
        }
        return 0;
    }

    if (selected == 12) {
        if (!json_object_is_type(values[selected], json_type_object) ||
            json_object_object_length(values[selected]) != 2)
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        json_object *name = nullptr;
        json_object *inner = nullptr;
        if (!json_object_object_get_ex(values[selected], "name", &name) ||
            !json_object_is_type(name, json_type_string) || json_object_get_string_len(name) == 0 ||
            !json_object_object_get_ex(values[selected], "production", &inner) ||
            !json_object_is_type(inner, json_type_object))
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        int child_id = -1;
        int ret = json_grammar_append_anonymous(builder, &child_id);
        if (ret != 0) return ret;
        int *children = malloc(sizeof(*children));
        if (children == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
        children[0] = child_id;
        production = &builder->items[production_id];
        production->kind = TEXTPARSER_PROD_LEXICAL_GOAL;
        production->children = children;
        production->child_count = 1;
        production->lexical_goal = textparser_string_pool_strdup(
            builder->pool, json_object_get_string(name));
        if (production->lexical_goal == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
        return json_parse_grammar_construct(builder, inner, child_id);
    }

    if (selected == 13 || selected == 14) {
        if (!json_object_is_type(values[selected], json_type_object))
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        size_t expected_fields = selected == 13 ? 3 : 2;
        if ((size_t)json_object_object_length(values[selected]) != expected_fields)
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        json_object *name = nullptr;
        json_object *inner = nullptr;
        json_object *then = nullptr;
        if (!json_object_object_get_ex(values[selected], "name", &name) ||
            !json_object_is_type(name, json_type_string) || json_object_get_string_len(name) == 0 ||
            !json_object_object_get_ex(values[selected], "production", &inner) ||
            !json_object_is_type(inner, json_type_object) ||
            (selected == 13 &&
             (!json_object_object_get_ex(values[selected], "then", &then) ||
              !json_object_is_type(then, json_type_object))))
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        size_t child_count = selected == 13 ? 2 : 1;
        int *children = calloc(child_count, sizeof(*children));
        if (children == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
        production = &builder->items[production_id];
        production->kind = selected == 13 ? TEXTPARSER_PROD_CAPTURE : TEXTPARSER_PROD_MATCH_CAPTURE;
        production->children = children;
        production->child_count = child_count;
        production->capture_name = textparser_string_pool_strdup(
            builder->pool, json_object_get_string(name));
        if (production->capture_name == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
        for (size_t i = 0; i < child_count; i++) {
            int child_id = -1;
            int ret = json_grammar_append_anonymous(builder, &child_id);
            if (ret != 0) return ret;
            children[i] = child_id;
            ret = json_parse_grammar_construct(builder, i == 0 ? inner : then, child_id);
            if (ret != 0) return ret;
        }
        return 0;
    }

    if (!json_object_is_type(values[selected], json_type_object)) {
        return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
    }
    json_object *set = nullptr;
    json_object *inner_value = nullptr;
    const char *inner_key = nullptr;
    if (!json_object_object_get_ex(values[selected], "set", &set) ||
        !json_object_is_type(set, json_type_object) || json_object_object_length(set) == 0) {
        return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
    }
    if (json_object_object_get_ex(values[selected], "ref", &inner_value)) inner_key = "ref";
    if (json_object_object_get_ex(values[selected], "sequence", &inner_value)) {
        if (inner_key != nullptr) return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        inner_key = "sequence";
    }
    if (inner_key == nullptr || json_object_object_length(values[selected]) != 2) {
        return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
    }

    int current_id = production_id;
    json_object_iter setting;
    json_object_object_foreachC(set, setting) {
        if (!json_object_is_type(setting.val, json_type_boolean) &&
            !json_object_is_type(setting.val, json_type_int)) {
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        }
        int child_id = -1;
        int ret = json_grammar_append_anonymous(builder, &child_id);
        if (ret != 0) return ret;
        production = &builder->items[current_id];
        production->kind = TEXTPARSER_PROD_CONTEXT;
        production->context_name = textparser_string_pool_strdup(builder->pool, setting.key);
        if (production->context_name == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
        production->context_value = json_object_get_int64(setting.val);
        int *context_child = malloc(sizeof(*context_child));
        if (context_child == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
        context_child[0] = child_id;
        production->children = context_child;
        production->child_count = 1;
        current_id = child_id;
    }
    json_object *inner = json_object_new_object();
    if (inner == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
    json_object_object_add(inner, inner_key, json_object_get(inner_value));
    int ret = json_parse_grammar_construct(builder, inner, current_id);
    json_object_put(inner);
    return ret;
}

static int json_parse_recovery_tokens(
    json_grammar_builder *builder,
    json_object *array,
    textparser_production *production)
{
    if (!json_object_is_type(array, json_type_array) || json_object_array_length(array) == 0 ||
        production->recovery_sync_tokens != nullptr)
        return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
    size_t count = (size_t)json_object_array_length(array);
    int *tokens = calloc(count, sizeof(*tokens));
    if (tokens == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
    for (size_t i = 0; i < count; i++) {
        json_object *name = json_object_array_get_idx(array, (int)i);
        if (!json_object_is_type(name, json_type_string)) {
            free(tokens);
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        }
        tokens[i] = json_get_token_id_by_name(
            json_object_get_string(name), builder->tokens, builder->token_count);
        if (tokens[i] < 0) {
            free(tokens);
            return TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN;
        }
    }
    production->recovery_sync_tokens = tokens;
    production->recovery_sync_token_count = count;
    return 0;
}

static int json_parse_event_binding(
    json_grammar_builder *builder,
    json_object *binding,
    const char **out_handler,
    const char **out_configuration)
{
    const char *handler = nullptr;
    json_object *configuration = nullptr;
    if (json_object_is_type(binding, json_type_string)) {
        if (json_object_get_string_len(binding) == 0)
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        handler = json_object_get_string(binding);
    } else if (json_object_is_type(binding, json_type_object)) {
        json_object *handler_object = nullptr;
        json_object_iter member;
        json_object_object_foreachC(binding, member) {
            if (strcmp(member.key, "handler") != 0 &&
                strcmp(member.key, "configuration") != 0)
                return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        }
        if (!json_object_object_get_ex(binding, "handler", &handler_object) ||
            !json_object_is_type(handler_object, json_type_string) ||
            json_object_get_string_len(handler_object) == 0)
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        handler = json_object_get_string(handler_object);
        json_object_object_get_ex(binding, "configuration", &configuration);
    } else {
        return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
    }
    *out_handler = textparser_string_pool_strdup(builder->pool, handler);
    if (*out_handler == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
    if (configuration != nullptr) {
        const char *serialized = json_object_to_json_string_ext(
            configuration, JSON_C_TO_STRING_PLAIN);
        *out_configuration = textparser_string_pool_strdup(builder->pool, serialized);
        if (*out_configuration == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
    }
    return 0;
}

static int json_parse_production_events(
    json_grammar_builder *builder,
    json_object *events,
    textparser_production *production)
{
    if (!json_object_is_type(events, json_type_object) ||
        json_object_object_length(events) == 0)
        return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
    json_object_iter event;
    json_object_object_foreachC(events, event) {
        int ret;
        if (strcmp(event.key, "onValidate") == 0) {
            if (production->validate_handler != nullptr)
                return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
            ret = json_parse_event_binding(builder, event.val,
                &production->validate_handler, &production->validate_configuration);
        } else if (strcmp(event.key, "onCommit") == 0) {
            if (production->commit_handler != nullptr)
                return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
            ret = json_parse_event_binding(builder, event.val,
                &production->commit_handler, &production->commit_configuration);
        } else if (strcmp(event.key, "onRecovery") == 0) {
            if (production->recovery_handler != nullptr)
                return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
            ret = json_parse_event_binding(builder, event.val,
                &production->recovery_handler, &production->recovery_configuration);
        } else {
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        }
        if (ret != 0) return ret;
    }
    return 0;
}

static int json_parse_grammar_construct(
    json_grammar_builder *builder,
    json_object *construct,
    int production_id)
{
    if (construct == nullptr || !json_object_is_type(construct, json_type_object))
        return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
    json_object *plain = json_object_new_object();
    if (plain == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
    json_object *expect = nullptr, *recover = nullptr, *recover_until = nullptr;
    json_object *asi = nullptr, *events = nullptr;
    json_object_iter member;
    json_object_object_foreachC(construct, member) {
        if (strcmp(member.key, "expect") == 0) expect = member.val;
        else if (strcmp(member.key, "recover") == 0) recover = member.val;
        else if (strcmp(member.key, "recoverUntil") == 0) recover_until = member.val;
        else if (strcmp(member.key, "allowASI") == 0) asi = member.val;
        else if (strcmp(member.key, "events") == 0) events = member.val;
        else json_object_object_add(plain, member.key, json_object_get(member.val));
    }
    int ret = json_parse_grammar_construct_core(builder, plain, production_id);
    json_object_put(plain);
    if (ret != 0) return ret;
    textparser_production *production = &builder->items[production_id];
    if (events != nullptr) {
        ret = json_parse_production_events(builder, events, production);
        if (ret != 0) return ret;
    }
    if (expect != nullptr) {
        if (!json_object_is_type(expect, json_type_string) || json_object_get_string_len(expect) == 0)
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        production->expected_description = textparser_string_pool_strdup(
            builder->pool, json_object_get_string(expect));
        if (production->expected_description == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
    }
    if (asi != nullptr) {
        if (!json_object_is_type(asi, json_type_boolean))
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        production->allow_automatic_semicolon = json_object_get_boolean(asi);
        if (production->allow_automatic_semicolon && production->kind != TEXTPARSER_PROD_TOKEN)
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
    }
    if (recover_until != nullptr) {
        ret = json_parse_recovery_tokens(builder, recover_until, production);
        if (ret != 0) return ret;
        production->recovery_skip = true;
    }
    if (recover != nullptr) {
        if (!json_object_is_type(recover, json_type_object))
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        json_object *insert = nullptr, *skip = nullptr, *sync = nullptr;
        json_object_iter action;
        json_object_object_foreachC(recover, action) {
            if (strcmp(action.key, "insert") == 0) insert = action.val;
            else if (strcmp(action.key, "skip") == 0) skip = action.val;
            else if (strcmp(action.key, "synchronize") == 0) sync = action.val;
            else return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        }
        if (insert != nullptr) {
            if (!json_object_is_type(insert, json_type_string))
                return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
            int token = json_get_token_id_by_name(
                json_object_get_string(insert), builder->tokens, builder->token_count);
            if (token < 0) return TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN;
            production->recovery_insert_token = token;
            production->recovery_insert_enabled = true;
        }
        if (skip != nullptr) {
            if (!json_object_is_type(skip, json_type_boolean))
                return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
            production->recovery_skip = json_object_get_boolean(skip);
        }
        if (sync != nullptr) {
            ret = json_parse_recovery_tokens(builder, sync, production);
            if (ret != 0) return ret;
        }
        if (!production->recovery_insert_enabled && !production->recovery_skip)
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        if (production->recovery_skip && production->recovery_sync_token_count == 0 &&
            builder->global_sync_token_count == 0)
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
    }
    return 0;
}

static void json_grammar_compute_nullable(const json_grammar_builder *builder, bool *nullable)
{
    for (size_t pass = 0; pass < builder->count; pass++) {
        bool changed = false;
        for (size_t i = 0; i < builder->count; i++) {
            const textparser_production *p = &builder->items[i];
            bool value = false;
            switch (p->kind) {
            case TEXTPARSER_PROD_TOKEN:
                value = false;
                break;
            case TEXTPARSER_PROD_REF:
                value = p->referenced_production >= 0 && nullable[p->referenced_production];
                break;
            case TEXTPARSER_PROD_SEQUENCE:
                value = true;
                for (size_t c = 0; c < p->child_count; c++) value = value && nullable[p->children[c]];
                break;
            case TEXTPARSER_PROD_CHOICE:
                for (size_t c = 0; c < p->child_count; c++) value = value || nullable[p->children[c]];
                break;
            case TEXTPARSER_PROD_OPTIONAL:
            case TEXTPARSER_PROD_REPEAT:
                value = true;
                break;
            case TEXTPARSER_PROD_LOOKAHEAD:
            case TEXTPARSER_PROD_NOT:
            case TEXTPARSER_PROD_PREDICATE:
            case TEXTPARSER_PROD_COMMIT:
                value = true;
                break;
            case TEXTPARSER_PROD_CONTEXT:
            case TEXTPARSER_PROD_LEXICAL_GOAL:
            case TEXTPARSER_PROD_MATCH_CAPTURE:
                value = p->child_count == 1 && nullable[p->children[0]];
                break;
            case TEXTPARSER_PROD_CAPTURE:
                value = p->child_count == 2 && nullable[p->children[0]] && nullable[p->children[1]];
                break;
            case TEXTPARSER_PROD_PRATT:
                value = p->child_count >= 1 && nullable[p->children[0]];
                break;
            }
            if (value && !nullable[i]) {
                nullable[i] = true;
                changed = true;
            }
        }
        if (!changed) break;
    }
}

static bool json_grammar_left_recursive_visit(
    const json_grammar_builder *builder,
    int production_id,
    const bool *nullable,
    uint8_t *colors);

static bool json_grammar_visit_leading_child(
    const json_grammar_builder *builder,
    int child,
    const bool *nullable,
    uint8_t *colors)
{
    return json_grammar_left_recursive_visit(builder, child, nullable, colors);
}

static bool json_grammar_left_recursive_visit(
    const json_grammar_builder *builder,
    int production_id,
    const bool *nullable,
    uint8_t *colors)
{
    if (colors[production_id] == 1) return true;
    if (colors[production_id] == 2) return false;
    colors[production_id] = 1;
    const textparser_production *p = &builder->items[production_id];
    bool cycle = false;
    switch (p->kind) {
    case TEXTPARSER_PROD_REF:
        cycle = json_grammar_visit_leading_child(builder, p->referenced_production, nullable, colors);
        break;
    case TEXTPARSER_PROD_SEQUENCE:
        for (size_t i = 0; i < p->child_count && !cycle; i++) {
            cycle = json_grammar_visit_leading_child(builder, p->children[i], nullable, colors);
            if (!nullable[p->children[i]]) break;
        }
        break;
    case TEXTPARSER_PROD_CHOICE:
        for (size_t i = 0; i < p->child_count && !cycle; i++) {
            cycle = json_grammar_visit_leading_child(builder, p->children[i], nullable, colors);
        }
        break;
    case TEXTPARSER_PROD_OPTIONAL:
    case TEXTPARSER_PROD_REPEAT:
    case TEXTPARSER_PROD_LOOKAHEAD:
    case TEXTPARSER_PROD_NOT:
    case TEXTPARSER_PROD_CONTEXT:
    case TEXTPARSER_PROD_LEXICAL_GOAL:
    case TEXTPARSER_PROD_PRATT:
    case TEXTPARSER_PROD_MATCH_CAPTURE:
        cycle = json_grammar_visit_leading_child(builder, p->children[0], nullable, colors);
        break;
    case TEXTPARSER_PROD_CAPTURE:
        cycle = json_grammar_visit_leading_child(builder, p->children[0], nullable, colors);
        if (!cycle && nullable[p->children[0]])
            cycle = json_grammar_visit_leading_child(builder, p->children[1], nullable, colors);
        break;
    case TEXTPARSER_PROD_TOKEN:
    case TEXTPARSER_PROD_PREDICATE:
    case TEXTPARSER_PROD_COMMIT:
        break;
    }
    colors[production_id] = 2;
    return cycle;
}

static int json_validate_grammar(json_grammar_builder *builder)
{
    bool *nullable = calloc(builder->count, sizeof(*nullable));
    uint8_t *colors = calloc(builder->count, sizeof(*colors));
    if (nullable == nullptr || colors == nullptr) {
        free(nullable);
        free(colors);
        return TEXTPARSER_JSON_OUT_OF_MEMORY;
    }
    json_grammar_compute_nullable(builder, nullable);
    for (size_t i = 0; i < builder->count; i++) {
        const textparser_production *p = &builder->items[i];
        if (p->kind == TEXTPARSER_PROD_REPEAT && nullable[p->children[0]]) {
            free(nullable);
            free(colors);
            return TEXTPARSER_JSON_GRAMMAR_NULLABLE_REPEAT;
        }
        if (p->kind == TEXTPARSER_PROD_PRATT && p->child_count == 2 && nullable[p->children[1]]) {
            free(nullable);
            free(colors);
            return TEXTPARSER_JSON_GRAMMAR_NULLABLE_REPEAT;
        }
    }
    for (size_t i = 0; i < builder->count; i++) {
        memset(colors, 0, builder->count * sizeof(*colors));
        if (json_grammar_left_recursive_visit(builder, (int)i, nullable, colors)) {
            free(nullable);
            free(colors);
            return TEXTPARSER_JSON_GRAMMAR_LEFT_RECURSION;
        }
    }
    free(nullable);
    free(colors);
    return 0;
}

static int json_parse_grammar(
    json_object *root,
    textparser_language_definition *definition,
    size_t token_count,
    textparser_string_pool *pool)
{
    definition->maximum_diagnostics = 100;
    definition->maximum_skipped_tokens = 256;
    definition->maximum_recovery_attempts = 100;
    json_object *recovery_policy = nullptr;
    if (json_object_object_get_ex(root, "recovery", &recovery_policy)) {
        if (!json_object_is_type(recovery_policy, json_type_object))
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        json_object_iter policy;
        json_object_object_foreachC(recovery_policy, policy) {
            if (strcmp(policy.key, "maximumDiagnostics") == 0) {
                if (!json_object_is_type(policy.val, json_type_int) || json_object_get_int64(policy.val) <= 0)
                    return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
                definition->maximum_diagnostics = (size_t)json_object_get_int64(policy.val);
            } else if (strcmp(policy.key, "maximumSkippedTokens") == 0) {
                if (!json_object_is_type(policy.val, json_type_int) || json_object_get_int64(policy.val) <= 0)
                    return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
                definition->maximum_skipped_tokens = (size_t)json_object_get_int64(policy.val);
            } else if (strcmp(policy.key, "maximumRecoveryAttempts") == 0) {
                if (!json_object_is_type(policy.val, json_type_int) || json_object_get_int64(policy.val) <= 0)
                    return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
                definition->maximum_recovery_attempts = (size_t)json_object_get_int64(policy.val);
            } else if (strcmp(policy.key, "synchronizationTokens") == 0) {
                if (!json_object_is_type(policy.val, json_type_array) ||
                    json_object_array_length(policy.val) == 0)
                    return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
                size_t count = (size_t)json_object_array_length(policy.val);
                definition->recovery_sync_tokens = calloc(count, sizeof(int));
                if (definition->recovery_sync_tokens == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
                definition->recovery_sync_token_count = count;
                for (size_t i = 0; i < count; i++) {
                    json_object *name = json_object_array_get_idx(policy.val, (int)i);
                    if (!json_object_is_type(name, json_type_string))
                        return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
                    definition->recovery_sync_tokens[i] = json_get_token_id_by_name(
                        json_object_get_string(name), definition->tokens, token_count);
                    if (definition->recovery_sync_tokens[i] < 0)
                        return TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN;
                }
            } else {
                return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
            }
        }
    }
    json_object *grammar_obj = nullptr;
    if (!json_object_object_get_ex(root, "grammar", &grammar_obj)) return 0;
    if (!json_object_is_type(grammar_obj, json_type_object)) return TEXTPARSER_JSON_GRAMMAR_NOT_OBJECT;
    json_object *start_obj = nullptr;
    json_object *productions_obj = nullptr;
    json_object *grammar_events = nullptr;
    if (!json_object_object_get_ex(grammar_obj, "start", &start_obj) ||
        !json_object_is_type(start_obj, json_type_string)) {
        return TEXTPARSER_JSON_GRAMMAR_START_NOT_FOUND;
    }
    if (!json_object_object_get_ex(grammar_obj, "productions", &productions_obj) ||
        !json_object_is_type(productions_obj, json_type_object) ||
        json_object_object_length(productions_obj) == 0) {
        return TEXTPARSER_JSON_GRAMMAR_PRODUCTIONS_NOT_OBJECT;
    }
    json_object_object_get_ex(grammar_obj, "events", &grammar_events);

    json_grammar_builder builder = {0};
    builder.named_count = (size_t)json_object_object_length(productions_obj);
    builder.tokens = definition->tokens;
    builder.token_count = token_count;
    builder.pool = pool;
    builder.global_sync_token_count = definition->recovery_sync_token_count;
    int ret = json_grammar_reserve(&builder, builder.named_count);
    if (ret != 0) goto fail;
    builder.count = builder.named_count;

    size_t index = 0;
    json_object_iter iter;
    json_object_object_foreachC(productions_obj, iter) {
        builder.items[index].id = (int)index;
        builder.items[index].token_id = -1;
        builder.items[index].referenced_production = -1;
        builder.items[index].recovery_insert_token = -1;
        builder.items[index].name = textparser_string_pool_strdup(pool, iter.key);
        if (builder.items[index].name == nullptr) {
            ret = TEXTPARSER_JSON_OUT_OF_MEMORY;
            goto fail;
        }
        index++;
    }

    int start = json_grammar_named_id(&builder, json_object_get_string(start_obj));
    if (start < 0) {
        ret = TEXTPARSER_JSON_GRAMMAR_UNDEFINED_REFERENCE;
        goto fail;
    }
    index = 0;
    json_object_object_foreachC(productions_obj, iter) {
        ret = json_parse_grammar_construct(&builder, iter.val, (int)index++);
        if (ret != 0) goto fail;
    }
    ret = json_validate_grammar(&builder);
    if (ret != 0) goto fail;

    const char *source_complete_handler = nullptr;
    const char *source_complete_configuration = nullptr;
    if (grammar_events != nullptr) {
        if (!json_object_is_type(grammar_events, json_type_object) ||
            json_object_object_length(grammar_events) != 1) {
            ret = TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
            goto fail;
        }
        json_object *binding = nullptr;
        if (!json_object_object_get_ex(grammar_events, "onSourceComplete", &binding)) {
            ret = TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
            goto fail;
        }
        ret = json_parse_event_binding(&builder, binding,
            &source_complete_handler, &source_complete_configuration);
        if (ret != 0) goto fail;
    }

    textparser_grammar_definition *grammar = calloc(1, sizeof(*grammar));
    if (grammar == nullptr) {
        ret = TEXTPARSER_JSON_OUT_OF_MEMORY;
        goto fail;
    }
    grammar->start_production = start;
    grammar->production_count = builder.count;
    grammar->productions = builder.items;
    grammar->source_complete_handler = source_complete_handler;
    grammar->source_complete_configuration = source_complete_configuration;
    definition->grammar = grammar;
    return 0;

fail:
    json_grammar_builder_free(&builder);
    return ret;
}

static int json_parse_contextual_lexer(
    json_object *root,
    textparser_language_definition *definition,
    size_t token_count,
    textparser_string_pool *pool)
{
    json_object *lexer = nullptr;
    if (!json_object_object_get_ex(root, "lexer", &lexer) ||
        !json_object_is_type(lexer, json_type_object)) return 0;
    json_object *value = nullptr;
    const char *initial = "default";
    if (json_object_object_get_ex(lexer, "initialMode", &value)) {
        if (!json_object_is_type(value, json_type_string)) return TEXTPARSER_JSON_INVALID_TOKEN_TYPE;
        initial = json_object_get_string(value);
    }
    definition->initial_lexer_mode = textparser_string_pool_strdup(pool, initial);
    if (definition->initial_lexer_mode == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
    definition->lexer_rules = calloc(token_count, sizeof(*definition->lexer_rules));
    if (token_count && definition->lexer_rules == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;

    json_object *trivia = nullptr;
    if (json_object_object_get_ex(lexer, "trivia", &trivia) && json_object_is_type(trivia, json_type_object)) {
        json_object_iter item;
        json_object_object_foreachC(trivia, item) {
            int id = json_get_token_id_by_name(item.key, definition->tokens, token_count);
            if (id < 0) return TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN;
            definition->lexer_rules[id].is_trivia = true;
        }
    }
    json_object *tokens = nullptr;
    if (json_object_object_get_ex(lexer, "tokens", &tokens) && json_object_is_type(tokens, json_type_object)) {
        json_object_iter item;
        json_object_object_foreachC(tokens, item) {
            int id = json_get_token_id_by_name(item.key, definition->tokens, token_count);
            if (id < 0) return TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN;
            json_object *field = nullptr;
            if (json_object_object_get_ex(item.val, "priority", &field))
                definition->lexer_rules[id].priority = json_object_get_int(field);
            if (json_object_object_get_ex(item.val, "pushMode", &field)) {
                if (!json_object_is_type(field, json_type_string)) return TEXTPARSER_JSON_INVALID_TOKEN_TYPE;
                definition->lexer_rules[id].push_mode = textparser_string_pool_strdup(pool, json_object_get_string(field));
                if (definition->lexer_rules[id].push_mode == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
            }
            if (json_object_object_get_ex(item.val, "popMode", &field))
                definition->lexer_rules[id].pop_mode = json_object_get_boolean(field);
        }
    }

    json_object *modes = nullptr;
    if (json_object_object_get_ex(lexer, "modes", &modes)) {
        if (!json_object_is_type(modes, json_type_object)) return TEXTPARSER_JSON_INVALID_TOKEN_TYPE;
        definition->lexer_mode_count = json_object_object_length(modes);
        definition->lexer_modes = calloc(definition->lexer_mode_count, sizeof(*definition->lexer_modes));
        if (definition->lexer_mode_count && definition->lexer_modes == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
        size_t index = 0;
        json_object_iter mode;
        json_object_object_foreachC(modes, mode) {
            textparser_lexer_mode *out = &definition->lexer_modes[index++];
            out->name = textparser_string_pool_strdup(pool, mode.key);
            if (out->name == nullptr || !json_object_is_type(mode.val, json_type_object)) return TEXTPARSER_JSON_INVALID_TOKEN_TYPE;
            const char *fields[] = {"tokens", "trivia"};
            int **outputs[] = {&out->tokens, &out->trivia};
            for (int f = 0; f < 2; f++) {
                json_object *array = nullptr;
                if (!json_object_object_get_ex(mode.val, fields[f], &array)) continue;
                if (!json_object_is_type(array, json_type_array)) return TEXTPARSER_JSON_INVALID_TOKEN_TYPE;
                size_t count = json_object_array_length(array);
                int *ids = malloc((count + 1) * sizeof(*ids));
                if (ids == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
                for (size_t i = 0; i < count; i++) {
                    json_object *name = json_object_array_get_idx(array, i);
                    if (!json_object_is_type(name, json_type_string) ||
                        (ids[i] = json_get_token_id_by_name(json_object_get_string(name), definition->tokens, token_count)) < 0) {
                        free(ids); return TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN;
                    }
                }
                ids[count] = -1;
                *outputs[f] = ids;
            }
        }
    }
    if (definition->lexer_mode_count != 0) {
        bool initial_found = false;
        for (size_t i = 0; i < definition->lexer_mode_count; i++) {
            if (strcmp(definition->lexer_modes[i].name, definition->initial_lexer_mode) == 0)
                initial_found = true;
        }
        if (!initial_found) return TEXTPARSER_JSON_INVALID_TOKEN_TYPE;
        for (size_t token = 0; token < token_count; token++) {
            const char *push = definition->lexer_rules[token].push_mode;
            if (push == nullptr) continue;
            bool found_mode = false;
            for (size_t i = 0; i < definition->lexer_mode_count; i++)
                if (strcmp(definition->lexer_modes[i].name, push) == 0) found_mode = true;
            if (!found_mode) return TEXTPARSER_JSON_INVALID_TOKEN_TYPE;
        }
    }

    json_object *goals = nullptr;
    if (json_object_object_get_ex(lexer, "goals", &goals)) {
        if (!json_object_is_type(goals, json_type_object)) return TEXTPARSER_JSON_INVALID_TOKEN_TYPE;
        definition->lexer_goal_count = json_object_object_length(goals);
        definition->lexer_goals = calloc(definition->lexer_goal_count, sizeof(*definition->lexer_goals));
        if (definition->lexer_goal_count && definition->lexer_goals == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
        size_t index = 0;
        json_object_iter goal;
        json_object_object_foreachC(goals, goal) {
            textparser_lexer_goal *out = &definition->lexer_goals[index++];
            out->name = textparser_string_pool_strdup(pool, goal.key);
            if (out->name == nullptr || !json_object_is_type(goal.val, json_type_object)) return TEXTPARSER_JSON_INVALID_TOKEN_TYPE;
            out->mapping_count = json_object_object_length(goal.val);
            out->mappings = calloc(out->mapping_count, sizeof(*out->mappings));
            if (out->mapping_count && out->mappings == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
            size_t mapping = 0;
            json_object_iter pair;
            json_object_object_foreachC(goal.val, pair) {
                if (!json_object_is_type(pair.val, json_type_string)) return TEXTPARSER_JSON_INVALID_TOKEN_TYPE;
                int source = json_get_token_id_by_name(pair.key, definition->tokens, token_count);
                int target = json_get_token_id_by_name(json_object_get_string(pair.val), definition->tokens, token_count);
                if (source < 0 || target < 0) return TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN;
                out->mappings[mapping++] = (textparser_lexer_goal_mapping){source, target};
            }
        }
    }
    return 0;
}

static int json_operator_role(const char *name, textparser_operator_role *out)
{
    if (strcmp(name, "prefix") == 0) *out = TEXTPARSER_OP_PREFIX;
    else if (strcmp(name, "infix") == 0) *out = TEXTPARSER_OP_INFIX;
    else if (strcmp(name, "postfix") == 0) *out = TEXTPARSER_OP_POSTFIX;
    else if (strcmp(name, "ternary") == 0) *out = TEXTPARSER_OP_TERNARY;
    else return -1;
    return 0;
}

static int json_parse_pratt_operators(
    json_object *root,
    textparser_language_definition *definition,
    size_t token_count)
{
    json_object *operators = nullptr;
    if (!json_object_object_get_ex(root, "operators", &operators)) return 0;
    if (!json_object_is_type(operators, json_type_array)) return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
    size_t count = 0;
    for (size_t i = 0; i < json_object_array_length(operators); i++) {
        json_object *rule = json_object_array_get_idx(operators, i);
        json_object *roles = nullptr;
        if (!json_object_is_type(rule, json_type_object)) return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        if (json_object_object_get_ex(rule, "roles", &roles)) {
            if (!json_object_is_type(roles, json_type_array) || json_object_array_length(roles) == 0)
                return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
            count += json_object_array_length(roles);
        } else count++;
    }
    definition->operator_definitions = calloc(count, sizeof(*definition->operator_definitions));
    if (count && definition->operator_definitions == nullptr) return TEXTPARSER_JSON_OUT_OF_MEMORY;
    definition->operator_definition_count = count;
    size_t output = 0;
    for (size_t i = 0; i < json_object_array_length(operators); i++) {
        json_object *rule = json_object_array_get_idx(operators, i);
        json_object *token = nullptr;
        json_object *role = nullptr;
        json_object *roles = nullptr;
        json_object *precedence = nullptr;
        json_object *associativity = nullptr;
        json_object *middle = nullptr;
        json_object *left_validator = nullptr;
        json_object *operand_validator = nullptr;
        if (!json_object_object_get_ex(rule, "token", &token) ||
            !json_object_is_type(token, json_type_string)) return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        int token_id = json_get_token_id_by_name(json_object_get_string(token), definition->tokens, token_count);
        if (token_id < 0) return TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN;
        json_object_object_get_ex(rule, "role", &role);
        json_object_object_get_ex(rule, "roles", &roles);
        json_object_object_get_ex(rule, "precedence", &precedence);
        json_object_object_get_ex(rule, "associativity", &associativity);
        json_object_object_get_ex(rule, "middleTerminator", &middle);
        json_object_object_get_ex(rule, "leftValidator", &left_validator);
        json_object_object_get_ex(rule, "operandValidator", &operand_validator);
        if (left_validator != nullptr && !json_object_is_type(left_validator, json_type_string))
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        if (operand_validator != nullptr && !json_object_is_type(operand_validator, json_type_string))
            return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
        size_t role_count = roles ? json_object_array_length(roles) : 1;
        for (size_t r = 0; r < role_count; r++) {
            json_object *role_value = roles ? json_object_array_get_idx(roles, r) : role;
            textparser_operator_def *out = &definition->operator_definitions[output++];
            if (role_value == nullptr || !json_object_is_type(role_value, json_type_string) ||
                json_operator_role(json_object_get_string(role_value), &out->role) != 0)
                return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
            out->token_id = token_id;
            const char *specific = out->role == TEXTPARSER_OP_PREFIX ? "prefixPrecedence" :
                out->role == TEXTPARSER_OP_POSTFIX ? "postfixPrecedence" : "infixPrecedence";
            json_object *specific_value = nullptr;
            if (!json_object_object_get_ex(rule, specific, &specific_value)) specific_value = precedence;
            if (specific_value == nullptr || !json_object_is_type(specific_value, json_type_int))
                return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
            out->precedence = json_object_get_int(specific_value);
            out->associativity = associativity && strcmp(json_object_get_string(associativity), "right") == 0
                ? TEXTPARSER_ASSOC_RIGHT : TEXTPARSER_ASSOC_LEFT;
            out->secondary_token_id = -1;
            out->left_validator = left_validator == nullptr ? nullptr :
                textparser_string_pool_strdup(definition->string_pool,
                    json_object_get_string(left_validator));
            if (left_validator != nullptr && out->left_validator == nullptr)
                return TEXTPARSER_JSON_OUT_OF_MEMORY;
            out->operand_validator = operand_validator == nullptr ? nullptr :
                textparser_string_pool_strdup(definition->string_pool,
                    json_object_get_string(operand_validator));
            if (operand_validator != nullptr && out->operand_validator == nullptr)
                return TEXTPARSER_JSON_OUT_OF_MEMORY;
            if (out->role == TEXTPARSER_OP_TERNARY) {
                if (middle == nullptr || !json_object_is_type(middle, json_type_string))
                    return TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION;
                out->secondary_token_id = json_get_token_id_by_name(
                    json_object_get_string(middle), definition->tokens, token_count);
                if (out->secondary_token_id < 0) return TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN;
            }
        }
    }
    return 0;
}

static int textparser_json_load_language_definition_internal(struct json_object *root_obj, textparser_language_definition **definition)
{
    size_t array_length = 0;
    json_object *tokens = nullptr;
    json_object *value = nullptr;
    json_bool found = false;
    int ret_code = 0;

    if (root_obj == nullptr) {
        return TEXTPARSER_JSON_ROOT_OBJ_IS_NULL;
    }

    if (definition == nullptr) {
        return TEXTPARSER_JSON_DEFINITION_IS_NULL;
    }

    json_object_defer(root);
    root = root_obj;
    json_object_defer(normalized_tokens);
    json_object_defer(generated_start_tokens);
    bool lexer_token_shape = false;

    *definition = malloc(sizeof(textparser_language_definition));
    if (*definition == nullptr) {
        return TEXTPARSER_JSON_OUT_OF_MEMORY;
    }

    memset(*definition, 0, sizeof(textparser_language_definition));

    textparser_string_pool *pool = textparser_string_pool_create();
    if (pool == nullptr) {
        free(*definition);
        *definition = nullptr;
        return TEXTPARSER_JSON_OUT_OF_MEMORY;
    }
    (*definition)->string_pool = pool;

    found = json_object_object_get_ex(root_obj, "name", &value);
    if (!found){
        (*definition)->error_string = "Mandatory field `name` not set!";
        ret_code = TEXTPARSER_JSON_NAME_NOT_FOUND;
        goto err;
    }

    const char *name_str = json_object_get_string(value);
    if (name_str == nullptr) {
        (*definition)->error_string = "Mandatory field `name` is null!";
        ret_code = TEXTPARSER_JSON_NAME_NOT_FOUND;
        goto err;
    }
    (*definition)->name = textparser_string_pool_strdup(pool, name_str);
    if ((*definition)->name == nullptr) {
        (*definition)->error_string = "strdup for `name` FAILED!";
        ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
        goto err;
    }

    found = json_object_object_get_ex(root_obj, "version", &value);
    if (found)
        (*definition)->version = json_object_get_double(value);
    else
        (*definition)->version = 0.;

    found = json_object_object_get_ex(root_obj, "emptySegmentLanguage", &value);
    if (found) {
        const char *empty_lang = json_object_get_string(value);
        if (empty_lang) {
            (*definition)->empty_segment_language = textparser_string_pool_strdup(pool, empty_lang);
            if ((*definition)->empty_segment_language == nullptr) {
                (*definition)->error_string = "strdup for `emptySegmentLanguage` FAILED!";
                ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                goto err;
            }
        } else {
            (*definition)->empty_segment_language = nullptr;
        }
    }

    found = json_object_object_get_ex(root_obj, "caseSensitivity", &value);
    if (!found) {
        (*definition)->error_string = "Mandatory field `caseSensitivity` not set!";
        ret_code = TEXTPARSER_JSON_CASE_SENSITIVITY_NOT_FOUND;
        goto err;
    }
    (*definition)->case_sensitivity = json_object_get_boolean(value);

    found = json_object_object_get_ex(root_obj, "defaultFileExtensions", &value);
    if (!found) {
        (*definition)->error_string = "Mandatory field `defaultFileExtensions` not set!";
        ret_code = TEXTPARSER_JSON_FILE_EXTENSIONS_NOT_FOUND;
        goto err;
    }
    array_length = json_object_array_length(value);
    if (array_length > 0) {
        (*definition)->default_file_extensions = malloc((array_length + 1) * sizeof(char *));
        if ((*definition)->default_file_extensions == nullptr) {
            (*definition)->error_string = "malloc for default_file_extensions FAILED!";
            ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
            goto err;
        }

        memset((*definition)->default_file_extensions, 0, (array_length + 1) * sizeof(char *));

        for(size_t i = 0; i < array_length; i++) {
            json_object *array_item = json_object_array_get_idx(value, i);
            const char *ext_str = json_object_get_string(array_item);
            if (ext_str) {
                (*definition)->default_file_extensions[i] = textparser_string_pool_strdup(pool, ext_str);
                if ((*definition)->default_file_extensions[i] == nullptr) {
                    (*definition)->error_string = "strdup for default_file_extensions item FAILED!";
                    ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                    goto err;
                }
            } else {
                (*definition)->default_file_extensions[i] = nullptr;
            }
        }

        (*definition)->default_file_extensions[array_length] = nullptr;
    }

    found = json_object_object_get_ex(root_obj, "defaultTextEncoding", &value);
    if (found) {
        const char *encoding = json_object_get_string(value);
        if (encoding == NULL) {
            (*definition)->error_string = "Invalid `defaultTextEncoding` value: expected a string.";
            ret_code = TEXTPARSER_JSON_ENCODING_NOT_FOUND;
            goto err;
        }
        if(strcmp(encoding, "latin1") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_ENCODING_LATIN1;
        else if(strcmp(encoding, "utf8") == 0 || strcmp(encoding, "utf-8") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_ENCODING_UTF_8;
        else if(strcmp(encoding, "unicode") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_ENCODING_UNICODE;
        else if(strcmp(encoding, "utf16") == 0 || strcmp(encoding, "utf-16") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_ENCODING_UTF_16;
        else if(strcmp(encoding, "utf32") == 0 || strcmp(encoding, "utf-32") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_ENCODING_UTF_32;
        else {
            (*definition)->error_string = "Invalid `defaultTextEncoding` encoding! Should be one of the following: latin1, utf8, unicode, utf16, utf32.";
            ret_code = TEXTPARSER_JSON_ENCODING_NOT_FOUND;
            goto err;
        }
    }
    else
    {
        (*definition)->default_text_encoding = TEXTPARSER_ENCODING_LATIN1;
    }

    /* Parse supportedBom / SupportedBom */
    json_object *bom_val = nullptr;
    if (!json_object_object_get_ex(root_obj, "supportedBom", &bom_val)) {
        json_object_object_get_ex(root_obj, "SupportedBom", &bom_val);
    }
    if (bom_val != nullptr) {
        int bom_mask = 0;
        if (json_object_is_type(bom_val, json_type_string)) {
            const char *bom_str = json_object_get_string(bom_val);
            if (bom_str != nullptr) {
                char *str_copy = strdup(bom_str);
                if (str_copy != nullptr) {
                    char *saveptr = nullptr;
                    char *token = strtok_r(str_copy, ",", &saveptr);
                    while (token != nullptr) {
                        while (*token == ' ' || *token == '\t') token++;
                        char *end = token + strlen(token) - 1;
                        while (end > token && (*end == ' ' || *end == '\t')) { *end = '\0'; end--; }

                        if (strcasecmp(token, "utf-8") == 0) bom_mask |= TEXTPARSER_BOM_UTF_8;
                        else if (strcasecmp(token, "utf-16-be") == 0) bom_mask |= TEXTPARSER_BOM_UTF_16_BE;
                        else if (strcasecmp(token, "utf-16-le") == 0) bom_mask |= TEXTPARSER_BOM_UTF_16_LE;
                        else if (strcasecmp(token, "utf-32-be") == 0) bom_mask |= TEXTPARSER_BOM_UTF_32_BE;
                        else if (strcasecmp(token, "utf-32-le") == 0) bom_mask |= TEXTPARSER_BOM_UTF_32_LE;

                        token = strtok_r(nullptr, ",", &saveptr);
                    }
                    free(str_copy);
                }
            }
        } else if (json_object_is_type(bom_val, json_type_array)) {
            int bom_cnt = json_object_array_length(bom_val);
            for (int b = 0; b < bom_cnt; b++) {
                json_object *b_item = json_object_array_get_idx(bom_val, b);
                if (b_item && json_object_is_type(b_item, json_type_string)) {
                    const char *token = json_object_get_string(b_item);
                    if (strcasecmp(token, "utf-8") == 0) bom_mask |= TEXTPARSER_BOM_UTF_8;
                    else if (strcasecmp(token, "utf-16-be") == 0) bom_mask |= TEXTPARSER_BOM_UTF_16_BE;
                    else if (strcasecmp(token, "utf-16-le") == 0) bom_mask |= TEXTPARSER_BOM_UTF_16_LE;
                    else if (strcasecmp(token, "utf-32-be") == 0) bom_mask |= TEXTPARSER_BOM_UTF_32_BE;
                    else if (strcasecmp(token, "utf-32-le") == 0) bom_mask |= TEXTPARSER_BOM_UTF_32_LE;
                }
            }
        }
        (*definition)->supported_bom = bom_mask;
    } else {
        (*definition)->supported_bom = 0;
    }

    found = json_object_object_get_ex(root_obj, "tokens", &tokens);
    if (!found) {
        json_object *lexer_obj = nullptr;
        json_object *lexer_tokens = nullptr;
        if (!json_object_object_get_ex(root_obj, "lexer", &lexer_obj) ||
            !json_object_is_type(lexer_obj, json_type_object) ||
            !json_object_object_get_ex(lexer_obj, "tokens", &lexer_tokens) ||
            !json_object_is_type(lexer_tokens, json_type_object)) {
            (*definition)->error_string = "Mandatory field `tokens` is missing!";
            ret_code = TEXTPARSER_JSON_TOKENS_NOT_FOUND;
            goto err;
        }
        normalized_tokens = json_object_new_object();
        if (normalized_tokens == nullptr) {
            ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
            goto err;
        }
        json_object_iter lexer_iter;
        json_object_object_foreachC(lexer_tokens, lexer_iter) {
            json_object_object_add(normalized_tokens, lexer_iter.key, json_object_get(lexer_iter.val));
        }
        json_object *trivia_tokens = nullptr;
        if (json_object_object_get_ex(lexer_obj, "trivia", &trivia_tokens) &&
            json_object_is_type(trivia_tokens, json_type_object)) {
            json_object_object_foreachC(trivia_tokens, lexer_iter) {
                json_object_object_add(normalized_tokens, lexer_iter.key, json_object_get(lexer_iter.val));
            }
        }
        tokens = normalized_tokens;
        lexer_token_shape = true;
    }

    found = json_object_object_get_ex(root_obj, "startTokens", &value);
    if (!found && lexer_token_shape) {
        generated_start_tokens = json_object_new_array();
        if (generated_start_tokens == nullptr) {
            ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
            goto err;
        }
        json_object_iter token_iter;
        json_object_object_foreachC(tokens, token_iter) {
            json_object_array_add(generated_start_tokens, json_object_new_string(token_iter.key));
        }
        value = generated_start_tokens;
        found = true;
    }
    if (!found) {
        (*definition)->error_string = "Mandatory field `startTokens` is missing!";
        ret_code = TEXTPARSER_JSON_STARTS_WITH_NOT_FOUND;
        goto err;
    }

    if (!json_object_is_type(value, json_type_array)) {
        (*definition)->error_string = "`startTokens` is not array!";
        ret_code = TEXTPARSER_JSON_STARTS_WITH_NOT_ARRAY;
        goto err;
    }

    // Save startTokens json object for later processing
    json_object *start_tokens_arr = value;

    if (!json_object_is_type(tokens, json_type_object)) {
        (*definition)->error_string = "`tokens` is not object!";
        ret_code = TEXTPARSER_JSON_TOKENS_NOT_OBJECT;
        goto err;
    }

    size_t tokens_cnt = (size_t)json_object_object_length(tokens);

    (*definition)->tokens = malloc(sizeof(textparser_token) * (tokens_cnt + 1));
    if ((*definition)->tokens == nullptr) {
        (*definition)->error_string = "malloc for tokens list FAILED!";
        ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
        goto err;
    }

    memset((*definition)->tokens, 0, sizeof(textparser_token) * (tokens_cnt + 1));

    // Pass 1: Load basic token data
    {
        size_t token_idx = 0;
        json_object_iter iter;
        json_object_object_foreachC(tokens, iter) {
            json_object *token_item = iter.val;
            struct json_object *key_value = nullptr;
            const char *str_val = nullptr;

            key_value = nullptr;
            json_object_object_get_ex(token_item, "name", &key_value);
            str_val = json_object_get_string(key_value);
            const char *target_name = str_val ? str_val : iter.key;
            if (target_name) {
                (*definition)->tokens[token_idx].name = textparser_string_pool_strdup(pool, target_name);
                if ((*definition)->tokens[token_idx].name == nullptr) {
                    (*definition)->error_string = "strdup for token name FAILED!";
                    ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                    goto err;
                }
            }

            key_value = nullptr;
            json_object_object_get_ex(token_item, "type", &key_value);
            str_val = json_object_get_string(key_value);
            if (str_val) {
                 if (strcasecmp(str_val, "Group") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_GROUP;
                 else if (strcasecmp(str_val, "GroupAllChildrenInSameOrder") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER;
                 else if (strcasecmp(str_val, "GroupOneChildOnly") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY;
                 else if (strcasecmp(str_val, "SimpleToken") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN;
                 else if (strcasecmp(str_val, "StartStop") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_START_STOP;
                 else if (strcasecmp(str_val, "StartOptStop") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_START_OPT_STOP;
                 else if (strcasecmp(str_val, "Sequence") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_SEQUENCE;
                 else {
                     ret_code = TEXTPARSER_JSON_INVALID_TOKEN_TYPE;
                     goto err;
                 }
            } else if (lexer_token_shape) {
                (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN;
            } else {
                ret_code = TEXTPARSER_JSON_TOKEN_TYPE_NOT_FOUND;
                goto err;
            }

            key_value = nullptr;
            if (!json_object_object_get_ex(token_item, "startRegex", &key_value)) {
                json_object_object_get_ex(token_item, "regex", &key_value);
            }
            str_val = json_object_get_string(key_value);
            if (str_val) {
                (*definition)->tokens[token_idx].start_regex = textparser_string_pool_strdup(pool, str_val);
                if ((*definition)->tokens[token_idx].start_regex == nullptr) {
                    (*definition)->error_string = "strdup for token start_regex FAILED!";
                    ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                    goto err;
                }
            } else {
                (*definition)->tokens[token_idx].start_regex = nullptr;
            }

            key_value = nullptr;
            json_object_object_get_ex(token_item, "endRegex", &key_value);
            str_val = json_object_get_string(key_value);
            if (str_val) {
                (*definition)->tokens[token_idx].end_regex = textparser_string_pool_strdup(pool, str_val);
                if ((*definition)->tokens[token_idx].end_regex == nullptr) {
                    (*definition)->error_string = "strdup for token end_regex FAILED!";
                    ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                    goto err;
                }
            } else {
                (*definition)->tokens[token_idx].end_regex = nullptr;
            }

            key_value = nullptr;
            json_object_object_get_ex(token_item, "otherTextInside", &key_value);
            (*definition)->tokens[token_idx].other_text_inside = key_value ? json_object_get_boolean(key_value) : false;

            key_value = nullptr;
            json_object_object_get_ex(token_item, "deleteIfOnlyOneChild", &key_value);
            (*definition)->tokens[token_idx].delete_if_only_one_child = key_value ? json_object_get_boolean(key_value) : false;

            key_value = nullptr;
            json_object_object_get_ex(token_item, "mustHaveOneChild", &key_value);
            (*definition)->tokens[token_idx].must_have_one_child = key_value ? json_object_get_boolean(key_value) : false;

            key_value = nullptr;
            json_object_object_get_ex(token_item, "multiLine", &key_value);
            (*definition)->tokens[token_idx].multi_line = key_value ? json_object_get_boolean(key_value) : false;

            key_value = nullptr;
            json_object_object_get_ex(token_item, "searchParentEndTokenLast", &key_value);
            (*definition)->tokens[token_idx].search_parent_end_token_last = key_value ? json_object_get_boolean(key_value) : false;

            key_value = nullptr;
            json_object_object_get_ex(token_item, "textColor", &key_value);
            (*definition)->tokens[token_idx].text_color = get_color_or_flag_value(key_value, TEXTPARSER_NOCOLOR);

            key_value = nullptr;
            json_object_object_get_ex(token_item, "textBackground", &key_value);
            (*definition)->tokens[token_idx].text_background = get_color_or_flag_value(key_value, TEXTPARSER_NOCOLOR);

            key_value = nullptr;
            json_object_object_get_ex(token_item, "textFlags", &key_value);
            (*definition)->tokens[token_idx].text_flags = get_color_or_flag_value(key_value, 0);

            key_value = nullptr;
            json_object_object_get_ex(token_item, "delimiterTextColor", &key_value);
            (*definition)->tokens[token_idx].delimiter_text_color = get_color_or_flag_value(key_value, TEXTPARSER_NOCOLOR);

            key_value = nullptr;
            json_object_object_get_ex(token_item, "delimiterTextBackground", &key_value);
            (*definition)->tokens[token_idx].delimiter_text_background = get_color_or_flag_value(key_value, TEXTPARSER_NOCOLOR);

            key_value = nullptr;
            json_object_object_get_ex(token_item, "delimiterTextFlags", &key_value);
            (*definition)->tokens[token_idx].delimiter_text_flags = get_color_or_flag_value(key_value, 0);

            token_idx++;
        }
    }

    // Pass 2: Resolve nested tokens names to indices
    {
        size_t token_idx = 0;
        json_object_iter iter;
        json_object_object_foreachC(tokens, iter) {
            json_object *token_item = iter.val;
            struct json_object *nested_tokens_json = nullptr;

            nested_tokens_json = nullptr;
            json_object_object_get_ex(token_item, "nestedTokens", &nested_tokens_json);
            if (nested_tokens_json) {
                if (!json_object_is_type(nested_tokens_json, json_type_array)) {
                    (*definition)->error_string = "`nestedTokens` is not array!";
                    ret_code = TEXTPARSER_JSON_NESTED_TOKENS_NOT_ARRAY;
                    goto err;
                }

                int nested_cnt = json_object_array_length(nested_tokens_json);
                if (nested_cnt == 0) {
                     (*definition)->error_string = "`nestedTokens` array is empty!";
                     ret_code = TEXTPARSER_JSON_NESTED_TOKENS_IS_EMPTY;
                     goto err;
                }

                for (int i = 0; i < nested_cnt; i++) {
                     json_object *item = json_object_array_get_idx(nested_tokens_json, i);
                     if (!item || !json_object_is_type(item, json_type_string)) {
                         (*definition)->error_string = "`nestedTokens` array element is not a string!";
                         ret_code = TEXTPARSER_JSON_NESTED_TOKENS_ELEMENT_NOT_STRING;
                         goto err;
                     }
                }

                (*definition)->tokens[token_idx].nested_tokens = json_parse_token_id_array(nested_tokens_json, (*definition)->tokens, tokens_cnt);
                if (!(*definition)->tokens[token_idx].nested_tokens) {
                    (*definition)->error_string = "malloc for nested_tokens FAILED!";
                    ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                    goto err;
                }
            } else {
                 (*definition)->tokens[token_idx].nested_tokens = nullptr;
            }

            // Resolve contextNestedTokens
            struct json_object *cnt_json = nullptr;
            json_object_object_get_ex(token_item, "contextNestedTokens", &cnt_json);
            if (cnt_json && json_object_is_type(cnt_json, json_type_array)) {
                int rule_cnt = json_object_array_length(cnt_json);
                if (rule_cnt > 0) {
                    (*definition)->tokens[token_idx].context_nested_tokens = malloc(sizeof(textparser_context_nested_tokens) * (rule_cnt + 1));
                    if (!(*definition)->tokens[token_idx].context_nested_tokens) {
                        (*definition)->error_string = "malloc for context_nested_tokens FAILED!";
                        ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                        goto err;
                    }
                    memset((*definition)->tokens[token_idx].context_nested_tokens, 0, sizeof(textparser_context_nested_tokens) * (rule_cnt + 1));

                    for (int r = 0; r < rule_cnt; r++) {
                        json_object *rule_obj = json_object_array_get_idx(cnt_json, r);
                        if (!rule_obj || !json_object_is_type(rule_obj, json_type_object)) continue;

                        json_object *wpi_arr = nullptr;
                        json_object_object_get_ex(rule_obj, "whenParentIn", &wpi_arr);

                        json_object *nt_arr = nullptr;
                        json_object_object_get_ex(rule_obj, "nestedTokens", &nt_arr);

                        if (wpi_arr && json_object_is_type(wpi_arr, json_type_array)) {
                            int *wpi_ids = json_parse_token_id_array(wpi_arr, (*definition)->tokens, tokens_cnt);
                            if (wpi_ids == nullptr) {
                                (*definition)->error_string = "malloc for whenParentIn FAILED!";
                                ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                                goto err;
                            }
                            (*definition)->tokens[token_idx].context_nested_tokens[r].when_parent_in = wpi_ids;
                        }

                        if (nt_arr && json_object_is_type(nt_arr, json_type_array)) {
                            int *nt_ids = json_parse_token_id_array(nt_arr, (*definition)->tokens, tokens_cnt);
                            if (nt_ids == nullptr) {
                                (*definition)->error_string = "malloc for nestedTokens FAILED!";
                                ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                                goto err;
                            }
                            (*definition)->tokens[token_idx].context_nested_tokens[r].nested_tokens = nt_ids;
                        }
                    }
                    (*definition)->tokens[token_idx].context_nested_tokens[rule_cnt].when_parent_in = nullptr;
                    (*definition)->tokens[token_idx].context_nested_tokens[rule_cnt].nested_tokens = nullptr;
                }
            } else {
                (*definition)->tokens[token_idx].context_nested_tokens = nullptr;
            }

            token_idx++;
        }
    }
    (*definition)->tokens[tokens_cnt].name = nullptr; // Sentinel

    // Process startTokens (now that tokens are loaded)
    int starts_with_cnt = (int)json_object_array_length(start_tokens_arr);
    if (starts_with_cnt == 0) {
        (*definition)->error_string = "`startTokens` array is empty!";
        ret_code = TEXTPARSER_JSON_STARTS_WITH_IS_EMPTY;
        goto err;
    }

    for (int i = 0; i < starts_with_cnt; i++) {
        json_object *item = json_object_array_get_idx(start_tokens_arr, i);
        if (!item || !json_object_is_type(item, json_type_string)) {
            (*definition)->error_string = "`startTokens` array element is not a string!";
            ret_code = TEXTPARSER_JSON_STARTS_WITH_ELEMENT_NOT_STRING;
            goto err;
        }
    }

    (*definition)->starts_with = json_parse_token_id_array(start_tokens_arr, (*definition)->tokens, tokens_cnt);
    if ((*definition)->starts_with == nullptr) {
        (*definition)->error_string = "malloc for starts_with FAILED!";
        ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
        goto err;
    }

    found = json_object_object_get_ex(root_obj, "otherTextInside", &value);
    if (found) {
        (*definition)->other_text_inside = json_object_get_boolean(value);
    }

    json_object *sign_merge_obj = nullptr;
    if (json_object_object_get_ex(root_obj, "mergeSignIntoNumber", &sign_merge_obj) && json_object_is_type(sign_merge_obj, json_type_object)) {
        textparser_sign_merge *sign_merge = calloc(1, sizeof(textparser_sign_merge));
        if (sign_merge != nullptr) {
            int *sign_list = nullptr;
            int *number_list = nullptr;
            int *operand_list = nullptr;
            const char *list_names[3] = { "signTokens", "numberTokens", "operandTokens" };
            int **list_ptrs[3] = { &sign_list, &number_list, &operand_list };
            for (int l = 0; l < 3; l++) {
                json_object *list_arr = nullptr;
                if (!json_object_object_get_ex(sign_merge_obj, list_names[l], &list_arr) || !json_object_is_type(list_arr, json_type_array)) {
                    continue;
                }
                *list_ptrs[l] = json_parse_token_id_array(list_arr, (*definition)->tokens, tokens_cnt);
            }
            sign_merge->sign_tokens = sign_list;
            sign_merge->number_tokens = number_list;
            sign_merge->operand_tokens = operand_list;
            (*definition)->sign_merge = sign_merge;
        }
    }

    json_object *regdiv_obj = nullptr;
    if (json_object_object_get_ex(root_obj, "regexVsDivision", &regdiv_obj) && json_object_is_type(regdiv_obj, json_type_object)) {
        textparser_regex_disambiguation *regdiv = calloc(1, sizeof(textparser_regex_disambiguation));
        if (regdiv != nullptr) {
            json_object *arr = nullptr;
            if (json_object_object_get_ex(regdiv_obj, "regexTokens", &arr) && json_object_is_type(arr, json_type_array)) {
                regdiv->regex_tokens = json_parse_token_id_array(arr, (*definition)->tokens, tokens_cnt);
            }
            if (json_object_object_get_ex(regdiv_obj, "divisionTokens", &arr) && json_object_is_type(arr, json_type_array)) {
                regdiv->division_tokens = json_parse_token_id_array(arr, (*definition)->tokens, tokens_cnt);
            }
            if (json_object_object_get_ex(regdiv_obj, "operandTokens", &arr) && json_object_is_type(arr, json_type_array)) {
                regdiv->operand_tokens = json_parse_token_id_array(arr, (*definition)->tokens, tokens_cnt);
            }
            if (json_object_object_get_ex(regdiv_obj, "controlKeywords", &arr) && json_object_is_type(arr, json_type_array)) {
                regdiv->control_keywords = json_parse_string_array(arr, pool);
            }
            (*definition)->regex_disambiguation = regdiv;
        }
    }

    json_object *tpl_obj = nullptr;
    if (json_object_object_get_ex(root_obj, "templateDisambiguation", &tpl_obj) && json_object_is_type(tpl_obj, json_type_object)) {
        textparser_template_disambiguation *tpl = calloc(1, sizeof(textparser_template_disambiguation));
        if (tpl != nullptr) {
            tpl->template_group_token_id = -1;
            json_object *arr = nullptr;
            if (json_object_object_get_ex(tpl_obj, "templateOpenTokens", &arr) && json_object_is_type(arr, json_type_array)) {
                tpl->template_open_tokens = json_parse_token_id_array(arr, (*definition)->tokens, tokens_cnt);
            }
            if (json_object_object_get_ex(tpl_obj, "templateCloseTokens", &arr) && json_object_is_type(arr, json_type_array)) {
                tpl->template_close_tokens = json_parse_token_id_array(arr, (*definition)->tokens, tokens_cnt);
            }
            if (json_object_object_get_ex(tpl_obj, "validInnerTokens", &arr) && json_object_is_type(arr, json_type_array)) {
                tpl->valid_inner_tokens = json_parse_token_id_array(arr, (*definition)->tokens, tokens_cnt);
            }
            if (json_object_object_get_ex(tpl_obj, "invalidInnerOperators", &arr) && json_object_is_type(arr, json_type_array)) {
                tpl->invalid_inner_operators = json_parse_string_array(arr, pool);
            }
            json_object *grp_tok = nullptr;
            if (json_object_object_get_ex(tpl_obj, "templateGroupToken", &grp_tok) && json_object_is_type(grp_tok, json_type_string)) {
                const char *grp_name = json_object_get_string(grp_tok);
                if (grp_name) {
                    tpl->template_group_token_id = json_get_token_id_by_name(grp_name, (*definition)->tokens, tokens_cnt);
                }
            }
            (*definition)->template_disambiguation = tpl;
        }
    }

    json_object *cst_obj = nullptr;
    if (json_object_object_get_ex(root_obj, "castDisambiguation", &cst_obj) && json_object_is_type(cst_obj, json_type_object)) {
        textparser_cast_disambiguation *cst = calloc(1, sizeof(textparser_cast_disambiguation));
        if (cst != nullptr) {
            cst->cast_token_id = -1;
            json_object *arr = nullptr;
            if (json_object_object_get_ex(cst_obj, "typeTokens", &arr) && json_object_is_type(arr, json_type_array)) {
                cst->type_tokens = json_parse_token_id_array(arr, (*definition)->tokens, tokens_cnt);
            }
            if (json_object_object_get_ex(cst_obj, "typeKeywords", &arr) && json_object_is_type(arr, json_type_array)) {
                cst->type_keywords = json_parse_string_array(arr, pool);
            }
            if (json_object_object_get_ex(cst_obj, "typeSuffixes", &arr) && json_object_is_type(arr, json_type_array)) {
                cst->type_suffixes = json_parse_string_array(arr, pool);
            }
            json_object *cast_tok = nullptr;
            if (json_object_object_get_ex(cst_obj, "castToken", &cast_tok) && json_object_is_type(cast_tok, json_type_string)) {
                const char *cast_name = json_object_get_string(cast_tok);
                if (cast_name) {
                    cst->cast_token_id = json_get_token_id_by_name(cast_name, (*definition)->tokens, tokens_cnt);
                }
            }
            (*definition)->cast_disambiguation = cst;
        }
    }

    json_object *decl_obj = nullptr;
    if (json_object_object_get_ex(root_obj, "declarationDisambiguation", &decl_obj) && json_object_is_type(decl_obj, json_type_object)) {
        textparser_declaration_disambiguation *decl = calloc(1, sizeof(textparser_declaration_disambiguation));
        if (decl != nullptr) {
            decl->identifier_token_id = -1;
            decl->type_name_token_id = -1;
            decl->function_token_id = -1;
            decl->parameter_list_token_id = -1;
            json_object *arr = nullptr;
            if (json_object_object_get_ex(decl_obj, "returnTypeTokens", &arr) && json_object_is_type(arr, json_type_array)) {
                decl->return_type_tokens = json_parse_token_id_array(arr, (*definition)->tokens, tokens_cnt);
            }
            if (json_object_object_get_ex(decl_obj, "declaratorTokens", &arr) && json_object_is_type(arr, json_type_array)) {
                decl->declarator_tokens = json_parse_token_id_array(arr, (*definition)->tokens, tokens_cnt);
            }
            struct { const char *key; int *value; } token_fields[] = {
                {"identifierToken", &decl->identifier_token_id},
                {"typeNameToken", &decl->type_name_token_id},
                {"functionToken", &decl->function_token_id},
                {"parameterListToken", &decl->parameter_list_token_id}
            };
            for (size_t i = 0; i < sizeof(token_fields) / sizeof(token_fields[0]); i++) {
                json_object *tok = nullptr;
                if (json_object_object_get_ex(decl_obj, token_fields[i].key, &tok) && json_object_is_type(tok, json_type_string)) {
                    *token_fields[i].value = json_get_token_id_by_name(json_object_get_string(tok), (*definition)->tokens, tokens_cnt);
                }
            }
            (*definition)->declaration_disambiguation = decl;
        }
    }

    json_object *prec_arr = nullptr;
    if (json_object_object_get_ex(root_obj, "operator_precedence", &prec_arr) && json_object_is_type(prec_arr, json_type_array)) {
        int rule_count = json_object_array_length(prec_arr);
        if (rule_count > 0) {
            textparser_operator_precedence *op_prec = calloc(1, sizeof(textparser_operator_precedence));
            textparser_precedence_rule *rules = calloc(rule_count, sizeof(textparser_precedence_rule));
            if (op_prec == nullptr || rules == nullptr) {
                free(op_prec);
                free(rules);
                (*definition)->error_string = "calloc for operator_precedence FAILED!";
                ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                goto err;
            }
            op_prec->count = (size_t)rule_count;
            op_prec->rules = rules;
            for (int r = 0; r < rule_count; r++) {
                json_object *rule_obj = json_object_array_get_idx(prec_arr, r);
                if (!rule_obj || !json_object_is_type(rule_obj, json_type_object)) continue;

                json_object *assoc_val = nullptr;
                if (json_object_object_get_ex(rule_obj, "associativity", &assoc_val) && json_object_is_type(assoc_val, json_type_string)) {
                    const char *assoc_str = json_object_get_string(assoc_val);
                    if (assoc_str && strcasecmp(assoc_str, "right") == 0) {
                        rules[r].associativity = TEXTPARSER_ASSOC_RIGHT;
                    } else {
                        rules[r].associativity = TEXTPARSER_ASSOC_LEFT;
                    }
                }

                json_object *ops_arr = nullptr;
                if (json_object_object_get_ex(rule_obj, "operators", &ops_arr) && json_object_is_type(ops_arr, json_type_array)) {
                    rules[r].operators = json_parse_token_id_array(ops_arr, (*definition)->tokens, tokens_cnt);
                }
            }
            (*definition)->operator_precedence = op_prec;
        }
    }

    json_object *override_arr = nullptr;
    if (json_object_object_get_ex(root_obj, "overrideStartTokens", &override_arr) && json_object_is_type(override_arr, json_type_array)) {
        int rule_count = json_object_array_length(override_arr);
        if (rule_count > 0) {
            textparser_override_start_token_rule *rules = calloc(rule_count + 1, sizeof(textparser_override_start_token_rule));
            if (rules == nullptr) {
                (*definition)->error_string = "calloc for override_start_tokens FAILED!";
                ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                goto err;
            }
            for (int r = 0; r < rule_count; r++) {
                json_object *rule_obj = json_object_array_get_idx(override_arr, r);
                if (!rule_obj || !json_object_is_type(rule_obj, json_type_object)) continue;

                json_object *if_obj = nullptr;
                if (json_object_object_get_ex(rule_obj, "if", &if_obj) && json_object_is_type(if_obj, json_type_object)) {
                    json_object *exts_arr = nullptr;
                    if (json_object_object_get_ex(if_obj, "fileExtensions", &exts_arr) && json_object_is_type(exts_arr, json_type_array)) {
                        int ext_len = json_object_array_length(exts_arr);
                        const char **ext_list = calloc(ext_len + 1, sizeof(char *));
                        if (ext_list == nullptr) {
                            (*definition)->error_string = "calloc for override rule fileExtension FAILED!";
                            ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                            goto err;
                        }
                        for (int e = 0; e < ext_len; e++) {
                            json_object *ext_item = json_object_array_get_idx(exts_arr, e);
                            if (ext_item && json_object_is_type(ext_item, json_type_string)) {
                                const char *ext_val = json_object_get_string(ext_item);
                                if (ext_val) {
                                    ext_list[e] = textparser_string_pool_strdup(pool, ext_val);
                                    if (ext_list[e] == nullptr) {
                                        (*definition)->error_string = "strdup for override rule fileExtension FAILED!";
                                        ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                                        goto err;
                                    }
                                }
                            }
                        }
                        rules[r].file_extensions = ext_list;
                    }

                    json_object *regex_item = nullptr;
                    if (json_object_object_get_ex(if_obj, "regex", &regex_item) && json_object_is_type(regex_item, json_type_string)) {
                        const char *reg_val = json_object_get_string(regex_item);
                        if (reg_val) {
                            rules[r].regex = textparser_string_pool_strdup(pool, reg_val);
                            if (rules[r].regex == nullptr) {
                                (*definition)->error_string = "strdup for override rule regex FAILED!";
                                ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                                goto err;
                            }
                        }
                    }
                }

                json_object *st_arr = nullptr;
                if (json_object_object_get_ex(rule_obj, "startTokens", &st_arr) && json_object_is_type(st_arr, json_type_array)) {
                    rules[r].start_tokens = json_parse_token_id_array(st_arr, (*definition)->tokens, tokens_cnt);
                }
            }
            (*definition)->override_start_tokens = rules;
        }
    }

    ret_code = json_parse_contextual_lexer(root_obj, *definition, tokens_cnt, pool);
    if (ret_code != 0) {
        (*definition)->error_string = "Invalid contextual lexer";
        goto err;
    }

    ret_code = json_parse_pratt_operators(root_obj, *definition, tokens_cnt);
    if (ret_code != 0) {
        (*definition)->error_string = "Invalid Pratt operator table";
        goto err;
    }

    ret_code = json_parse_grammar(root_obj, *definition, tokens_cnt, pool);
    if (ret_code != 0) {
        (*definition)->error_string = "Invalid declarative grammar";
        goto err;
    }

    return 0;

err:
    if (definition && *definition) {
        textparser_free_language_definition(*definition);
        *definition = nullptr;
    }
    return ret_code;
}

int textparser_json_load_language_definition_from_json_file(const char *pathname, textparser_language_definition **definition)
{
    return textparser_json_load_language_definition_internal(json_object_from_file(pathname), definition);
}

int textparser_json_load_language_definition_from_string(const char *text, textparser_language_definition **definition)
{
    return textparser_json_load_language_definition_internal(json_tokener_parse(text), definition);
}

const char *textparser_json_strerror(int error_code)
{
    switch (error_code) {
    case TEXTPARSER_JSON_NO_ERROR:
        return "Success";
    case TEXTPARSER_JSON_ROOT_OBJ_IS_NULL:
        return "JSON root object is null";
    case TEXTPARSER_JSON_DEFINITION_IS_NULL:
        return "Definition pointer is null";
    case TEXTPARSER_JSON_OUT_OF_MEMORY:
        return "Out of memory";
    case TEXTPARSER_JSON_NAME_NOT_FOUND:
        return "Mandatory field 'name' not found or invalid";
    case TEXTPARSER_JSON_VERSION_NOT_FOUND:
        return "Field 'version' not found";
    case TEXTPARSER_JSON_EMPTY_SEGMENT_LANGUAGE_NOT_FOUND:
        return "Field 'emptySegmentLanguage' not found";
    case TEXTPARSER_JSON_CASE_SENSITIVITY_NOT_FOUND:
        return "Field 'caseSensitivity' not found";
    case TEXTPARSER_JSON_FILE_EXTENSIONS_NOT_FOUND:
        return "Field 'defaultFileExtensions' not found";
    case TEXTPARSER_JSON_ENCODING_NOT_FOUND:
        return "Field 'defaultTextEncoding' not found";
    case TEXTPARSER_JSON_STARTS_WITH_NOT_FOUND:
        return "Field 'startsWith' not found";
    case TEXTPARSER_JSON_STARTS_WITH_NOT_ARRAY:
        return "Field 'startsWith' is not an array";
    case TEXTPARSER_JSON_TOKENS_NOT_FOUND:
        return "Field 'tokens' not found";
    case TEXTPARSER_JSON_TOKENS_NOT_OBJECT:
        return "Field 'tokens' is not an object";
    case TEXTPARSER_JSON_STARTS_WITH_IS_EMPTY:
        return "Field 'startsWith' array is empty";
    case TEXTPARSER_JSON_STARTS_WITH_ELEMENT_NOT_STRING:
        return "Element in 'startsWith' is not a string";
    case TEXTPARSER_JSON_NESTED_TOKENS_NOT_ARRAY:
        return "Field 'nestedTokens' is not an array";
    case TEXTPARSER_JSON_NESTED_TOKENS_IS_EMPTY:
        return "Field 'nestedTokens' array is empty";
    case TEXTPARSER_JSON_NESTED_TOKENS_ELEMENT_NOT_STRING:
        return "Element in 'nestedTokens' is not a string";
    case TEXTPARSER_JSON_TOKEN_TYPE_NOT_FOUND:
        return "Field 'type' not found in token definition";
    case TEXTPARSER_JSON_INVALID_TOKEN_TYPE:
        return "Invalid token type";
    case TEXTPARSER_JSON_GRAMMAR_NOT_OBJECT:
        return "Field 'grammar' is not an object";
    case TEXTPARSER_JSON_GRAMMAR_START_NOT_FOUND:
        return "Grammar start production is missing or invalid";
    case TEXTPARSER_JSON_GRAMMAR_PRODUCTIONS_NOT_OBJECT:
        return "Grammar productions are missing, empty, or not an object";
    case TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION:
        return "Grammar production must contain exactly one supported construct";
    case TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN:
        return "Grammar production references an undefined token";
    case TEXTPARSER_JSON_GRAMMAR_UNDEFINED_REFERENCE:
        return "Grammar references an undefined production";
    case TEXTPARSER_JSON_GRAMMAR_LEFT_RECURSION:
        return "Grammar contains a recursive cycle before token consumption";
    case TEXTPARSER_JSON_GRAMMAR_NULLABLE_REPEAT:
        return "Grammar repeat child can match without consuming a token";
    default:
        return "Unknown JSON parser error";
    }
}
