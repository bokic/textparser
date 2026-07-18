#include "css.h"
#include "css_tags.h"

#include <textparser.h>
#include <css_definition.json.h>

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef _WIN32
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

typedef struct {
    int AtRule;
    int PseudoClass;
    int Declaration;
} css_dynamic_token_ids;

static char *dynamic_printf(const char *format, ...) {
    va_list args1, args2;
    va_start(args1, format);
    va_copy(args2, args1);

    int length = vsnprintf(NULL, 0, format, args1);
    va_end(args1);

    if (length < 0) {
        va_end(args2);
        return NULL;
    }

    char *buffer = malloc(length + 1);
    if (buffer == NULL) {
        va_end(args2);
        return NULL;
    }

    vsnprintf(buffer, length + 1, format, args2);
    va_end(args2);

    return buffer;
}

static void textparser_validation_item_add(enum textparser_validation_item_type type, textparser_validation **validation, char *text, size_t position, size_t length) {
    int current_len = *validation ? (*validation)->len : 0;
    textparser_validation *new_val = realloc(*validation, offsetof(textparser_validation, items) + sizeof(textparser_validation_item *) * (current_len + 1));
    if (new_val == NULL) {
        free(text);
        return;
    }
    *validation = new_val;
    if (current_len == 0) {
        (*validation)->len = 0;
    }

    textparser_validation_item *item = malloc(sizeof(textparser_validation_item));
    if (item == NULL) {
        free(text);
        return;
    }
    item->type = type;
    item->position = position;
    item->length = length;
    item->text = text;

    (*validation)->items[(*validation)->len] = item;
    (*validation)->len++;
}

void textparser_validation_clear(textparser_validation *validation) {
    if (validation != NULL) {
        for (int i = 0; i < validation->len; i++) {
            free(validation->items[i]->text);
            free(validation->items[i]);
        }
        free(validation);
    }
}

static int find_token_id_by_name(textparser_t handle, const char *name) {
    const textparser_language_definition *definition = textparser_get_language(handle);
    if (definition == NULL || definition->tokens == NULL) {
        return TextParser_END;
    }
    int token_idx = 0;
    while (definition->tokens[token_idx].name != NULL) {
        if (strcmp(definition->tokens[token_idx].name, name) == 0) {
            return token_idx;
        }
        token_idx++;
    }
    return TextParser_END;
}

static void resolve_css_token_ids(textparser_t handle, css_dynamic_token_ids *ids) {
    ids->AtRule = find_token_id_by_name(handle, "AtRule");
    ids->PseudoClass = find_token_id_by_name(handle, "PseudoClass");
    ids->Declaration = find_token_id_by_name(handle, "Declaration");
}

static int compare_strings(const void *a, const void *b) {
    const char *str_a = *(const char *const *)a;
    const char *str_b = *(const char *const *)b;
    return strcmp(str_a, str_b);
}

static bool is_css_property(const char *name) {
    if (strncmp(name, "--", 2) == 0) return true;
    
    if (strncmp(name, "-webkit-", 8) == 0 ||
        strncmp(name, "-moz-", 5) == 0 ||
        strncmp(name, "-ms-", 4) == 0 ||
        strncmp(name, "-o-", 3) == 0) {
        return true;
    }
    
    return bsearch(&name, css_properties, css_property_count, sizeof(char *), compare_strings) != NULL;
}

static bool is_css_pseudo_class(const char *name) {
    if (strncmp(name, ":-webkit-", 9) == 0 ||
        strncmp(name, ":-moz-", 6) == 0 ||
        strncmp(name, ":-ms-", 5) == 0 ||
        strncmp(name, ":-o-", 4) == 0) {
        return true;
    }
    return bsearch(&name, css_pseudo_classes, css_pseudo_class_count, sizeof(char *), compare_strings) != NULL;
}

static bool is_css_pseudo_element(const char *name) {
    if (strncmp(name, "::-webkit-", 10) == 0 ||
        strncmp(name, "::-moz-", 7) == 0 ||
        strncmp(name, "::-ms-", 6) == 0 ||
        strncmp(name, "::-o-", 5) == 0) {
        return true;
    }
    return bsearch(&name, css_pseudo_elements, css_pseudo_element_count, sizeof(char *), compare_strings) != NULL;
}

static bool is_css_at_rule(const char *name) {
    if (strncmp(name, "@-webkit-", 9) == 0 ||
        strncmp(name, "@-moz-", 6) == 0 ||
        strncmp(name, "@-ms-", 5) == 0 ||
        strncmp(name, "@-o-", 4) == 0) {
        return true;
    }
    return bsearch(&name, css_at_rules, css_at_rule_count, sizeof(char *), compare_strings) != NULL;
}

static void get_property_name(const char *text, textparser_token_item *token, char *buf, size_t max_len) {
    size_t p = token->position;
    size_t end = token->position + token->len;
    size_t i = 0;
    while (p < end && i + 1 < max_len) {
        char c = text[p];
        if (c == ':' || c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            break;
        }
        buf[i++] = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
        p++;
    }
    buf[i] = '\0';
}

static void get_token_name_lower(const char *text, textparser_token_item *token, char *buf, size_t max_len) {
    size_t len = token->len < max_len - 1 ? token->len : max_len - 1;
    for (size_t i = 0; i < len; i++) {
        char c = text[token->position + i];
        buf[i] = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
    }
    buf[len] = '\0';
}

static void textparser_validate_css_token(const css_dynamic_token_ids *ids, textparser_validation **ret, textparser_t handle, textparser_token_item *token) {
    const char *text = textparser_get_text(handle);
    if (token->token_id != TextParser_END && token->token_id == ids->Declaration) {
        char prop_name[128];
        get_property_name(text, token, prop_name, sizeof(prop_name));

        if (!is_css_property(prop_name)) {
            char *str = dynamic_printf("Unknown CSS property: [%s]", prop_name);
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, strlen(prop_name));
        }
    }
    else if (token->token_id != TextParser_END && token->token_id == ids->PseudoClass) {
        char pseudo_name[128];
        get_token_name_lower(text, token, pseudo_name, sizeof(pseudo_name));

        if (strncmp(pseudo_name, "::", 2) == 0) {
            if (!is_css_pseudo_element(pseudo_name)) {
                char *str = dynamic_printf("Unknown CSS pseudo-element: [%s]", pseudo_name);
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, token->len);
            }
        } else {
            if (!is_css_pseudo_class(pseudo_name)) {
                char *str = dynamic_printf("Unknown CSS pseudo-class: [%s]", pseudo_name);
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, token->len);
            }
        }
    }
    else if (token->token_id != TextParser_END && token->token_id == ids->AtRule) {
        char at_name[128];
        get_token_name_lower(text, token, at_name, sizeof(at_name));

        if (!is_css_at_rule(at_name)) {
            char *str = dynamic_printf("Unknown CSS At-Rule: [%s]", at_name);
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, token->len);
        }
    }
}

static void textparser_validate_css_tree(const css_dynamic_token_ids *ids, textparser_validation **ret, textparser_t handle, textparser_token_item *token, int depth) {
    if (depth >= MAX_RECURSION_DEPTH) {
        return;
    }
    while (token != nullptr) {
        textparser_validate_css_token(ids, ret, handle, token);
        if (token->child != nullptr) {
            textparser_validate_css_tree(ids, ret, handle, token->child, depth + 1);
        }
        token = token->next;
    }
}

textparser_validation *textparser_validate_css(textparser_t handle) {
    css_dynamic_token_ids ids;
    resolve_css_token_ids(handle, &ids);

    textparser_validation *ret = nullptr;
    textparser_token_item *token = textparser_get_first_token(handle);
    textparser_validate_css_tree(&ids, &ret, handle, token, 0);

    return ret;
}
