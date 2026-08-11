#include <textparser.h>
#include "adv_regex.h"
#include "logger.h"
#include <os.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>

#ifndef SSIZE_MAX
#define SSIZE_MAX ((ssize_t)((((size_t)-1) << 1) >> 1))
#endif

#define MAX_PARSE_SIZE (16 * 1024 * 1024)

#define TOKEN_NOT_FOUND -1

#define exit_with_error(handle, error_text, offset)   \
    LOGE("Error: %s at %zu", error_text, offset);     \
    if(handle) (handle)->error = error_text;          \
    if(handle) (handle)->error_offset = offset;       \
    goto exit;                                        \

#define check_and_exit_on_fatal_parsing_error(handle, child, offset)                          \
    if ((handle)->error) {                                                                    \
        LOGW("Fatal error detected(%s) at offset %zu. exiting..", (handle)->error, offset);   \
        goto exit;                                                                            \
    }                                                                                         \
    if ((child)->len == 0) {                                                                  \
        LOGW("child->len == 0 detected(%s) at offset %zu. exiting..", (handle)->error ? (handle)->error : "none", offset); \
        goto exit;                                                                            \
    }

enum textparser_inc_bom {
    NO_BOM,
    BOM_UTF_8,
    BOM_UTF_16_BE,
    BOM_UTF_16_LE,
    BOM_UTF_32_BE,
    BOM_UTF_32_LE,
    BOM_UTF_7_1,
    BOM_UTF_7_2,
    BOM_UTF_7_3,
    BOM_UTF_7_4,
    BOM_UTF_7_5,
    BOM_UTF_1,
    BOM_UTF_EBCDIC,
    BOM_UTF_SCSU,
    BOM_UTF_BOCU1,
    BOM_UTF_GB_18030,
};

enum parent_start_stop{
    TEXTPARSER_SEARCH_END_TOKEN,
    TEXTPARSER_SEARCH_START_TOKEN,
};

struct textparser_handle {
    const textparser_language_definition *language;
    void *start_regex;
    void *end_regex;
    void *mmap_addr;
    size_t mmap_size;
    void *owned_buffer;
    enum textparser_inc_bom bom;
    enum textparser_encoding text_format;
    textparser_token_item *first_item;
    size_t error_offset;
    const char *error;
    size_t token_count;
    const char *text_addr;
    size_t text_size;
    size_t no_lines;
    size_t *lines;
    // Arena allocator fields
    void **chunks;
    size_t chunk_count;
    size_t chunk_capacity;
    size_t chunk_size; // size of chunk (for new allocations)
    void *current_chunk;
    size_t current_chunk_index;
    size_t current_chunk_used;
    void (*callback)(textparser_t, textparser_token_item *, enum textparser_callback_type callback_type, void *user_data);
    void *user_data;
    int recursion_depth;
    char *filename;
};

static size_t textparser_get_byte_offset(const struct textparser_handle *handle, size_t pos)
{
    switch (handle->text_format)
    {
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
        return pos * sizeof(uint16_t);
    case TEXTPARSER_ENCODING_UTF_32:
        return pos * sizeof(uint32_t);
    default:
        return pos;
    }
}

static size_t textparser_get_total_units(const struct textparser_handle *handle)
{
    switch (handle->text_format)
    {
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
        return handle->text_size / sizeof(uint16_t);
    case TEXTPARSER_ENCODING_UTF_32:
        return handle->text_size / sizeof(uint32_t);
    default:
        return handle->text_size;
    }
}

static uint32_t textparser_get_unit_at(const struct textparser_handle *handle, size_t pos)
{
    switch (handle->text_format)
    {
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
        return ((const uint16_t *)handle->text_addr)[pos];
    case TEXTPARSER_ENCODING_UTF_32:
        return ((const uint32_t *)handle->text_addr)[pos];
    default:
        return (unsigned char)handle->text_addr[pos];
    }
}

static size_t textparser_char_len(const struct textparser_handle *handle, size_t pos)
{
    if (handle->text_format == TEXTPARSER_ENCODING_UTF_8)
    {
        unsigned char c = (unsigned char)handle->text_addr[pos];
        if ((c & 0x80) == 0) return 1;
        if ((c & 0xE0) == 0xC0) return 2;
        if ((c & 0xF0) == 0xE0) return 3;
        if ((c & 0xF8) == 0xF0) return 4;
        return 1;
    }
    return 1;
}

static void free_arena(struct textparser_handle *handle)
{
    if (handle->chunks) {
        for (size_t i = 0; i < handle->chunk_count; i++) {
            free(handle->chunks[i]);
        }
        free(handle->chunks);
        handle->chunks = nullptr;
    }
    handle->chunk_count = 0;
    handle->chunk_capacity = 0;
    handle->current_chunk = nullptr;
    handle->current_chunk_index = 0;
    handle->current_chunk_used = 0;
}

static textparser_token_item *allocate_token(struct textparser_handle *handle)
{
    size_t token_size = sizeof(textparser_token_item);
    if (handle->current_chunk == nullptr ||
        handle->current_chunk_used + token_size > handle->chunk_size)
    {
        void *new_chunk = malloc(handle->chunk_size);
        if (new_chunk == nullptr) {
            handle->error = "Can't allocate memory!";
            return nullptr;
        }
        memset(new_chunk, 0, handle->chunk_size);

        if (handle->chunk_count >= handle->chunk_capacity) {
            size_t new_capacity = handle->chunk_capacity == 0 ? 4 : handle->chunk_capacity * 2;
            void **new_chunks = realloc(handle->chunks, new_capacity * sizeof(void *));
            if (new_chunks == nullptr) {
                free(new_chunk);
                handle->error = "Can't allocate memory!";
                return nullptr;
            }
            handle->chunks = new_chunks;
            handle->chunk_capacity = new_capacity;
        }

        handle->chunks[handle->chunk_count] = new_chunk;
        handle->current_chunk = new_chunk;
        handle->current_chunk_index = handle->chunk_count;
        handle->chunk_count++;
        handle->current_chunk_used = 0;
    }

    textparser_token_item *ret = (textparser_token_item *)((char *)handle->current_chunk + handle->current_chunk_used);
    handle->current_chunk_used += token_size;
    memset(ret, 0, token_size);
    return ret;
}

static size_t textparser_skip_whitespace(const struct textparser_handle *handle, size_t pos)
{
    size_t maxPos = textparser_get_total_units(handle);

    for (size_t c = pos; c < maxPos; c++)
    {
        uint32_t ch = textparser_get_unit_at(handle, c);

        if ((ch != ' ') && (ch != '\t') && (ch != '\n') && (ch != '\r'))
        {
            return c;
        }
    }

    return maxPos;
}

static const int *get_effective_nested_tokens(const struct textparser_handle *handle, int token_id, const textparser_token_item *parent_item)
{
    const textparser_language_definition *definition = handle->language;
    const textparser_token *token = &definition->tokens[token_id];

    if (token->context_nested_tokens != nullptr) {
        for (int i = 0; token->context_nested_tokens[i].when_parent_in != nullptr; i++) {
            const int *when_parents = token->context_nested_tokens[i].when_parent_in;
            const textparser_token_item *curr = parent_item;
            while (curr != nullptr) {
                int curr_token_id = curr->token_id;
                for (int p = 0; when_parents[p] != TextParser_END; p++) {
                    if (when_parents[p] == curr_token_id) {
                        return token->context_nested_tokens[i].nested_tokens;
                    }
                }
                curr = curr->parent;
            }
        }
    }

    return token->nested_tokens;
}

static void adjust_search_order(const int *original_list, int *adjusted_list)
{
    int count = 0;
    while (original_list[count] != TextParser_END) {
        adjusted_list[count] = original_list[count];
        count++;
    }
    adjusted_list[count] = TextParser_END;

}

static bool textparser_token_in_id_list(const int *list, int token_id)
{
    if (list == nullptr) return false;
    for (int i = 0; list[i] != TextParser_END; i++) {
        if (list[i] == token_id) return true;
    }
    return false;
}

static textparser_token_item *textparser_get_last_child_item(const textparser_token_item *item)
{
    if (item == nullptr || item->child == nullptr) return nullptr;
    textparser_token_item *last = item->child;
    while (last->next) last = last->next;
    return last;
}

// Called right after a child token n is created and linked into its parent's
// child list. When n is a number and the immediately preceding sibling is a sign
// (+/-) in unary context (i.e. the sign is NOT preceded by an operand), the sign
// is absorbed into n and its own token is removed. This also handles a sign that
// is the last child of an operator group (e.g. "12 +-43" -> "12 + -43").
static void maybe_merge_sign(struct textparser_handle *handle, textparser_token_item *n)
{
    const textparser_language_definition *definition = handle->language;
    const textparser_sign_merge *sign_merge = definition->sign_merge;
    if (sign_merge == nullptr) return;
    if (!textparser_token_in_id_list(sign_merge->number_tokens, n->token_id)) return;

    textparser_token_item *prev = n->prev;
    if (prev == nullptr) return;

    textparser_token_item *sign = nullptr;
    textparser_token_item *context = nullptr;

    if (textparser_token_in_id_list(sign_merge->sign_tokens, prev->token_id)) {
        sign = prev;
        context = sign->prev;
    } else {
        textparser_token_item *last = textparser_get_last_child_item(prev);
        if (last != nullptr && textparser_token_in_id_list(sign_merge->sign_tokens, last->token_id)) {
            sign = last;
            context = (sign->prev != nullptr) ? sign->prev : prev->prev;
        } else {
            return;
        }
    }

    // Adjacency only: the sign must touch the number directly.
    if (sign->position + sign->len != n->position) return;

    // Only literal "+" and "-" are signs; never absorb other operators (e.g. "!3").
    if (sign->len != 1) return;
    uint32_t sign_ch = textparser_get_unit_at(handle, sign->position);
    if (sign_ch != '+' && sign_ch != '-') return;

    // Unary context: the token before the sign must not be an operand.
    if (context != nullptr && textparser_token_in_id_list(sign_merge->operand_tokens, context->token_id)) return;

    // Absorb the sign into the number.
    n->position = sign->position;
    n->len += sign->len;

    // Unlink the sign token. In the standalone case sign->next == n, so this
    // also repoints n->prev to the context token (or to null when first child).
    if (sign->prev != nullptr) {
        sign->prev->next = sign->next;
    }
    if (sign->next != nullptr) {
        sign->next->prev = sign->prev;
    }
    if (n->parent != nullptr && n->parent->child == sign) {
        n->parent->child = n;
    }

    // When the sign was the last child of a container (operator group), shrink
    // the container and unwrap it if a single child remains.
    if (prev != sign) {
        if (prev->len >= sign->len) prev->len -= sign->len;
        textparser_token_item *remaining = prev->child;
        if (remaining != nullptr && remaining->next == nullptr) {
            remaining->parent = prev->parent;
            remaining->prev = prev->prev;
            if (prev->prev != nullptr) {
                prev->prev->next = remaining;
            } else if (prev->parent != nullptr) {
                prev->parent->child = remaining;
            }
            remaining->next = n;
            n->prev = remaining;
        }
    }
}

static ssize_t textparser_find_token(const struct textparser_handle *handle, int token_id, size_t pos, bool other_text_inside, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    if (pos >= textparser_get_total_units(handle)) {
        return TOKEN_NOT_FOUND;
    }

    const textparser_language_definition *definition = nullptr;
    const textparser_token *token = nullptr;
    const char *text = nullptr;
    size_t found_at = 0;
    size_t len = 0;

    definition = handle->language;
    if (definition == nullptr || definition->tokens == nullptr || token_id < 0 || (size_t)token_id >= handle->token_count) {
        return TOKEN_NOT_FOUND;
    }

    token = &definition->tokens[token_id];
    text = handle->text_addr + textparser_get_byte_offset(handle, pos);
    len = textparser_get_total_units(handle) - pos;

    LOGV("textparser_find_token token->type [%s] pos %zu", token->name, pos);

    switch(token->type)
    {
        case TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY");
            /* fallthrough */
        case TEXTPARSER_TOKEN_TYPE_GROUP:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_GROUP");
            {
                const int *effective_nested = get_effective_nested_tokens(handle, token_id, parent_item);
                if (effective_nested)
                {
                    int nested_count = 0;
                    while (effective_nested[nested_count] != TextParser_END) {
                        nested_count++;
                    }

                    ssize_t closest_child_pos = SSIZE_MAX;
                    {
                        int adjusted_list[nested_count + 1];
                        adjust_search_order(effective_nested, adjusted_list);

                        for(int c = 0; adjusted_list[c] != TextParser_END; c++)
                        {
                            ssize_t child_token_pos = textparser_find_token(handle, adjusted_list[c], pos, token->other_text_inside, parent_item, prev_sibling);
                            if (child_token_pos == TOKEN_NOT_FOUND) continue;
                            if (child_token_pos == 0) {
                                closest_child_pos = 0;
                                break;
                            }

                            if (child_token_pos < closest_child_pos) {
                                closest_child_pos = child_token_pos;
                            }
                        }
                    }

                    if (closest_child_pos < SSIZE_MAX) {
                        return closest_child_pos;
                    }
                }
            }
            break;
        case TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER:
            {
                const int *effective_nested = get_effective_nested_tokens(handle, token_id, parent_item);
                if (effective_nested) {
                    LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER");
                    return textparser_find_token(handle, effective_nested[0], pos, other_text_inside, parent_item, prev_sibling);
                }
                LOGE("nested_tokens = nullptr for TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER");
            }
            break;
        case TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN");
            /* fallthrough */
        case TEXTPARSER_TOKEN_TYPE_START_STOP:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_START_STOP");
            /* fallthrough */
        case TEXTPARSER_TOKEN_TYPE_START_OPT_STOP:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_START_OPT_STOP");
            if (adv_regex_find_pattern(token->start_regex, (void **)handle->start_regex + token_id, handle->text_format, text, len, &found_at, nullptr, !handle->language->case_sensitivity, true)) {
                LOGI("found_at token type: [%s] at %zu",  handle->language->tokens[token_id].name, pos + found_at);
                return (ssize_t)found_at;
            }
            break;
        default:
            LOGF("textparser_find_token() - unknown!!!!!");
            break;
    }

    return TOKEN_NOT_FOUND;
}

static  void parse_token_error_error(struct textparser_handle *handle, const char *error, size_t offset)
{
    if (handle == nullptr)
    {
        LOGF("handle == nullptr");
        return;
    }

    LOGF("parse_token_error_error(). error %s.", error);

    handle->error_offset = offset;
    handle->error = error;
}

typedef struct {
    int token_id;
    int parent_token_id;
    int parent_start_stop;
    size_t offset;
    textparser_token_item *ret_item;
    const textparser_token_item *parent_item;
    const textparser_token_item *prev_sibling;
    int state_step;
    union {
        struct {
            textparser_token_item *child;
        } group;
        struct {
            textparser_token_item *last_child;
            const int *nested_tokens;
            size_t end_match_len;
        } start_stop;
        struct {
            textparser_token_item *last_child;
            int start_token_id;
            int inner_token_id;
            int end_token_id;
        } group_all;
    } u;
} ParserStackFrame;

typedef struct {
    ParserStackFrame *frames;
    size_t size;
    size_t capacity;
} ParserStack;

static void push_frame(ParserStack *stack, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling) {
    if (stack->size >= stack->capacity) {
        stack->capacity = stack->capacity == 0 ? 16 : stack->capacity * 2;
        stack->frames = realloc(stack->frames, stack->capacity * sizeof(ParserStackFrame));
    }
    ParserStackFrame *f = &stack->frames[stack->size++];
    memset(f, 0, sizeof(ParserStackFrame));
    f->token_id = token_id;
    f->parent_token_id = parent_token_id;
    f->parent_start_stop = parent_start_stop;
    f->offset = offset;
    f->parent_item = parent_item;
    f->prev_sibling = prev_sibling;
    f->state_step = 0;
}

static void textparser_init_regex(struct textparser_handle *handle)
{
    int token_cnt = 0;

    if (handle == nullptr)
        return;

    while(handle->language->tokens[token_cnt].name != nullptr)
        token_cnt++;

    handle->token_count = (size_t)token_cnt;

    if (token_cnt > 0)
    {
        size_t malloc_size = (size_t)token_cnt * sizeof(void *);

        handle->start_regex = malloc(malloc_size);
        if (handle->start_regex == nullptr) {
            LOGE("malloc() failed for start_regex");
            return;
        }
        memset(handle->start_regex, 0, malloc_size);

        handle->end_regex = malloc(malloc_size);
        if (handle->end_regex == nullptr) {
            LOGE("malloc() failed for end_regex");
            free(handle->start_regex);
            handle->start_regex = nullptr;
            return;
        }
        memset(handle->end_regex, 0, malloc_size);
    }
}

static void textparser_free_regex(struct textparser_handle *handle)
{
    enum textparser_encoding text_format = TEXTPARSER_ENCODING_NONE;

    if ((handle == nullptr)||((handle->start_regex == nullptr)&&(handle->end_regex == nullptr)))
        return;

    text_format = handle->text_format;

    size_t token_cnt = handle->token_count;

    if (handle->start_regex)
    {
        if (token_cnt > 0)
        {
            void **regex = (void **)handle->start_regex;

            for(size_t c = 0; c < token_cnt; c++)
            {
                adv_regex_free(&regex[c], text_format);
            }
        }
        free(handle->start_regex);
        handle->start_regex = nullptr;
    }

    if (handle->end_regex)
    {
        if (token_cnt > 0)
        {
            void **regex = (void **)handle->end_regex;

            for(size_t c = 0; c < token_cnt; c++)
            {
                adv_regex_free(&regex[c], text_format);
            }
        }
        free(handle->end_regex);
        handle->end_regex = nullptr;
    }
}

static void link_new_token(struct textparser_handle *handle, textparser_token_item *ret, int parent_token_id, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling, textparser_token_item **p_prev_item) {
    if (parent_token_id == TextParser_END && parent_item == nullptr) {
        if (handle->first_item == nullptr) {
            handle->first_item = ret;
        }
        if (*p_prev_item) {
            (*p_prev_item)->next = ret;
            ret->prev = *p_prev_item;
        }
        *p_prev_item = ret;
    } else if (parent_item) {
        ret->parent = (textparser_token_item *)parent_item;
        if (parent_item->child == nullptr) {
            ((textparser_token_item *)parent_item)->child = ret;
        } else if (prev_sibling && prev_sibling != parent_item) {
            ret->prev = (textparser_token_item *)prev_sibling;
            ((textparser_token_item *)prev_sibling)->next = ret;
        }
    }
}

int textparser_parse_incremental(textparser_t handle, const textparser_language_definition *definition, textparser_parser_state *state, size_t start_pos, size_t end_pos)
{
    if (handle == nullptr || definition == nullptr)
        return -1;

    handle->error = nullptr;
    handle->error_offset = 0;

    if (!state) {
        free_arena(handle);
        handle->first_item = nullptr;
    }

    if (handle->language != definition) {
        textparser_free_regex(handle);
        handle->language = definition;
        textparser_init_regex(handle);
    }

    ParserStack stack;
    stack.frames = nullptr;
    stack.size = 0;
    stack.capacity = 0;

    if (state && start_pos > 0 && start_pos - 1 < (size_t)state->len) {
        const textparser_token_item *curr = state->state[start_pos - 1];
        while (curr) {
            const textparser_token *def = &definition->tokens[curr->token_id];
            bool is_container = (def->type == TEXTPARSER_TOKEN_TYPE_GROUP ||
                                 def->type == TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER ||
                                 def->type == TEXTPARSER_TOKEN_TYPE_START_STOP ||
                                 def->type == TEXTPARSER_TOKEN_TYPE_START_OPT_STOP);
            if (is_container && start_pos < curr->position + curr->len) {
                break;
            }
            curr = curr->parent;
        }

        const textparser_token_item *ancestors[128];
        int count = 0;
        while (curr && count < 128) {
            ancestors[count++] = curr;
            curr = curr->parent;
        }

        for (int i = count - 1; i >= 0; i--) {
            const textparser_token_item *ancestor = ancestors[i];
            const textparser_token *def = &definition->tokens[ancestor->token_id];
            
            int parent_token_id = TextParser_END;
            if (i < count - 1) {
                parent_token_id = ancestors[i + 1]->token_id;
            }
            int parent_start_stop = TEXTPARSER_SEARCH_END_TOKEN;
            
            textparser_token_item *last_child = nullptr;
            textparser_token_item *c = ancestor->child;
            while (c) {
                if (c->position + c->len <= start_pos) {
                    last_child = c;
                }
                c = c->next;
            }
            
            push_frame(&stack, ancestor->token_id, parent_token_id, parent_start_stop, start_pos, ancestor->parent, last_child);
            ParserStackFrame *frame = &stack.frames[stack.size - 1];
            frame->ret_item = (textparser_token_item *)ancestor;
            frame->state_step = 1;
            
            if (def->type == TEXTPARSER_TOKEN_TYPE_GROUP) {
                frame->u.group.child = last_child;
            } else if (def->type == TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER) {
                frame->u.group_all.last_child = last_child;
                frame->u.group_all.start_token_id = def->nested_tokens[0];
                frame->u.group_all.inner_token_id = def->nested_tokens[1];
                frame->u.group_all.end_token_id   = def->nested_tokens[2];
                frame->state_step = 2;
            } else if (def->type == TEXTPARSER_TOKEN_TYPE_START_STOP || def->type == TEXTPARSER_TOKEN_TYPE_START_OPT_STOP) {
                frame->u.start_stop.last_child = last_child;
                frame->u.start_stop.nested_tokens = def->nested_tokens;
            }
        }
    }

    size_t pos = start_pos;
    textparser_token_item *prev_item = nullptr;

    if (state && start_pos > 0 && start_pos - 1 < (size_t)state->len) {
        const textparser_token_item *curr = state->state[start_pos - 1];
        while (curr && curr->parent != nullptr) {
            curr = curr->parent;
        }
        if (curr && curr->position + curr->len <= start_pos) {
            prev_item = (textparser_token_item *)curr;
        }
    }

    const int *effective_starts_with = definition->starts_with;

    if (definition->override_start_tokens && handle->filename) {
        const char *file_ext = strrchr(handle->filename, '.');
        if (file_ext) {
            file_ext++;
            for (int r = 0; definition->override_start_tokens[r].file_extensions != nullptr ||
                            definition->override_start_tokens[r].regex != nullptr ||
                            definition->override_start_tokens[r].start_tokens != nullptr; r++) {
                const textparser_override_start_token_rule *rule = &definition->override_start_tokens[r];
                bool ext_matches = false;
                if (rule->file_extensions) {
                    for (int e = 0; rule->file_extensions[e] != nullptr; e++) {
                        bool match = false;
                        if (definition->case_sensitivity) {
                            match = (strcmp(file_ext, rule->file_extensions[e]) == 0);
                        } else {
#ifdef _WIN32
                            match = (_stricmp(file_ext, rule->file_extensions[e]) == 0);
#else
                            match = (strcasecmp(file_ext, rule->file_extensions[e]) == 0);
#endif
                        }
                        if (match) {
                            ext_matches = true;
                            break;
                        }
                    }
                }

                if (ext_matches && rule->regex && rule->start_tokens) {
                    void *rule_regex = nullptr;
                    size_t found_at = 0;
                    size_t found_len = 0;
                    bool matched = adv_regex_find_pattern(rule->regex, &rule_regex, handle->text_format, handle->text_addr, handle->text_size, &found_at, &found_len, !definition->case_sensitivity, true);
                    if (rule_regex) {
                        adv_regex_free(&rule_regex, handle->text_format);
                    }
                    if (matched) {
                        effective_starts_with = rule->start_tokens;
                        break;
                    }
                }
            }
        }
    }

    textparser_token_item *child_result = nullptr;

    while (pos < end_pos || stack.size > 0) {
        if (handle->error) {
            break;
        }

        if (stack.size == 0) {
            pos = textparser_skip_whitespace(handle, pos);
            if (pos >= end_pos) {
                break;
            }

            int matched_token_id = TextParser_END;
            int count = 0;
            while (effective_starts_with[count] != TextParser_END) {
                count++;
            }
            {
                int adjusted_list[count + 1];
                adjust_search_order(effective_starts_with, adjusted_list);

                for (int c = 0; adjusted_list[c] != TextParser_END; c++) {
                    int token_id = adjusted_list[c];
                    ssize_t offset = textparser_find_token(handle, token_id, pos, definition->other_text_inside, nullptr, prev_item);
                    if (offset == 0) {
                        matched_token_id = token_id;
                        break;
                    }
                }
            }

            if (matched_token_id != TextParser_END) {
                push_frame(&stack, matched_token_id, TextParser_END, TEXTPARSER_SEARCH_END_TOKEN, pos, nullptr, prev_item);
            } else {
                if (definition->other_text_inside) {
                    size_t char_l = textparser_char_len(handle, pos);
                    if (prev_item && prev_item->token_id == TEXTPARSER_TOKEN_ID_ERROR) {
                        prev_item->len += char_l;
                    } else {
                        textparser_token_item *err_item = allocate_token(handle);
                        if (err_item) {
                            memset(err_item, 0, sizeof(textparser_token_item));
                            err_item->token_id = TEXTPARSER_TOKEN_ID_ERROR;
                            err_item->position = pos;
                            err_item->len = char_l;
                            err_item->text_color = TEXTPARSER_NOCOLOR;
                            err_item->text_background = TEXTPARSER_NOCOLOR;
                            if (handle->first_item == nullptr) {
                                handle->first_item = err_item;
                            }
                            if (prev_item) {
                                prev_item->next = err_item;
                                err_item->prev = prev_item;
                            }
                            prev_item = err_item;
                        }
                    }
                    pos += char_l;
                } else {
                    break;
                }
            }
            continue;
        }

        ParserStackFrame *f = &stack.frames[stack.size - 1];
        const textparser_token *token_def = &definition->tokens[f->token_id];

        switch (token_def->type) {
            case TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN: {
                if (f->state_step == 0) {
                    if (f->offset >= textparser_get_total_units(handle)) {
                        parse_token_error_error(handle, "offset >= total units count!", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    textparser_token_item *ret = allocate_token(handle);
                    if (ret == nullptr) {
                        parse_token_error_error(handle, "Can't allocate memory!", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    link_new_token(handle, ret, f->parent_token_id, f->parent_item, f->prev_sibling, &prev_item);
                    size_t offset = textparser_skip_whitespace(handle, f->offset);
                    ret->token_id = f->token_id;
                    ret->position = offset;

                    size_t len = 0;
                    if (!adv_regex_find_pattern(token_def->start_regex, (void **)handle->start_regex + f->token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &len, !handle->language->case_sensitivity, true)) {
                        parse_token_error_error(handle, "Can't find start of the token!", offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    ret->position = offset;
                    ret->len = len;
                    ret->text_color = token_def->text_color;
                    ret->text_background = token_def->text_background;
                    ret->text_flags = token_def->text_flags;

                    if (handle->callback) {
                        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
                    }
                    child_result = ret;
                    
                    if (f->parent_token_id == TextParser_END && f->parent_item == nullptr) {

                    
                        pos = ret->position + ret->len;

                    
                        prev_item = ret;

                        maybe_merge_sign(handle, ret);

                    
                    }
                    stack.size--;
                }
                break;
            }

            case TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY: {
                if (f->state_step == 0) {
                    if (!token_def->nested_tokens) {
                        parse_token_error_error(handle, "group_one_child token type nested_tokens list is empty!", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    textparser_token_item *ret = allocate_token(handle);
                    if (ret == nullptr) {
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    link_new_token(handle, ret, f->parent_token_id, f->parent_item, f->prev_sibling, &prev_item);
                    size_t offset = textparser_skip_whitespace(handle, f->offset);
                    ret->token_id = f->token_id;
                    ret->position = offset;
                    ret->text_color = token_def->text_color;
                    ret->text_background = token_def->text_background;
                    ret->text_flags = token_def->text_flags;
                    f->ret_item = ret;

                    const int *effective_nested = get_effective_nested_tokens(handle, f->token_id, ret);
                    int count = 0;
                    while (effective_nested[count] != TextParser_END) {
                        count++;
                    }

                    size_t closest = SIZE_MAX;
                    int current_token_id = TextParser_END;
                    {
                        int adjusted_list[count + 1];
                        adjust_search_order(effective_nested, adjusted_list);

                        for (int c = 0; adjusted_list[c] != TextParser_END; c++) {
                            ssize_t current_closest = textparser_find_token(handle, adjusted_list[c], offset, token_def->other_text_inside, ret, f->prev_sibling);
                            if ((current_closest >= 0) && ((size_t)current_closest < closest)) {
                                closest = (size_t)current_closest;
                                current_token_id = adjusted_list[c];
                            }
                        }
                    }

                    if (current_token_id == TextParser_END) {
                        parse_token_error_error(handle, "Search for group_one_child token type failed. Can't find one child.", offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }

                    f->state_step = 1;
                    push_frame(&stack, current_token_id, f->parent_token_id, f->parent_start_stop, offset, ret, f->prev_sibling);
                } else if (f->state_step == 1) {
                    textparser_token_item *child = child_result;
                    textparser_token_item *ret = f->ret_item;
                    if (child == nullptr) {
                        parse_token_error_error(handle, "Search for group_one_child token type failed. Child token parsing failed.", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    child->parent = ret;
                    ret->position = child->position;
                    ret->len = child->len;
                    ret->child = child;

                    if (handle->callback) {
                        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
                    }
                    child_result = ret;
                    
                    if (f->parent_token_id == TextParser_END && f->parent_item == nullptr) {

                    
                        pos = ret->position + ret->len;

                    
                        prev_item = ret;

                    
                    }
                    stack.size--;
                }
                break;
            }

            case TEXTPARSER_TOKEN_TYPE_GROUP: {
                if (f->state_step == 0) {
                    if (!token_def->nested_tokens) {
                        parse_token_error_error(handle, "nested_tokens list is empty!", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    textparser_token_item *ret = allocate_token(handle);
                    if (ret == nullptr) {
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    link_new_token(handle, ret, f->parent_token_id, f->parent_item, f->prev_sibling, &prev_item);
                    size_t offset = textparser_skip_whitespace(handle, f->offset);
                    ret->token_id = f->token_id;
                    ret->position = offset;
                    ret->text_color = token_def->text_color;
                    ret->text_background = token_def->text_background;
                    ret->text_flags = token_def->text_flags;
                    f->ret_item = ret;
                    f->offset = offset;
                    f->u.group.child = nullptr;
                    f->state_step = 1;
                }

                if (f->state_step == 1) {
                    textparser_token_item *ret = f->ret_item;
                    f->offset = textparser_skip_whitespace(handle, f->offset);
                    if (f->offset >= textparser_get_total_units(handle)) {
                        if (f->u.group.child) {
                            ret->len = f->offset - ret->position;
                            child_result = ret;
                            if (f->parent_token_id == TextParser_END && f->parent_item == nullptr) {

                                pos = ret->position + ret->len;

                                prev_item = ret;

                            }
                            stack.size--;
                        } else {
                            parse_token_error_error(handle, "Search for group token type failed. Can't find any child.", f->offset);
                            child_result = nullptr;
                            stack.size--;
                        }
                        break;
                    }

                    const textparser_token_item *current_prev = (f->u.group.child == nullptr) ? f->prev_sibling : f->u.group.child;
                    const char *parent_regex_pattern = nullptr;
                    void **parent_regex_compiled_ptr = nullptr;

                    if (f->parent_token_id != TextParser_END) {
                        switch (f->parent_start_stop) {
                            case TEXTPARSER_SEARCH_END_TOKEN:
                                parent_regex_pattern = definition->tokens[f->parent_token_id].end_regex;
                                parent_regex_compiled_ptr = (void **)handle->end_regex + f->parent_token_id;
                                break;
                            case TEXTPARSER_SEARCH_START_TOKEN:
                                parent_regex_pattern = definition->tokens[f->parent_token_id].start_regex;
                                parent_regex_compiled_ptr = (void **)handle->start_regex + f->parent_token_id;
                                break;
                            default:
                                parse_token_error_error(handle, "Unknown parent_start_stop value!", f->offset);
                                child_result = nullptr;
                                stack.size--;
                                break;
                        }
                        if (handle->error) break;
                    }

                    if (parent_regex_pattern) {
                        size_t parent_match_len = 0;
                        bool found_parent_token = adv_regex_find_pattern(parent_regex_pattern, parent_regex_compiled_ptr, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, f->offset), textparser_get_total_units(handle) - f->offset, nullptr, &parent_match_len, !handle->language->case_sensitivity, true);

                        if (found_parent_token) {
                            ret->len = f->offset - ret->position;
                            child_result = ret;
                            if (f->parent_token_id == TextParser_END && f->parent_item == nullptr) {

                                pos = ret->position + ret->len;

                                prev_item = ret;

                            }
                            stack.size--;
                            break;
                        }
                    }

                    const int *effective_nested = get_effective_nested_tokens(handle, f->token_id, ret);
                    int current_token_id = TextParser_END;
                    int count = 0;
                    while (effective_nested[count] != TextParser_END) {
                        count++;
                    }

                    {
                        int adjusted_list[count + 1];
                        adjust_search_order(effective_nested, adjusted_list);

                        for (int c = 0; adjusted_list[c] != TextParser_END; c++) {
                            ssize_t current_closest = textparser_find_token(handle, adjusted_list[c], f->offset, token_def->other_text_inside, ret, current_prev);
                            if (current_closest == 0) {
                                current_token_id = adjusted_list[c];
                                break;
                            }
                        }
                    }

                    if (current_token_id != TextParser_END) {
                        f->state_step = 2;
                        push_frame(&stack, current_token_id, f->parent_token_id, f->parent_start_stop, f->offset, ret, current_prev);
                    } else {
                        if (token_def->other_text_inside && f->offset < textparser_get_total_units(handle)) {
                            f->offset += textparser_char_len(handle, f->offset);
                        } else {
                            if (f->u.group.child) {
                                ret->len = f->offset - ret->position;
                                child_result = ret;
                                if (f->parent_token_id == TextParser_END && f->parent_item == nullptr) {

                                    pos = ret->position + ret->len;

                                    prev_item = ret;

                                }
                                stack.size--;
                            } else {
                                parse_token_error_error(handle, "Unrecognized token inside group", f->offset);
                                child_result = nullptr;
                                stack.size--;
                            }
                        }
                    }
                } else if (f->state_step == 2) {
                    textparser_token_item *child = child_result;
                    textparser_token_item *ret = f->ret_item;
                    if (child == nullptr) {
                        if (token_def->other_text_inside && f->offset < textparser_get_total_units(handle)) {
                            handle->error = nullptr;
                            handle->error_offset = 0;
                            f->offset += textparser_char_len(handle, f->offset);
                            f->state_step = 1;
                            break;
                        }
                        parse_token_error_error(handle, "Parsing child token failed", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    child->parent = ret;
                    if (f->u.group.child == nullptr) {
                        ret->child = child;
                    } else {
                        child->prev = f->u.group.child;
                        f->u.group.child->next = child;
                    }
                    f->u.group.child = child;

                    maybe_merge_sign(handle, child);

                    if (child->len == 0) {
                        parse_token_error_error(handle, "0-length child token match caused infinite loop", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }

                    f->offset = child->position + child->len;
                    ret->len = child->position + child->len - ret->position;
                    f->state_step = 1;
                }
                break;
            }

            case TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER: {
                if (f->state_step == 0) {
                    const int *effective_nested = get_effective_nested_tokens(handle, f->token_id, f->parent_item);
                    if (!effective_nested) {
                        parse_token_error_error(handle, "nested_tokens list is empty!", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    int nested_count = 0;
                    while (effective_nested[nested_count] != TextParser_END) nested_count++;
                    if (nested_count != 3) {
                        parse_token_error_error(handle, "GroupAllChildrenInSameOrder should have exactly 3 nested tokens", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }

                    f->u.group_all.start_token_id = effective_nested[0];
                    f->u.group_all.inner_token_id = effective_nested[1];
                    f->u.group_all.end_token_id   = effective_nested[2];

                    textparser_token_item *ret = allocate_token(handle);
                    if (ret == nullptr) {
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    link_new_token(handle, ret, f->parent_token_id, f->parent_item, f->prev_sibling, &prev_item);
                    size_t offset = textparser_skip_whitespace(handle, f->offset);
                    ret->token_id = f->token_id;
                    ret->position = offset;
                    ret->text_color = token_def->text_color;
                    ret->text_background = token_def->text_background;
                    ret->text_flags = token_def->text_flags;
                    f->ret_item = ret;
                    f->offset = offset;

                    ssize_t start_pos = textparser_find_token(handle, f->u.group_all.start_token_id, offset, definition->other_text_inside, f->parent_item, f->prev_sibling);
                    if (start_pos != 0) {
                        parse_token_error_error(handle, "Expected start token!", offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }

                    f->state_step = 1;
                    push_frame(&stack, f->u.group_all.start_token_id, f->token_id, TEXTPARSER_SEARCH_END_TOKEN, offset, ret, f->prev_sibling);
                } else if (f->state_step == 1) {
                    textparser_token_item *child = child_result;
                    textparser_token_item *ret = f->ret_item;
                    if (child == nullptr) {
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    child->parent = ret;
                    ret->child = child;
                    f->u.group_all.last_child = child;
                    f->offset = child->position + child->len;
                    f->state_step = 2;
                } else if (f->state_step == 2) {
                    textparser_token_item *ret = f->ret_item;
                    f->offset = textparser_skip_whitespace(handle, f->offset);
                    if (f->offset >= textparser_get_total_units(handle)) {
                        parse_token_error_error(handle, "Expected end token, reached end of text!", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }

                    ssize_t end_pos = textparser_find_token(handle, f->u.group_all.end_token_id, f->offset, definition->other_text_inside, ret, f->u.group_all.last_child);
                    if (end_pos == 0) {
                        size_t offset = textparser_skip_whitespace(handle, f->offset);
                        f->state_step = 3;
                        push_frame(&stack, f->u.group_all.end_token_id, f->token_id, TEXTPARSER_SEARCH_END_TOKEN, offset, ret, f->u.group_all.last_child);
                    } else {
                        ssize_t inner_pos = textparser_find_token(handle, f->u.group_all.inner_token_id, f->offset, definition->other_text_inside, ret, f->u.group_all.last_child);
                        if (inner_pos == 0) {
                            f->state_step = 4;
                            push_frame(&stack, f->u.group_all.inner_token_id, f->u.group_all.end_token_id, TEXTPARSER_SEARCH_START_TOKEN, f->offset, ret, f->u.group_all.last_child);
                        } else {
                            if (definition->other_text_inside) {
                                f->offset += textparser_char_len(handle, f->offset);
                            } else {
                                parse_token_error_error(handle, "Expected inner or end token!", f->offset);
                                child_result = nullptr;
                                stack.size--;
                                break;
                            }
                        }
                    }
                } else if (f->state_step == 4) {
                    textparser_token_item *child = child_result;
                    textparser_token_item *ret = f->ret_item;
                    if (child == nullptr) {
                        if (definition->other_text_inside && f->offset < textparser_get_total_units(handle)) {
                            handle->error = nullptr;
                            handle->error_offset = 0;
                            f->offset += textparser_char_len(handle, f->offset);
                            f->state_step = 2;
                            break;
                        }
                        parse_token_error_error(handle, "Parsing inner token failed", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    child->parent = ret;
                    child->prev = f->u.group_all.last_child;
                    f->u.group_all.last_child->next = child;
                    f->u.group_all.last_child = child;

                    if (child->len == 0) {
                        parse_token_error_error(handle, "0-length child token match caused infinite loop", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }

                    f->offset = child->position + child->len;
                    f->state_step = 2;
                } else if (f->state_step == 3) {
                    textparser_token_item *child = child_result;
                    textparser_token_item *ret = f->ret_item;
                    if (child == nullptr) {
                        parse_token_error_error(handle, "Parsing end token failed", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    child->parent = ret;
                    child->prev = f->u.group_all.last_child;
                    f->u.group_all.last_child->next = child;

                    ret->len = child->position + child->len - ret->position;

                    if (handle->callback) {
                        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
                    }
                    child_result = ret;
                    
                    if (f->parent_token_id == TextParser_END && f->parent_item == nullptr) {

                    
                        pos = ret->position + ret->len;

                    
                        prev_item = ret;

                    
                    }
                    stack.size--;
                }
                break;
            }

            case TEXTPARSER_TOKEN_TYPE_START_STOP:
            case TEXTPARSER_TOKEN_TYPE_START_OPT_STOP: {
                bool stop_required = (token_def->type == TEXTPARSER_TOKEN_TYPE_START_STOP);
                if (f->state_step == 0) {
                    if (f->offset >= textparser_get_total_units(handle)) {
                        parse_token_error_error(handle, "offset >= total units count!", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    textparser_token_item *ret = allocate_token(handle);
                    if (ret == nullptr) {
                        parse_token_error_error(handle, "Can't allocate memory!", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    link_new_token(handle, ret, f->parent_token_id, f->parent_item, f->prev_sibling, &prev_item);
                    size_t offset = textparser_skip_whitespace(handle, f->offset);
                    ret->token_id = f->token_id;
                    ret->position = offset;
                    ret->text_color = token_def->text_color;
                    ret->text_background = token_def->text_background;
                    ret->text_flags = token_def->text_flags;
                    f->ret_item = ret;

                    size_t len = 0;
                    if (!adv_regex_find_pattern(token_def->start_regex, (void **)handle->start_regex + f->token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &len, !handle->language->case_sensitivity, true)) {
                        parse_token_error_error(handle, "Can't find start of the token!", offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    ret->position = offset;
                    f->offset = offset + len;
                    ret->len = len;

                    if (handle->callback) {
                        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_START, handle->user_data);
                    }

                    if (f->offset > textparser_get_total_units(handle)) {
                        parse_token_error_error(handle, "offset >= total units count!", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }

                    if (f->offset == textparser_get_total_units(handle)) {
                        if (stop_required) {
                            parse_token_error_error(handle, "reached end of text!", f->offset);
                            child_result = nullptr;
                            stack.size--;
                        } else {
                            ret->len = f->offset - ret->position;
                            if (handle->callback) {
                                handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
                            }
                            child_result = ret;
                            if (f->parent_token_id == TextParser_END && f->parent_item == nullptr) {

                                pos = ret->position + ret->len;

                                prev_item = ret;

                            }
                            stack.size--;
                        }
                        break;
                    }

                    const int *effective_nested = get_effective_nested_tokens(handle, f->token_id, ret);
                    if (effective_nested) {
                        f->u.start_stop.last_child = nullptr;
                        f->u.start_stop.nested_tokens = effective_nested;
                        f->state_step = 1;
                    } else {
                        size_t end_match_len = 0;
                        size_t token_end = 0;
                        bool found_end = adv_regex_find_pattern(token_def->end_regex, (void **)handle->end_regex + f->token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, f->offset), textparser_get_total_units(handle) - f->offset, &token_end, &end_match_len, !handle->language->case_sensitivity, false);
                        if (!found_end) {
                            if (stop_required) {
                                parse_token_error_error(handle, "Can't find end of the token!", f->offset);
                                child_result = nullptr;
                                stack.size--;
                            } else {
                                ret->len = textparser_get_total_units(handle) - ret->position;
                                if (handle->callback) {
                                    handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
                                }
                                child_result = ret;
                                if (f->parent_token_id == TextParser_END && f->parent_item == nullptr) {

                                    pos = ret->position + ret->len;

                                    prev_item = ret;

                                }
                                stack.size--;
                            }
                        } else {
                            ret->len = f->offset + token_end + end_match_len - ret->position;
                            if (handle->callback) {
                                handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
                            }
                            child_result = ret;
                            if (f->parent_token_id == TextParser_END && f->parent_item == nullptr) {

                                pos = ret->position + ret->len;

                                prev_item = ret;

                            }
                            stack.size--;
                        }
                    }
                } else if (f->state_step == 1) {
                    textparser_token_item *ret = f->ret_item;
                    f->offset = textparser_skip_whitespace(handle, f->offset);

                    if (f->offset >= textparser_get_total_units(handle)) {
                        if (stop_required) {
                            parse_token_error_error(handle, "Reached end of text before finding end token!", f->offset);
                            child_result = nullptr;
                            stack.size--;
                        } else {
                            ret->len = f->offset - ret->position;
                            if (handle->callback) {
                                handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
                            }
                            child_result = ret;
                            if (f->parent_token_id == TextParser_END && f->parent_item == nullptr) {

                                pos = ret->position + ret->len;

                                prev_item = ret;

                            }
                            stack.size--;
                        }
                        break;
                    }

                    if (token_def->search_parent_end_token_last == false) {
                        size_t end_match_len = 0;
                        bool found_end = adv_regex_find_pattern(token_def->end_regex, (void **)handle->end_regex + f->token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, f->offset), textparser_get_total_units(handle) - f->offset, nullptr, &end_match_len, !handle->language->case_sensitivity, true);
                        if (found_end) {
                            f->u.start_stop.end_match_len = end_match_len;
                            f->state_step = 3;
                            break;
                        }
                    }

                    int child_token_id = TextParser_END;
                    const textparser_token_item *current_prev = (f->u.start_stop.last_child == nullptr) ? ret : f->u.start_stop.last_child;

                    int nested_count = 0;
                    while (f->u.start_stop.nested_tokens[nested_count] != TextParser_END) {
                        nested_count++;
                    }

                    {
                        int adjusted_list[nested_count + 1];
                        adjust_search_order(f->u.start_stop.nested_tokens, adjusted_list);

                        for (int c = 0; adjusted_list[c] != TextParser_END; c++) {
                            ssize_t pos = textparser_find_token(handle, adjusted_list[c], f->offset, token_def->other_text_inside, ret, current_prev);
                            if (pos == 0) {
                                child_token_id = adjusted_list[c];
                                break;
                            }
                        }
                    }

                    if (child_token_id != TextParser_END) {
                        f->state_step = 2;
                        push_frame(&stack, child_token_id, f->token_id, TEXTPARSER_SEARCH_END_TOKEN, f->offset, ret, current_prev);
                    } else {
                        if (token_def->search_parent_end_token_last == true) {
                            size_t end_match_len = 0;
                            bool found_end = adv_regex_find_pattern(token_def->end_regex, (void **)handle->end_regex + f->token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, f->offset), textparser_get_total_units(handle) - f->offset, nullptr, &end_match_len, !handle->language->case_sensitivity, true);
                            if (found_end) {
                                f->u.start_stop.end_match_len = end_match_len;
                                f->state_step = 3;
                                break;
                            }
                        }

                        if (token_def->other_text_inside) {
                            f->offset += textparser_char_len(handle, f->offset);
                        } else {
                            parse_token_error_error(handle, "Unexpected token inside start-stop block!", f->offset);
                            child_result = nullptr;
                            stack.size--;
                        }
                    }
                } else if (f->state_step == 2) {
                    textparser_token_item *child = child_result;
                    textparser_token_item *ret = f->ret_item;
                    if (child == nullptr) {
                        parse_token_error_error(handle, "Parsing nested child token failed", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }
                    child->parent = ret;
                    if (f->u.start_stop.last_child) {
                        child->prev = f->u.start_stop.last_child;
                        f->u.start_stop.last_child->next = child;
                    }
                    if (ret->child == nullptr) {
                        ret->child = child;
                    }
                    f->u.start_stop.last_child = child;

                    maybe_merge_sign(handle, child);

                    if (child->len == 0) {
                        parse_token_error_error(handle, "0-length child token match caused infinite loop", f->offset);
                        child_result = nullptr;
                        stack.size--;
                        break;
                    }

                    f->offset = child->position + child->len;
                    f->state_step = 1;
                } else if (f->state_step == 3) {
                    textparser_token_item *ret = f->ret_item;
                    f->offset = textparser_skip_whitespace(handle, f->offset);

                    if (f->offset >= textparser_get_total_units(handle)) {
                        if (stop_required) {
                            parse_token_error_error(handle, "offset >= total units count!", f->offset);
                            child_result = nullptr;
                            stack.size--;
                        } else {
                            ret->len = f->offset - ret->position;
                            if (handle->callback) {
                                handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
                            }
                            child_result = ret;
                            if (f->parent_token_id == TextParser_END && f->parent_item == nullptr) {

                                pos = ret->position + ret->len;

                                prev_item = ret;

                            }
                            stack.size--;
                        }
                        break;
                    }

                    size_t token_end = 0;
                    size_t end_len = 0;
                    bool found_end = adv_regex_find_pattern(token_def->end_regex, (void **)handle->end_regex + f->token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, f->offset), textparser_get_total_units(handle) - f->offset, &token_end, &end_len, !handle->language->case_sensitivity, false);

                    if (!found_end) {
                        if (stop_required) {
                            parse_token_error_error(handle, "Can't find end of the token!", f->offset);
                            child_result = nullptr;
                            stack.size--;
                        } else {
                            ret->len = f->offset - ret->position;
                            if (handle->callback) {
                                handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
                            }
                            child_result = ret;
                            if (f->parent_token_id == TextParser_END && f->parent_item == nullptr) {

                                pos = ret->position + ret->len;

                                prev_item = ret;

                            }
                            stack.size--;
                        }
                    } else {
                        ret->len = f->offset + token_end + end_len - ret->position;
                        if (handle->callback) {
                            handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
                        }
                        child_result = ret;
                        if (f->parent_token_id == TextParser_END && f->parent_item == nullptr) {

                            pos = ret->position + ret->len;

                            prev_item = ret;

                        }
                        stack.size--;
                    }
                }
                break;
            }
        }
    }

    free(stack.frames);
    if (handle->error) {
        return -1;
    }
    return 0;
}

static const textparser_token_item *find_token_at_position(const textparser_token_item *token, size_t pos) {
    const textparser_token_item *best = nullptr;
    while (token != nullptr) {
        if (token->position <= pos && pos < token->position + token->len) {
            best = token;
            if (token->child) {
                const textparser_token_item *child_best = find_token_at_position(token->child, pos);
                if (child_best) {
                    best = child_best;
                }
            }
            break;
        }
        token = token->next;
    }
    return best;
}

textparser_parser_state *textparser_state_generate(const textparser_t handle, size_t position)
{
    if (handle == nullptr)
        return nullptr;

    size_t size = textparser_get_total_units(handle);
    if (size >= MAX_PARSE_SIZE)
        return nullptr;

    size_t len = (position == 0) ? 0 : position;
    size_t allocated = len * sizeof(const textparser_token_item *);
    size_t to_allocate = offsetof(textparser_parser_state, state) + allocated;

    textparser_parser_state *ret = malloc(to_allocate);
    if (ret) {
        ret->len = (int)len;
        if (allocated > 0) {
            memset(ret->state, 0, allocated);
        }

        if (position > 0) {
            const textparser_token_item *active_token = find_token_at_position(handle->first_item, position - 1);
            if (active_token) {
                ret->state[position - 1] = active_token;
            }
        }
    }
    return ret;
}

