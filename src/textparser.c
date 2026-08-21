#include <textparser.h>
#include "adv_regex.h"
#include "logger.h"
#include "string_pool.h"
#include <os.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <ctype.h>

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
        exit_with_error(handle, "infinite loop due to 0-length token", offset);               \
    }

#define check_and_exit_on_fatal_parsing_error_start_stop(handle, child, offset)                \
    if ((handle)->error) {                                                                    \
        LOGW("Fatal error detected(%s) at offset %zu. exiting..", (handle)->error, offset);   \
        goto exit;                                                                            \
    }

static size_t calculate_chunk_size(size_t text_size);
static size_t textparser_skip_whitespace(const struct textparser_handle *handle, size_t pos);

enum parent_start_stop{
    TEXTPARSER_SEARCH_END_TOKEN,
    TEXTPARSER_SEARCH_START_TOKEN,
};

struct textparser_handle {
    const textparser_language_definition *language;
    adv_regex_context *regex_ctx;
    void *start_regex;
    void *end_regex;
    void *mmap_addr;
    size_t mmap_size;
    void *owned_buffer;
    size_t owned_buffer_capacity;
    enum textparser_bom bom;
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

static size_t textparser_get_byte_len(const struct textparser_handle *handle, size_t len)
{
    return textparser_get_byte_offset(handle, len);
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




static size_t calculate_chunk_size(size_t filesize)
{
    size_t min_chunk = 4096;      // 4KB minimum
    size_t max_chunk = 4194304;   // 4MB initial cap

    // Estimate initial arena size as 75% of file size
    size_t estimated = (filesize * 3) / 4;
    if (estimated < min_chunk) {
        return min_chunk;
    }

    size_t chunk_size = min_chunk;
    while (chunk_size < estimated && chunk_size < max_chunk) {
        chunk_size *= 2;
    }
    return chunk_size;
}

static void *textparser_convert_utf16be_to_native(const char *src, size_t size)
{
    size_t unit_count = size / sizeof(uint16_t);
    if (unit_count == 0)
        return nullptr;

    uint16_t *buf = (uint16_t *)malloc(size);
    if (buf == nullptr)
        return nullptr;

    const uint16_t *units = (const uint16_t *)src;
    for (size_t i = 0; i < unit_count; i++) {
        buf[i] = (uint16_t)((units[i] >> 8) | (units[i] << 8));
    }
    return buf;
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

typedef struct {
    size_t chunk_index;
    size_t chunk_used;
    size_t token_count;
} textparser_checkpoint_t;

static inline textparser_checkpoint_t textparser_checkpoint_save(const struct textparser_handle *handle)
{
    textparser_checkpoint_t cp;
    cp.chunk_index = handle->current_chunk_index;
    cp.chunk_used = handle->current_chunk_used;
    cp.token_count = handle->token_count;
    return cp;
}

static inline void textparser_checkpoint_restore(struct textparser_handle *handle, const textparser_checkpoint_t *cp)
{
    handle->current_chunk_index = cp->chunk_index;
    if (handle->chunks && cp->chunk_index < handle->chunk_count) {
        handle->current_chunk = handle->chunks[cp->chunk_index];
    } else {
        handle->current_chunk = nullptr;
    }
    handle->current_chunk_used = cp->chunk_used;
    handle->token_count = cp->token_count;
}

static textparser_token_item *textparser_alloc_token(struct textparser_handle *handle, int token_id, size_t len)
{
    size_t token_size = sizeof(textparser_token_item);
    if (handle->current_chunk == nullptr ||
        handle->current_chunk_used + token_size > handle->chunk_size)
    {
        if (handle->current_chunk != nullptr && handle->current_chunk_index + 1 < handle->chunk_count) {
            handle->current_chunk_index++;
            handle->current_chunk = handle->chunks[handle->current_chunk_index];
            handle->current_chunk_used = 0;
            memset(handle->current_chunk, 0, handle->chunk_size);
        } else {
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
    }

    textparser_token_item *ret = (textparser_token_item *)((char *)handle->current_chunk + handle->current_chunk_used);
    handle->current_chunk_used += token_size;
    memset(ret, 0, token_size);

    ret->token_id = token_id;
    ret->len = len;
    ret->text_color = TEXTPARSER_NOCOLOR;
    ret->text_background = TEXTPARSER_NOCOLOR;
    return ret;
}

static bool textparser_has_newline(const struct textparser_handle *handle, size_t pos, size_t len)
{
    if (handle == nullptr || len == 0) return false;

    size_t max_units = textparser_get_total_units(handle);
    if (pos >= max_units) return false;
    size_t end = (pos + len > max_units) ? max_units : pos + len;
    for (size_t c = pos; c < end; c++)
    {
        uint32_t ch = textparser_get_unit_at(handle, c);
        if (ch == '\n' || ch == '\r') return true;
    }

    return false;
}

static size_t textparser_skip_whitespace(const struct textparser_handle *handle, size_t pos)
{
    if (handle == nullptr) return pos;

    size_t max_units = textparser_get_total_units(handle);
    while (pos < max_units)
    {
        uint32_t ch = textparser_get_unit_at(handle, pos);
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
        {
            pos++;
        }
        else
        {
            break;
        }
    }

    return pos;
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


static bool textparser_token_in_id_list(const int *list, int token_id)
{
    if (list == nullptr) return false;
    for (int i = 0; list[i] != TextParser_END; i++) {
        if (list[i] == token_id) return true;
    }
    return false;
}

static textparser_token_item *textparser_get_last_child_item(textparser_token_item *item)
{
    if (item == nullptr) return nullptr;
    while (item->child != nullptr) {
        textparser_token_item *last = item->child;
        while (last->next != nullptr) {
            last = last->next;
        }
        item = last;
    }
    return item;
}

static size_t textparser_get_semantic_children_count(const textparser_token_item *token)
{
    if (token == nullptr) return 0;
    size_t ret = 0;
    const textparser_token_item *child = token->child;
    while (child) {
        if (child->token_id != TEXTPARSER_TOKEN_ID_UNPROCESSED) {
            ret++;
        }
        child = child->next;
    }
    return ret;
}

static void maybe_merge_sign(struct textparser_handle *handle, textparser_token_item *n)
{
    if (handle == nullptr || handle->language == nullptr || n == nullptr) return;
    const textparser_sign_merge *sign_merge = handle->language->sign_merge;
    if (sign_merge == nullptr || sign_merge->sign_tokens == nullptr || sign_merge->number_tokens == nullptr) return;

    if (!textparser_token_in_id_list(sign_merge->number_tokens, n->token_id)) return;

    textparser_token_item *prev = n->prev;
    if (prev == nullptr) return;

    textparser_token_item *sign = nullptr;
    textparser_token_item *context = nullptr;

    if (textparser_token_in_id_list(sign_merge->sign_tokens, prev->token_id)) {
        sign = prev;
        context = prev->prev;
    } else {
        textparser_token_item *last = textparser_get_last_child_item(prev);
        if (last != nullptr && textparser_token_in_id_list(sign_merge->sign_tokens, last->token_id)) {
            sign = last;
            context = (sign->prev != nullptr) ? sign->prev : prev->prev;
        } else {
            return;
        }
    }

    while (context && context->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED) {
        context = context->prev;
    }
    if (context == nullptr && prev != sign) {
        context = prev->prev;
        while (context && context->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED) {
            context = context->prev;
        }
    }

    // Only literal "+" and "-" are signs; never absorb other operators (e.g. "!3").
    if (sign->len != 1) return;

    // Unary context: the token before the sign must not be an operand.
    if (context != nullptr && textparser_token_in_id_list(sign_merge->operand_tokens, context->token_id)) return;

    size_t sign_pos = textparser_get_token_position(sign);
    uint32_t sign_ch = textparser_get_unit_at(handle, sign_pos);
    if (sign_ch != '+' && sign_ch != '-') return;

    // Absorb the sign into the number.
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
    // the container and unwrap it if a single semantic child remains.
    if (prev != sign) {
        if (prev->len >= sign->len) prev->len -= sign->len;
        if (textparser_get_semantic_children_count(prev) == 1) {
            textparser_token_item *first_child = prev->child;
            textparser_token_item *last_child = prev->child;
            for (textparser_token_item *it = first_child; it != nullptr; it = it->next) {
                it->parent = prev->parent;
                last_child = it;
            }
            first_child->prev = prev->prev;
            last_child->next = n;

            if (prev->prev != nullptr) {
                prev->prev->next = first_child;
            } else if (prev->parent != nullptr) {
                prev->parent->child = first_child;
            }
            n->prev = last_child;
        }
    }
}

static bool is_regex_valid_in_context(
    const struct textparser_handle *handle,
    const textparser_token_item *parent_item,
    const textparser_token_item *prev_sibling,
    size_t pos)
{
    (void)pos;
    if (handle == nullptr || handle->language == nullptr) return true;
    const textparser_regex_disambiguation *reg_div = handle->language->regex_disambiguation;
    if (reg_div == nullptr) return true;

    // Find the last non-unprocessed, non-comment token before pos
    const textparser_token_item *curr = prev_sibling;
    const textparser_token_item *prev = nullptr;
    while (curr) {
        if (curr->token_id != TEXTPARSER_TOKEN_ID_UNPROCESSED && curr->token_id >= 0) {
            const char *tok_name = handle->language->tokens[curr->token_id].name;
            if (tok_name && (strstr(tok_name, "Comment") != nullptr || strstr(tok_name, "comment") != nullptr)) {
                curr = curr->prev;
                continue;
            }
            prev = curr;
            break;
        }
        curr = curr->prev;
    }

    if (prev == nullptr && parent_item != nullptr) {
        // At start of container (e.g. inside `(...)` or `{...}` or `[...]`)
        return true;
    }

    if (prev == nullptr) {
        // At start of document
        return true;
    }

    // Check if prev token is in operand tokens
    if (textparser_token_in_id_list(reg_div->operand_tokens, prev->token_id)) {
        // Check special case: Parenthesis condition for control statements (if, while, for, switch, catch, with)
        const char *prev_name = handle->language->tokens[prev->token_id].name;
        if (prev_name && (strcasecmp(prev_name, "Parenthesis") == 0)) {
            // Find token preceding this parenthesis
            const textparser_token_item *before_paren = prev->prev;
            while (before_paren && before_paren->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED) {
                before_paren = before_paren->prev;
            }
            if (before_paren && before_paren->token_id >= 0 && reg_div->control_keywords != nullptr) {
                char *kw_text = textparser_get_token_text((textparser_t)handle, before_paren);
                if (kw_text) {
                    bool is_ctrl = false;
                    for (int k = 0; reg_div->control_keywords[k] != nullptr; k++) {
                        if (strcmp(kw_text, reg_div->control_keywords[k]) == 0) {
                            is_ctrl = true;
                            break;
                        }
                    }
                    textparser_free_token_text(kw_text);
                    if (is_ctrl) {
                        return true; // e.g. `if (x) /abc/` -> regex allowed
                    }
                }
            }
        }

        // If prev is an operand, `/` is division, not regex!
        return false;
    }

    // If prev is ++ or --, check if it was postfix (preceded by operand)
    if (prev->token_id >= 0) {
        char *prev_txt = textparser_get_token_text((textparser_t)handle, prev);
        if (prev_txt) {
            bool is_inc_dec = (strcmp(prev_txt, "++") == 0 || strcmp(prev_txt, "--") == 0);
            textparser_free_token_text(prev_txt);
            if (is_inc_dec) {
                const textparser_token_item *before_op = prev->prev;
                while (before_op && before_op->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED) {
                    before_op = before_op->prev;
                }
                if (before_op && before_op->token_id >= 0 && textparser_token_in_id_list(reg_div->operand_tokens, before_op->token_id)) {
                    return false; // postfix operand -> division
                }
            }
        }
    }

    return true;
}

static ssize_t textparser_find_token(const struct textparser_handle *handle, int token_id, size_t pos, bool other_text_inside, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    if (handle == nullptr || handle->recursion_depth >= MAX_RECURSION_DEPTH) {
        return TOKEN_NOT_FOUND;
    }

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

    ((struct textparser_handle *)handle)->recursion_depth++;
    ssize_t result = TOKEN_NOT_FOUND;

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
                    ssize_t closest_child_pos = SSIZE_MAX;
                    for (int c = 0; effective_nested[c] != TextParser_END; c++)
                    {
                        ssize_t child_token_pos = textparser_find_token(handle, effective_nested[c], pos, token->other_text_inside, parent_item, prev_sibling);
                        if (child_token_pos == TOKEN_NOT_FOUND) continue;
                        if (child_token_pos == 0) {
                            closest_child_pos = 0;
                            break;
                        }

                        if (child_token_pos < closest_child_pos) {
                            closest_child_pos = child_token_pos;
                        }
                    }

                    if (closest_child_pos < SSIZE_MAX) {
                        result = closest_child_pos;
                    }
                }
            }
            break;
        case TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER");
            {
                const int *effective_nested = get_effective_nested_tokens(handle, token_id, parent_item);
                if (effective_nested) {
                    result = textparser_find_token(handle, effective_nested[0], pos, other_text_inside, parent_item, prev_sibling);
                } else {
                    LOGE("nested_tokens = nullptr for TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER");
                }
            }
            break;
        case TEXTPARSER_TOKEN_TYPE_SEQUENCE:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_SEQUENCE");
            {
                const int *effective_nested = get_effective_nested_tokens(handle, token_id, parent_item);
                if (effective_nested && effective_nested[0] != TextParser_END) {
                    result = textparser_find_token(handle, effective_nested[0], pos, other_text_inside, parent_item, prev_sibling);
                } else {
                    LOGE("nested_tokens = nullptr for TEXTPARSER_TOKEN_TYPE_SEQUENCE");
                }
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
            if (adv_regex_find_pattern_ctx(handle->regex_ctx, token->start_regex, (void **)handle->start_regex + token_id, handle->text_format, text, len, &found_at, nullptr, !handle->language->case_sensitivity, true)) {
                LOGI("found_at token type: [%s] at %zu",  handle->language->tokens[token_id].name, pos + found_at);
                result = (ssize_t)found_at;
                if (result == 0 && handle->language && handle->language->regex_disambiguation) {
                    if (textparser_token_in_id_list(handle->language->regex_disambiguation->regex_tokens, token_id)) {
                        if (!is_regex_valid_in_context(handle, parent_item, prev_sibling, pos)) {
                            result = TOKEN_NOT_FOUND;
                        }
                    }
                }
            }
            break;
        default:
            LOGF("textparser_find_token() - unknown!!!!!");
            break;
    }

    ((struct textparser_handle *)handle)->recursion_depth--;
    return result;
}

static textparser_token_item *textparser_parse_token(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling);

static bool check_parent_token_boundary(
    struct textparser_handle *handle,
    int parent_token_id,
    int parent_start_stop,
    size_t offset)
{
    if (parent_token_id < 0) return false;
    const textparser_language_definition *definition = handle->language;
    const textparser_token *parent_def = &definition->tokens[parent_token_id];

    if (parent_start_stop == TEXTPARSER_SEARCH_END_TOKEN)
    {
        if (parent_def->end_regex != nullptr)
        {
            size_t token_end = 0;
            size_t end_len = 0;
            bool found_end = adv_regex_find_pattern_ctx(handle->regex_ctx, parent_def->end_regex, (void **)handle->end_regex + parent_token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, &token_end, &end_len, !handle->language->case_sensitivity, false);
            if (found_end && token_end == 0)
            {
                return true;
            }
        }
    }
    else if (parent_start_stop == TEXTPARSER_SEARCH_START_TOKEN)
    {
        if (parent_def->start_regex != nullptr)
        {
            size_t token_start = 0;
            size_t start_len = 0;
            bool found_start = adv_regex_find_pattern_ctx(handle->regex_ctx, parent_def->start_regex, (void **)handle->start_regex + parent_token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, &token_start, &start_len, !handle->language->case_sensitivity, false);
            if (found_start && token_start == 0)
            {
                return true;
            }
        }
    }

    return false;
}

static void append_child_to_ast(
    textparser_token_item *parent,
    textparser_token_item **head,
    textparser_token_item **tail,
    textparser_token_item *new_child)
{
    new_child->parent = parent;
    if (*tail != nullptr)
    {
        (*tail)->next = new_child;
        new_child->prev = *tail;
        *tail = new_child;
    }
    else
    {
        *head = new_child;
        *tail = new_child;
        new_child->prev = nullptr;
    }
}

static void append_unprocessed_if_needed(
    struct textparser_handle *handle,
    textparser_token_item *parent,
    textparser_token_item **head,
    textparser_token_item **tail,
    size_t len)
{
    if (len == 0) return;
    if (*tail != nullptr && (*tail)->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED)
    {
        (*tail)->len += len;
        return;
    }
    textparser_token_item *item = textparser_alloc_token(handle, TEXTPARSER_TOKEN_ID_UNPROCESSED, len);
    if (item == nullptr) return;
    append_child_to_ast(parent, head, tail, item);
}

static textparser_token_item *parse_token_group_one_child_only(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    (void)parent_item;
    textparser_token_item *ret = nullptr;

    if (handle == nullptr)
    {
        LOGF("handle == nullptr");
        return nullptr;
    }

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];
    textparser_token_item *child = nullptr;

    LOGV("enter TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY");
    const int *effective_nested = get_effective_nested_tokens(handle, token_id, parent_item);
    if (!effective_nested) {
        exit_with_error(handle, "group_one_child token type nested_tokens list is empty!", offset);
    }

    size_t start_offset = offset;

    ret = textparser_alloc_token(handle, token_id, 0);
    if (ret == nullptr) {
        return nullptr;
    }

    ret->parent = (textparser_token_item *)parent_item;
    ret->prev = (textparser_token_item *)prev_sibling;

    LOGV("id: %d - [%s]  at offset: %zu", token_id, token_def->name, offset);

    textparser_token_item *last_child = nullptr;
    for (int c = 0; effective_nested[c] != TextParser_END; c++)
    {
        int cand_id = effective_nested[c];
        ssize_t cand_pos = textparser_find_token(handle, cand_id, offset, token_def->other_text_inside, parent_item, prev_sibling);
        if (cand_pos == 0)
        {
            textparser_checkpoint_t cp = textparser_checkpoint_save(handle);
            const char *saved_err = handle->error;
            size_t saved_err_off = handle->error_offset;

            child = textparser_parse_token(handle, cand_id, parent_token_id, parent_start_stop, offset, ret, last_child);
            if (child != nullptr && handle->error == nullptr && child->len > 0) {
                break;
            }

            textparser_checkpoint_restore(handle, &cp);
            handle->error = saved_err;
            handle->error_offset = saved_err_off;
            child = nullptr;
        }
    }

    if (child == nullptr)
    {
        size_t closest = SIZE_MAX;
        int current_token_id = TextParser_END;
        for (int c = 0; effective_nested[c] != TextParser_END; c++)
        {
            ssize_t current_closest = textparser_find_token(handle, effective_nested[c], offset, token_def->other_text_inside, parent_item, prev_sibling);
            if ((current_closest > 0) && ((size_t)current_closest < closest))
            {
                closest = (size_t)current_closest;
                current_token_id = effective_nested[c];
            }
        }

        if (current_token_id == TextParser_END)
        {
            exit_with_error(handle, "Search for group_one_child token type failed. Can't find one child.", offset);
        }

        if (closest > 0) {
            append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, closest);
            offset += closest;
        }

        child = textparser_parse_token(handle, current_token_id, parent_token_id, parent_start_stop, offset, ret, last_child);
        if (child == nullptr) {
            exit_with_error(handle, "Search for group_one_child token type failed. Child token parsing failed.", offset);
        }
    }

    append_child_to_ast(ret, &ret->child, &last_child, child);
    LOGV("TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY - Found [%s]", handle->language->tokens[child->token_id].name);
    offset += child->len;
    ret->len = offset - start_offset;
    check_and_exit_on_fatal_parsing_error(handle, child, offset);

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

static textparser_token_item *parse_token_group(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    textparser_token_item *ret = nullptr;

    if (handle == nullptr)
    {
        LOGF("handle == nullptr");
        return nullptr;
    }

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];
    textparser_token_item *child = nullptr;

    LOGV("enter TEXTPARSER_TOKEN_TYPE_GROUP");
    const int *effective_nested = get_effective_nested_tokens(handle, token_id, parent_item);
    if (!effective_nested) {
        exit_with_error(handle, "nested_tokens list is empty!", offset);
    }

    size_t start_offset = offset;

    ret = textparser_alloc_token(handle, token_id, 0);
    if (ret == nullptr) {
        return nullptr;
    }

    ret->parent = (textparser_token_item *)parent_item;
    ret->prev = (textparser_token_item *)prev_sibling;

    LOGV("id: %d - [%s]  at offset: %zu", token_id, token_def->name, offset);
    while(1) {
        size_t ws_skipped = textparser_skip_whitespace(handle, offset) - offset;
        if (ws_skipped > 0) {
            append_unprocessed_if_needed(handle, ret, &ret->child, &child, ws_skipped);
            offset += ws_skipped;
        }

        if (offset >= textparser_get_total_units(handle))
        {
            if (child)
            {
                ret->len = offset - start_offset;
                goto exit;
            }
            exit_with_error(handle, "Search for group token type failed. Can't find any child.", offset);
        }

        const textparser_token_item *current_prev = child;

        if (check_parent_token_boundary(handle, parent_token_id, parent_start_stop, offset)) {
            ret->len = offset - start_offset;
            break;
        }

        const int *loop_effective_nested = get_effective_nested_tokens(handle, token_id, ret);
        textparser_token_item *new_child = nullptr;
        textparser_token_item *error_child = nullptr;
        if (loop_effective_nested) {
            for (int c = 0; loop_effective_nested[c] != TextParser_END; c++) {
                int cand_id = loop_effective_nested[c];
                ssize_t found = textparser_find_token(handle, cand_id, offset, token_def->other_text_inside, ret, current_prev);
                if (found == 0) {
                    textparser_checkpoint_t cp = textparser_checkpoint_save(handle);
                    const char *saved_err = handle->error;
                    size_t saved_err_off = handle->error_offset;

                    textparser_token_item *attempt = textparser_parse_token(handle, cand_id, parent_token_id, parent_start_stop, offset, ret, current_prev);
                    if (attempt != nullptr && handle->error == nullptr && attempt->len > 0) {
                        new_child = attempt;
                        break;
                    }

                    if (attempt != nullptr && handle->error != nullptr && error_child == nullptr) {
                        error_child = attempt;
                    } else {
                        textparser_checkpoint_restore(handle, &cp);
                        handle->error = saved_err;
                        handle->error_offset = saved_err_off;
                    }
                }
            }
        }

        if (new_child == nullptr && error_child != nullptr) {
            new_child = error_child;
        }

        if (new_child != nullptr)
        {
            if (handle->error) {
                if (token_def->other_text_inside && offset < textparser_get_total_units(handle)) {
                    handle->error = nullptr;
                    handle->error_offset = 0;
                    size_t char_l = textparser_char_len(handle, offset);
                    append_unprocessed_if_needed(handle, ret, &ret->child, &child, char_l);
                    offset += char_l;
                    continue;
                }
                goto exit;
            }

            size_t child_advance = new_child->len;
            append_child_to_ast(ret, &ret->child, &child, new_child);
            maybe_merge_sign(handle, child);

            if (child->len == 0) {
                exit_with_error(handle, "0-length child token match caused infinite loop", offset);
            }

            offset += child_advance;
            ret->len = offset - start_offset;
        }
        else
        {
            if (token_def->other_text_inside && offset < textparser_get_total_units(handle))
            {
                size_t char_l = textparser_char_len(handle, offset);
                append_unprocessed_if_needed(handle, ret, &ret->child, &child, char_l);
                offset += char_l;
            }
            else
            {
                if (child)
                {
                    ret->len = offset - start_offset;
                    goto exit;
                }
                exit_with_error(handle, "Unrecognized token inside group", offset);
            }
        }
    }

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

static textparser_token_item *parse_token_group_all_children_in_same_order(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    textparser_token_item *ret = nullptr;

    if (handle == nullptr)
    {
        LOGF("handle == nullptr");
        return nullptr;
    }

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];
    textparser_token_item *child = nullptr;
    textparser_token_item *last_child = nullptr;

    LOGV("enter TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER");
    if (!token_def->nested_tokens) {
        exit_with_error(handle, "nested_tokens list is empty!", offset);
    }

    int nested_count = 0;
    while(token_def->nested_tokens[nested_count] != TextParser_END) nested_count++;

    if (nested_count != 3) {
         exit_with_error(handle, "GroupAllChildrenInSameOrder should have exactly 3 nested tokens", offset);
    }

    int start_token_id = token_def->nested_tokens[0];
    int inner_token_id = token_def->nested_tokens[1];
    int end_token_id   = token_def->nested_tokens[2];

    size_t start_offset = offset;

    ret = textparser_alloc_token(handle, token_id, 0);
    if (ret == nullptr) {
        return nullptr;
    }

    ret->parent = (textparser_token_item *)parent_item;
    ret->prev = (textparser_token_item *)prev_sibling;

    ssize_t start_pos = textparser_find_token(handle, start_token_id, offset, definition->other_text_inside, ret, prev_sibling);
    if (start_pos != 0) {
        exit_with_error(handle, "Expected start token!", offset);
    }

    child = textparser_parse_token(handle, start_token_id, parent_token_id, parent_start_stop, offset, ret, prev_sibling);
    if (child == nullptr) {
        exit_with_error(handle, "Parsing start token failed", offset);
    }
    append_child_to_ast(ret, &ret->child, &last_child, child);
    maybe_merge_sign(handle, child);
    check_and_exit_on_fatal_parsing_error(handle, child, offset);

    offset += child->len;

    while(1)
    {
        size_t ws_skipped = textparser_skip_whitespace(handle, offset) - offset;
        if (ws_skipped > 0) {
            append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, ws_skipped);
            offset += ws_skipped;
        }

        if (offset >= textparser_get_total_units(handle)) {
            exit_with_error(handle, "Expected end token, reached end of text!", offset);
        }

        ssize_t end_pos   = textparser_find_token(handle, end_token_id,   offset, definition->other_text_inside, ret, last_child);
        if (end_pos == 0) {
            break;
        }

        ssize_t inner_pos = textparser_find_token(handle, inner_token_id, offset, definition->other_text_inside, ret, last_child);
        if (inner_pos == 0) {
            child = textparser_parse_token(handle, inner_token_id, end_token_id, TEXTPARSER_SEARCH_START_TOKEN, offset, ret, last_child);
            if (child == nullptr) {
                exit_with_error(handle, "Parsing inner token failed", offset);
            }
            size_t child_advance = child->len;
            append_child_to_ast(ret, &ret->child, &last_child, child);
            maybe_merge_sign(handle, child);
            check_and_exit_on_fatal_parsing_error(handle, child, offset);

            if (child->len == 0) {
                exit_with_error(handle, "0-length child token match caused infinite loop", offset);
            }

            offset += child_advance;
            continue;
        }

        if (definition->other_text_inside) {
            size_t char_l = textparser_char_len(handle, offset);
            append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, char_l);
            offset += char_l;
        } else {
            exit_with_error(handle, "Expected inner or end token!", offset);
        }
    }

    size_t ws_skipped_end = textparser_skip_whitespace(handle, offset) - offset;
    if (ws_skipped_end > 0) {
        append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, ws_skipped_end);
        offset += ws_skipped_end;
    }

    child = textparser_parse_token(handle, end_token_id, parent_token_id, parent_start_stop, offset, ret, last_child);
    if (child == nullptr) {
        exit_with_error(handle, "Parsing end token failed", offset);
    }
    append_child_to_ast(ret, &ret->child, &last_child, child);
    maybe_merge_sign(handle, child);
    check_and_exit_on_fatal_parsing_error(handle, child, offset);

    offset += child->len;
    ret->len = offset - start_offset;

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

static textparser_token_item *parse_token_sequence(
    struct textparser_handle *handle,
    int token_id,
    int parent_token_id,
    int parent_start_stop,
    size_t offset,
    const textparser_token_item *parent_item,
    const textparser_token_item *prev_sibling)
{
    (void)parent_token_id;
    (void)parent_start_stop;
    if (handle == nullptr) return nullptr;

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];
    const int *nested = get_effective_nested_tokens(handle, token_id, parent_item);
    if (!nested || nested[0] == TextParser_END) {
        return nullptr;
    }

    textparser_checkpoint_t cp = textparser_checkpoint_save(handle);
    const char *saved_error = handle->error;
    size_t saved_error_offset = handle->error_offset;

    size_t start_offset = offset;
    textparser_token_item *ret = textparser_alloc_token(handle, token_id, 0);
    if (!ret) {
        return nullptr;
    }
    ret->parent = (textparser_token_item *)parent_item;
    ret->prev = (textparser_token_item *)prev_sibling;
    textparser_token_item *last_child = nullptr;

    for (int i = 0; nested[i] != TextParser_END; i++) {
        int elem_id = nested[i];

        size_t ws_skipped = textparser_skip_whitespace(handle, offset) - offset;
        if (ws_skipped > 0) {
            append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, ws_skipped);
            offset += ws_skipped;
        }

        if (offset >= textparser_get_total_units(handle)) {
            textparser_checkpoint_restore(handle, &cp);
            handle->error = saved_error;
            handle->error_offset = saved_error_offset;
            return nullptr;
        }

        ssize_t found = textparser_find_token(handle, elem_id, offset, token_def->other_text_inside, ret, last_child);
        if (found != 0) {
            textparser_checkpoint_restore(handle, &cp);
            handle->error = saved_error;
            handle->error_offset = saved_error_offset;
            return nullptr;
        }

        textparser_token_item *elem_item = textparser_parse_token(handle, elem_id, token_id, TEXTPARSER_SEARCH_START_TOKEN, offset, ret, last_child);
        if (elem_item == nullptr || handle->error != nullptr || elem_item->len == 0) {
            textparser_checkpoint_restore(handle, &cp);
            handle->error = saved_error;
            handle->error_offset = saved_error_offset;
            return nullptr;
        }

        append_child_to_ast(ret, &ret->child, &last_child, elem_item);
        maybe_merge_sign(handle, elem_item);
        offset += elem_item->len;
    }

    ret->len = offset - start_offset;
    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }
    return ret;
}

static textparser_token_item *parse_token_simple_token(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    (void)parent_item;
    (void)prev_sibling;
    (void)parent_start_stop;
    (void)parent_token_id;
    textparser_token_item *ret = nullptr;

    if (handle == nullptr)
    {
        LOGF("handle == nullptr");
        return nullptr;
    }

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];

    LOGV("enter TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN");

    if (offset >= textparser_get_total_units(handle)) {
        exit_with_error(handle, "offset >= total units count!", offset);
    }

    ret = textparser_alloc_token(handle, token_id, 0);
    if (ret == nullptr) {
        exit_with_error(handle, "Can't allocate memory!", offset);
    }

    size_t len = 0;
    if (!adv_regex_find_pattern_ctx(handle->regex_ctx, token_def->start_regex, (void **)handle->start_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &len, !handle->language->case_sensitivity, true)) {
        exit_with_error(handle, "Can't find start of the token!", offset);
    }

    LOGV("TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN - Found [%s]", handle->language->tokens[ret->token_id].name);
    ret->len = len;
    ret->parent = (textparser_token_item *)parent_item;
    ret->prev = (textparser_token_item *)prev_sibling;

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

static textparser_token_item *parse_token_start_stop(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, bool stop_required, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    (void)prev_sibling;
    (void)parent_start_stop;
    (void)parent_token_id;
    textparser_token_item *ret = nullptr;
    textparser_token_item *child = nullptr;

    size_t len = 0;
    size_t token_end = 0;

    if (handle == nullptr) {
        exit_with_error(handle, "handle == nullptr!", offset);
    }

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];

    if (stop_required) {
        LOGV("enter TEXTPARSER_TOKEN_TYPE_START_STOP");
    } else {
        LOGV("enter TEXTPARSER_TOKEN_TYPE_START_OPT_STOP");
    }

    if (offset >= textparser_get_total_units(handle)) {
        exit_with_error(handle, "offset >= total units count!", offset);
    }

    size_t start_offset = offset;

    ret = textparser_alloc_token(handle, token_id, 0);
    if (ret == nullptr) {
        exit_with_error(handle, "Can't allocate memory!", offset);
    }

    ret->parent = (textparser_token_item *)parent_item;
    ret->prev = (textparser_token_item *)prev_sibling;

    // Search for start token
    if (!adv_regex_find_pattern_ctx(handle->regex_ctx, token_def->start_regex, (void **)handle->start_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &len, !handle->language->case_sensitivity, true)) {
        exit_with_error(handle, "Can't find start of the token!", offset);
    }

    textparser_token_item *last_child = nullptr;
    append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, len);
    offset += len;

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_START, handle->user_data);
    }

    if (offset > textparser_get_total_units(handle)) {
        exit_with_error(handle, "offset >= total units count!", offset);
    }

    if (offset == textparser_get_total_units(handle)) {
        if (token_def->type == TEXTPARSER_TOKEN_TYPE_START_STOP) {
            exit_with_error(handle, "reached end of text!", offset);
        } else {
            ret->len = offset - start_offset;
            goto exit;
        }
    }

    const int *effective_nested = get_effective_nested_tokens(handle, token_id, ret);
    if (effective_nested)
    {
        const int *nested_tokens = effective_nested;

        while (1) {
            size_t ws_skipped = textparser_skip_whitespace(handle, offset) - offset;
            if (ws_skipped > 0) {
                append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, ws_skipped);
                offset += ws_skipped;
            }

            if (token_def->search_parent_end_token_last == false)
            {
                size_t end_match_len = 0;
                bool found_end = adv_regex_find_pattern_ctx(handle->regex_ctx, token_def->end_regex, (void **)handle->end_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &end_match_len, !handle->language->case_sensitivity, true);
                if (found_end)
                {
                    break;
                }
            }

            if (offset >= textparser_get_total_units(handle))
            {
                if (token_def->search_parent_end_token_last == true)
                {
                    size_t end_match_len = 0;
                    bool found_end = adv_regex_find_pattern_ctx(handle->regex_ctx, token_def->end_regex, (void **)handle->end_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &end_match_len, !handle->language->case_sensitivity, true);
                    if (found_end)
                    {
                        break;
                    }
                }
                if (token_def->type == TEXTPARSER_TOKEN_TYPE_START_STOP) {
                    exit_with_error(handle, "Reached end of text before finding end token!", offset);
                } else {
                    break;
                }
            }

            const textparser_token_item *current_prev = last_child;
            textparser_token_item *new_child = nullptr;
            textparser_token_item *error_child = nullptr;
            if (nested_tokens) {
                for (int c = 0; nested_tokens[c] != TextParser_END; c++) {
                    int cand_id = nested_tokens[c];
                    ssize_t found = textparser_find_token(handle, cand_id, offset, token_def->other_text_inside, ret, current_prev);
                    if (found == 0) {
                        textparser_checkpoint_t cp = textparser_checkpoint_save(handle);
                        const char *saved_err = handle->error;
                        size_t saved_err_off = handle->error_offset;

                        textparser_token_item *attempt = textparser_parse_token(handle, cand_id, token_id, TEXTPARSER_SEARCH_END_TOKEN, offset, ret, current_prev);
                        if (attempt != nullptr && handle->error == nullptr && attempt->len > 0) {
                            new_child = attempt;
                            break;
                        }

                        if (attempt != nullptr && handle->error != nullptr && error_child == nullptr) {
                            error_child = attempt;
                        } else {
                            textparser_checkpoint_restore(handle, &cp);
                            handle->error = saved_err;
                            handle->error_offset = saved_err_off;
                        }
                    }
                }
            }

            if (new_child == nullptr && error_child != nullptr) {
                new_child = error_child;
            }

            if (new_child != nullptr)
            {
                if (handle->error) {
                    if (token_def->other_text_inside && offset < textparser_get_total_units(handle)) {
                        handle->error = nullptr;
                        handle->error_offset = 0;
                        size_t char_l = textparser_char_len(handle, offset);
                        append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, char_l);
                        offset += char_l;
                        continue;
                    }
                    goto exit;
                }

                child = new_child;
                size_t child_advance = new_child->len;
                append_child_to_ast(ret, &ret->child, &last_child, child);
                maybe_merge_sign(handle, child);

                if (child->len == 0) {
                    exit_with_error(handle, "0-length child token match caused infinite loop", offset);
                }

                offset += child_advance;
                check_and_exit_on_fatal_parsing_error(handle, child, offset);
                continue;
            }

            if (token_def->search_parent_end_token_last == true)
            {
                size_t end_match_len = 0;
                bool found_end = adv_regex_find_pattern_ctx(handle->regex_ctx, token_def->end_regex, (void **)handle->end_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &end_match_len, !handle->language->case_sensitivity, true);
                if (found_end)
                {
                    break;
                }
            }

            if (token_def->other_text_inside) {
                size_t char_l = textparser_char_len(handle, offset);
                append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, char_l);
                offset += char_l;
            } else {
                exit_with_error(handle, "Unexpected token inside start-stop block!", offset);
            }
        }
    }

    size_t ws_skipped_final = textparser_skip_whitespace(handle, offset) - offset;
    if (ws_skipped_final > 0) {
        append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, ws_skipped_final);
        offset += ws_skipped_final;
    }

    size_t end_len = 0;
    bool found_end = adv_regex_find_pattern_ctx(handle->regex_ctx, token_def->end_regex, (void **)handle->end_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, &token_end, &end_len, !handle->language->case_sensitivity, false);

    if (!found_end) {
        if (token_def->type == TEXTPARSER_TOKEN_TYPE_START_STOP) {
            LOGE("Can't find [%s] at %zu. Text: [%s]", token_def->end_regex, offset, handle->text_addr + textparser_get_byte_offset(handle, offset));
            exit_with_error(handle, "Can't find end of the token!", offset);
        } else {
            ret->len = offset - start_offset;
            goto exit;
        }
    }

    LOGV("TEXTPARSER_TOKEN_TYPE_START_(OPT)_STOP - Found [%s]", handle->language->tokens[ret->token_id].name);
    append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, token_end + end_len);
    offset += token_end + end_len;
    ret->len = offset - start_offset;

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

static textparser_token_item *parse_token_error_error(struct textparser_handle *handle, const char *msg, size_t offset)
{
    exit_with_error(handle, msg, offset);
exit:
    return nullptr;
}

static textparser_token_item *textparser_parse_token(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    if (handle == nullptr) {
        LOGF("handle == nullptr");
        return nullptr;
    }

    if (handle->recursion_depth >= MAX_RECURSION_DEPTH) {
        exit_with_error(handle, "Maximum recursion depth exceeded!", offset);
    }
    handle->recursion_depth++;

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];

    LOGV("id: %d - [%s]  at offset: %zu", token_id, token_def->name, offset);
    textparser_token_item *ret = nullptr;
    switch(token_def->type)
    {
        case TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY:             ret = parse_token_group_one_child_only(handle, token_id, parent_token_id, parent_start_stop, offset, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_GROUP:                            ret = parse_token_group(handle, token_id, parent_token_id, parent_start_stop, offset, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER: ret = parse_token_group_all_children_in_same_order(handle, token_id, parent_token_id, parent_start_stop, offset, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_SEQUENCE:                         ret = parse_token_sequence(handle, token_id, parent_token_id, parent_start_stop, offset, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN:                     ret = parse_token_simple_token(handle, token_id, parent_token_id, parent_start_stop, offset, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_START_STOP:                       ret = parse_token_start_stop(handle, token_id, parent_token_id, parent_start_stop, offset, true, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_START_OPT_STOP:                   ret = parse_token_start_stop(handle, token_id, parent_token_id, parent_start_stop, offset, false, parent_item, prev_sibling); break;
        default:
            parse_token_error_error(handle, "Unknown token type!", offset);
            break;
    }

    if (ret) {
        ret->text_color = token_def->text_color;
        ret->text_background = token_def->text_background;
        ret->text_flags = token_def->text_flags;

        if (!token_def->multi_line && textparser_has_newline(handle, offset, ret->len)) {
            exit_with_error(handle, "Token spans multiple lines but multi_line flag is not set!", offset);
        }

        if (token_def->must_have_one_child && textparser_get_semantic_children_count(ret) != 1) {
            exit_with_error(handle, "Token must have exactly one child token!", offset);
        }

        if ((token_def->type == TEXTPARSER_TOKEN_TYPE_GROUP || token_def->type == TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY) &&
            token_def->delete_if_only_one_child && textparser_get_semantic_children_count(ret) == 1) {
            textparser_token_item *first_child = ret->child;
            textparser_token_item *last_child = ret->child;
            while (last_child->next) {
                last_child->parent = ret->parent;
                last_child = last_child->next;
            }
            last_child->parent = ret->parent;
            ret = first_child;
        }
    }

exit:
    handle->recursion_depth--;
    return ret;
}

static int textparser_init_regex(struct textparser_handle *handle)
{
    if (handle == nullptr)
        return -1;

    if (handle->regex_ctx == nullptr) {
        handle->regex_ctx = adv_regex_context_create();
    }
    int token_cnt = 0;

    while(handle->language->tokens[token_cnt].name != nullptr)
        token_cnt++;

    handle->token_count = (size_t)token_cnt;

    if (token_cnt > 0)
    {
        size_t malloc_size = (size_t)token_cnt * sizeof(void *);

        handle->start_regex = malloc(malloc_size);
        if (handle->start_regex == nullptr) {
            LOGE("malloc() failed for start_regex");
            handle->error = "Can't allocate memory!";
            return -1;
        }
        memset(handle->start_regex, 0, malloc_size);

        handle->end_regex = malloc(malloc_size);
        if (handle->end_regex == nullptr) {
            LOGE("malloc() failed for end_regex");
            free(handle->start_regex);
            handle->start_regex = nullptr;
            handle->error = "Can't allocate memory!";
            return -1;
        }
        memset(handle->end_regex, 0, malloc_size);
    }
    return 0;
}

static void textparser_free_regex(struct textparser_handle *handle)
{
    if (handle == nullptr)
        return;

    // Always free regex_ctx, even if regexes were never compiled
    if (handle->regex_ctx) {
        adv_regex_context_free(handle->regex_ctx);
        handle->regex_ctx = nullptr;
    }

    if ((handle->start_regex == nullptr) && (handle->end_regex == nullptr))
        return;

    enum textparser_encoding text_format = handle->text_format;
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

void textparser_free_language_definition(textparser_language_definition *definition)
{
    if (definition == nullptr)
        return;

    bool uses_pool = (definition->string_pool != nullptr);

    if (definition->default_file_extensions) {
        if (!uses_pool) {
            int c = 0;
            while(definition->default_file_extensions[c]) {
                free((void *)definition->default_file_extensions[c]);
                c++;
            }
        }
        free((void *)definition->default_file_extensions);
    }

    if (!uses_pool) {
        if (definition->name) {
            free((void *)definition->name);
        }

        if (definition->empty_segment_language) {
            free((void *)definition->empty_segment_language);
        }
    }

    if (definition->starts_with) {
        free((void *)definition->starts_with);
    }

    if (definition->sign_merge) {
        if (definition->sign_merge->sign_tokens) {
            free((void *)definition->sign_merge->sign_tokens);
        }
        if (definition->sign_merge->number_tokens) {
            free((void *)definition->sign_merge->number_tokens);
        }
        if (definition->sign_merge->operand_tokens) {
            free((void *)definition->sign_merge->operand_tokens);
        }
        free((void *)definition->sign_merge);
    }

    if (definition->operator_precedence) {
        if (definition->operator_precedence->rules) {
            for (size_t r = 0; r < definition->operator_precedence->count; r++) {
                if (definition->operator_precedence->rules[r].operators) {
                    free((void *)definition->operator_precedence->rules[r].operators);
                }
            }
            free((void *)definition->operator_precedence->rules);
        }
        free(definition->operator_precedence);
    }

    if (definition->regex_disambiguation) {
        if (definition->regex_disambiguation->regex_tokens) free((void *)definition->regex_disambiguation->regex_tokens);
        if (definition->regex_disambiguation->division_tokens) free((void *)definition->regex_disambiguation->division_tokens);
        if (definition->regex_disambiguation->operand_tokens) free((void *)definition->regex_disambiguation->operand_tokens);
        if (definition->regex_disambiguation->control_keywords) {
            if (!uses_pool) {
                for (int k = 0; definition->regex_disambiguation->control_keywords[k] != nullptr; k++) {
                    free((void *)definition->regex_disambiguation->control_keywords[k]);
                }
            }
            free((void *)definition->regex_disambiguation->control_keywords);
        }
        free(definition->regex_disambiguation);
    }

    if (definition->template_disambiguation) {
        if (definition->template_disambiguation->template_open_tokens) free((void *)definition->template_disambiguation->template_open_tokens);
        if (definition->template_disambiguation->template_close_tokens) free((void *)definition->template_disambiguation->template_close_tokens);
        if (definition->template_disambiguation->valid_inner_tokens) free((void *)definition->template_disambiguation->valid_inner_tokens);
        if (definition->template_disambiguation->invalid_inner_operators) {
            if (!uses_pool) {
                for (int k = 0; definition->template_disambiguation->invalid_inner_operators[k] != nullptr; k++) {
                    free((void *)definition->template_disambiguation->invalid_inner_operators[k]);
                }
            }
            free((void *)definition->template_disambiguation->invalid_inner_operators);
        }
        free(definition->template_disambiguation);
    }

    if (definition->cast_disambiguation) {
        if (definition->cast_disambiguation->type_tokens) free((void *)definition->cast_disambiguation->type_tokens);
        if (definition->cast_disambiguation->type_keywords) {
            if (!uses_pool) {
                for (int k = 0; definition->cast_disambiguation->type_keywords[k] != nullptr; k++) {
                    free((void *)definition->cast_disambiguation->type_keywords[k]);
                }
            }
            free((void *)definition->cast_disambiguation->type_keywords);
        }
        if (definition->cast_disambiguation->type_suffixes) {
            if (!uses_pool) {
                for (int k = 0; definition->cast_disambiguation->type_suffixes[k] != nullptr; k++) {
                    free((void *)definition->cast_disambiguation->type_suffixes[k]);
                }
            }
            free((void *)definition->cast_disambiguation->type_suffixes);
        }
        free(definition->cast_disambiguation);
    }

    if (definition->override_start_tokens) {
        for (int r = 0; definition->override_start_tokens[r].file_extensions != nullptr ||
                        definition->override_start_tokens[r].regex != nullptr ||
                        definition->override_start_tokens[r].start_tokens != nullptr; r++) {
            if (definition->override_start_tokens[r].file_extensions) {
                if (!uses_pool) {
                    for (int e = 0; definition->override_start_tokens[r].file_extensions[e] != nullptr; e++) {
                        free((void *)definition->override_start_tokens[r].file_extensions[e]);
                    }
                }
                free((void *)definition->override_start_tokens[r].file_extensions);
            }
            if (!uses_pool && definition->override_start_tokens[r].regex) {
                free((void *)definition->override_start_tokens[r].regex);
            }
            if (definition->override_start_tokens[r].start_tokens) {
                free((void *)definition->override_start_tokens[r].start_tokens);
            }
        }
        free((void *)definition->override_start_tokens);
    }

    if (definition->tokens) {
        int c = 0;
        while(definition->tokens[c].name != nullptr) {
            textparser_token *token = &definition->tokens[c];

            if (!uses_pool) {
                if (token->name) {
                    free((void *)token->name);
                }
                if (token->start_regex) {
                    free((void *)token->start_regex);
                }
                if (token->end_regex) {
                    free((void *)token->end_regex);
                }
            }
            if (token->nested_tokens) {
                free((void *)token->nested_tokens);
            }
            if (token->context_nested_tokens) {
                for (int r = 0; token->context_nested_tokens[r].when_parent_in != nullptr; r++) {
                    if (token->context_nested_tokens[r].when_parent_in) {
                        free((void *)token->context_nested_tokens[r].when_parent_in);
                    }
                    if (token->context_nested_tokens[r].nested_tokens) {
                        free((void *)token->context_nested_tokens[r].nested_tokens);
                    }
                }
                free((void *)token->context_nested_tokens);
            }

            c++;
        }
        free(definition->tokens);
    }

    if (uses_pool) {
        /* Free the continuous string pool arena */
        textparser_string_pool_free((textparser_string_pool *)definition->string_pool);
    }

    free(definition);
}

int textparser_openfile(const char *pathname, int default_text_format, int bom_mask, textparser_t *handle)
{
    struct textparser_handle local_hnd;
    int err = TEXTPARSER_OK;

    memset(&local_hnd, 0, sizeof(local_hnd));
    if (pathname) {
        local_hnd.filename = strdup(pathname);
        if (local_hnd.filename == nullptr) {
            err = TEXTPARSER_ERROR_OUT_OF_MEMORY;
            goto err;
        }
    }

    local_hnd.mmap_addr = os_map(pathname, &local_hnd.mmap_size);
    if (!local_hnd.mmap_addr && local_hnd.mmap_size != 0) {
        err = TEXTPARSER_ERROR_FILE_OPEN;
        goto err;
    }

    local_hnd.text_addr = local_hnd.mmap_addr;
    local_hnd.text_size = local_hnd.mmap_size;

    if (local_hnd.text_size >= MAX_PARSE_SIZE) {
        err = TEXTPARSER_ERROR_FILE_TOO_LARGE;
        goto err;
    }

    if ((bom_mask & TEXTPARSER_BOM_UTF_32_BE)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x00')&&(local_hnd.text_addr[1] == '\x00')&&(local_hnd.text_addr[2] == '\xfe')&&(local_hnd.text_addr[3] == '\xff')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = TEXTPARSER_BOM_UTF_32_BE;
    } else if ((bom_mask & TEXTPARSER_BOM_UTF_32_LE)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\xff')&&(local_hnd.text_addr[1] == '\xfe')&&(local_hnd.text_addr[2] == '\x00')&&(local_hnd.text_addr[3] == '\x00')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = TEXTPARSER_BOM_UTF_32_LE;
    } else if ((bom_mask & TEXTPARSER_BOM_UTF_8)&&(local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\xef')&&(local_hnd.text_addr[1] == '\xbb')&&(local_hnd.text_addr[2] == '\xbf')) {
        local_hnd.text_addr += 3;
        local_hnd.text_size -= 3;
        local_hnd.bom = TEXTPARSER_BOM_UTF_8;
    } else if ((bom_mask & TEXTPARSER_BOM_UTF_16_BE)&&(local_hnd.text_size >= 2)&&(local_hnd.text_addr[0] == '\xfe')&&(local_hnd.text_addr[1] == '\xff')) {
        local_hnd.text_addr += 2;
        local_hnd.text_size -= 2;
        local_hnd.bom = TEXTPARSER_BOM_UTF_16_BE;
    } else if ((bom_mask & TEXTPARSER_BOM_UTF_16_LE)&&(local_hnd.text_size >= 2)&&(local_hnd.text_addr[0] == '\xff')&&(local_hnd.text_addr[1] == '\xfe')) {
        local_hnd.text_addr += 2;
        local_hnd.text_size -= 2;
        local_hnd.bom = TEXTPARSER_BOM_UTF_16_LE;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_7_1)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x38')) {
    //     local_hnd.text_addr += 4;
    //     local_hnd.text_size -= 4;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_7_1;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_7_2)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x39')) {
    //     local_hnd.text_addr += 4;
    //     local_hnd.text_size -= 4;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_7_2;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_7_3)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x2b')) {
    //     local_hnd.text_addr += 4;
    //     local_hnd.text_size -= 4;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_7_3;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_7_4)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x2f')) {
    //     local_hnd.text_addr += 4;
    //     local_hnd.text_size -= 4;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_7_4;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_7_5)&&(local_hnd.text_size >= 5)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x38')&&(local_hnd.text_addr[4] == '\x2d')) {
    //     local_hnd.text_addr += 5;
    //     local_hnd.text_size -= 5;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_7_5;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_1)&&(local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\xf7')&&(local_hnd.text_addr[1] == '\x64')&&(local_hnd.text_addr[2] == '\x4c')) {
    //     local_hnd.text_addr += 3;
    //     local_hnd.text_size -= 3;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_1;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_EBCDIC)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\xdd')&&(local_hnd.text_addr[1] == '\x73')&&(local_hnd.text_addr[2] == '\x66')&&(local_hnd.text_addr[3] == '\x73')) {
    //     local_hnd.text_addr += 4;
    //     local_hnd.text_size -= 4;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_EBCDIC;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_SCSU)&&(local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\x0e')&&(local_hnd.text_addr[1] == '\xfe')&&(local_hnd.text_addr[2] == '\xff')) {
    //     local_hnd.text_addr += 3;
    //     local_hnd.text_size -= 3;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_SCSU;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_BOCU1)&&(local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\xfb')&&(local_hnd.text_addr[1] == '\xee')&&(local_hnd.text_addr[2] == '\x28')) {
    //     local_hnd.text_addr += 3;
    //     local_hnd.text_size -= 3;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_BOCU1;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_GB_18030)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x84')&&(local_hnd.text_addr[1] == '\x31')&&(local_hnd.text_addr[2] == '\x95')&&(local_hnd.text_addr[3] == '\x33')) {
    //     local_hnd.text_addr += 4;
    //     local_hnd.text_size -= 4;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_GB_18030;
    } else {
        local_hnd.bom = TEXTPARSER_BOM_NONE;
    }

    local_hnd.text_format = (enum textparser_encoding)default_text_format;

    switch(local_hnd.bom)
    {
        case TEXTPARSER_BOM_NONE:
            break;
        case TEXTPARSER_BOM_UTF_8:
            local_hnd.text_format = TEXTPARSER_ENCODING_UTF_8;
            break;
        case TEXTPARSER_BOM_UTF_16_LE:
            local_hnd.text_format = TEXTPARSER_ENCODING_UTF_16;
            break;
        case TEXTPARSER_BOM_UTF_16_BE:
            local_hnd.text_format = TEXTPARSER_ENCODING_UTF_16;
            break;
        case TEXTPARSER_BOM_UTF_32_LE:
            local_hnd.text_format = TEXTPARSER_ENCODING_UTF_32;
            break;
        default:
            err = TEXTPARSER_ERROR_UNSUPPORTED_BOM;
            goto err;
    }

    local_hnd.no_lines = 0;
    local_hnd.lines = nullptr;
    local_hnd.chunk_size = calculate_chunk_size(local_hnd.text_size);

    switch(local_hnd.text_format) {
    case TEXTPARSER_ENCODING_LATIN1:
    case TEXTPARSER_ENCODING_UTF_8:
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
    case TEXTPARSER_ENCODING_UTF_32:
        break;
    default:
        err = TEXTPARSER_ERROR_INVALID_ENCODING;
        goto err;
    }

    if (local_hnd.text_format == TEXTPARSER_ENCODING_UTF_16 || local_hnd.text_format == TEXTPARSER_ENCODING_UNICODE) {
        if (local_hnd.text_size % sizeof(uint16_t) != 0) {
            err = TEXTPARSER_ERROR_INVALID_UTF16_SIZE;
            goto err;
        }
    } else if (local_hnd.text_format == TEXTPARSER_ENCODING_UTF_32) {
        if (local_hnd.text_size % sizeof(uint32_t) != 0) {
            err = TEXTPARSER_ERROR_INVALID_UTF32_SIZE;
            goto err;
        }
    }

    if (local_hnd.bom == TEXTPARSER_BOM_UTF_16_BE && local_hnd.text_size > 0) {
        void *swapped = textparser_convert_utf16be_to_native(local_hnd.text_addr, local_hnd.text_size);
        if (swapped == nullptr) {
            err = TEXTPARSER_ERROR_BYTE_ORDER_CONVERSION;
            goto err;
        }
        local_hnd.owned_buffer = swapped;
        local_hnd.text_addr = swapped;
    }

    *handle = malloc(sizeof(struct textparser_handle));
    if (*handle == nullptr) {
        err = TEXTPARSER_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    memcpy(*handle, &local_hnd, sizeof(struct textparser_handle));

    return TEXTPARSER_OK;

err:
    if (local_hnd.mmap_addr) {
        os_unmap(local_hnd.mmap_addr, local_hnd.mmap_size);
    }

    if (local_hnd.owned_buffer) {
        free(local_hnd.owned_buffer);
        local_hnd.owned_buffer = nullptr;
    }

    if (local_hnd.filename) {
        free(local_hnd.filename);
        local_hnd.filename = nullptr;
    }

    return err;
}


int textparser_openmem(const char *text, int len, int text_format, textparser_t *handle)
{
    if (len < 0 || handle == nullptr || text == nullptr) {
        return -1;
    }

    if ((size_t)len >= MAX_PARSE_SIZE) {
        return -1;
    }

    switch (text_format) {
    case TEXTPARSER_ENCODING_LATIN1:
    case TEXTPARSER_ENCODING_UTF_8:
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
    case TEXTPARSER_ENCODING_UTF_32:
        break;
    default:
        return -1;
    }

    if (text_format == TEXTPARSER_ENCODING_UTF_16 || text_format == TEXTPARSER_ENCODING_UNICODE) {
        if ((size_t)len % sizeof(uint16_t) != 0) {
            return -1;
        }
    } else if (text_format == TEXTPARSER_ENCODING_UTF_32) {
        if ((size_t)len % sizeof(uint32_t) != 0) {
            return -1;
        }
    }

    struct textparser_handle *ret = nullptr;

    ret = malloc(sizeof(struct textparser_handle));
    if (ret == nullptr) {
        return 6;
    }

    memset(ret, 0, sizeof(struct textparser_handle));

    ret->text_format = (enum textparser_encoding)text_format;
    ret->text_addr = text;
    ret->text_size = (size_t)len;
    ret->chunk_size = calculate_chunk_size(ret->text_size);
    ret->regex_ctx = adv_regex_context_create();

    *handle = (textparser_t)ret;

    return 0;
}

EXPORT_TEXTPARSER int textparser_set_text(textparser_t handle, const char *text, int len)
{
    if (handle == nullptr || text == nullptr)
        return -1;

    if (len < 0) {
        len = (int)strlen(text);
    }

    if ((size_t)len >= MAX_PARSE_SIZE)
        return -1;

    if (handle->lines) {
        free(handle->lines);
        handle->lines = nullptr;
        handle->no_lines = 0;
    }

    handle->text_addr = text;
    handle->text_size = (size_t)len;
    return 0;
}

static void free_post_processed_tokens(textparser_token_item *node)
{
    if (node == nullptr) return;
    textparser_token_item *c = node->child;
    while (c != nullptr) {
        textparser_token_item *next_sibling = c->next;
        free_post_processed_tokens(c);
        c = next_sibling;
    }
    if (node->text_flags & 0x80000000) {
        free(node);
    }
}

void textparser_close(textparser_t handle)
{
    void *mmap_addr = nullptr;
    size_t mmap_size = 0;

    if (handle == nullptr)
        return;

    textparser_free_regex(handle);

    mmap_addr = handle->mmap_addr;
    mmap_size = handle->mmap_size;

    if (mmap_addr) {
        os_unmap(mmap_addr, mmap_size);
    }

    if (handle->owned_buffer) {
        free(handle->owned_buffer);
        handle->owned_buffer = nullptr;
    }

    if (handle->first_item) {
        textparser_token_item *it = handle->first_item;
        while (it != nullptr) {
            textparser_token_item *next_item = it->next;
            free_post_processed_tokens(it);
            it = next_item;
        }
    }

    free_arena(handle);

    if (handle->lines) {
        free(handle->lines);
        handle->lines = nullptr;
    }

    if (handle->filename) {
        free(handle->filename);
        handle->filename = nullptr;
    }

    free(handle);
}

void textparser_cleanup(textparser_t *handle)
{
    if (handle)
    {
        textparser_close(*handle);
        *handle = nullptr;
    }
}

void textparser_set_filename(textparser_t handle, const char *filename)
{
    if (handle == nullptr)
        return;

    if (handle->filename) {
        free(handle->filename);
        handle->filename = nullptr;
    }

    if (filename) {
        handle->filename = strdup(filename);
        if (handle->filename == nullptr) {
            return;
        }
    }
}

const char *textparser_get_filename(const textparser_t handle)
{
    if (handle == nullptr)
        return nullptr;

    return handle->filename;
}

static const textparser_token_item *find_token_at_position_internal(const textparser_token_item *token, size_t position, int depth)
{
    if (depth >= MAX_RECURSION_DEPTH || token == nullptr) {
        return nullptr;
    }
    const textparser_token_item *best = nullptr;
    size_t curr_pos = (token->parent != nullptr) ? textparser_get_token_position(token) : 0;
    while (token != nullptr) {
        if (position >= curr_pos && position < curr_pos + token->len) {
            best = token;
            if (token->child != nullptr) {
                const textparser_token_item *child_match = find_token_at_position_internal(token->child, position, depth + 1);
                if (child_match) {
                    best = child_match;
                }
            }
            break;
        }
        curr_pos += token->len;
        token = token->next;
    }
    return best;
}


static const textparser_token_item *find_open_container(const textparser_token_item *token, size_t position, const textparser_language_definition *definition)
{
    const textparser_token_item *curr = token;
    while (curr != nullptr) {
        if (curr->token_id >= 0) {
            const textparser_token *def = &definition->tokens[curr->token_id];
            if (def->type == TEXTPARSER_TOKEN_TYPE_START_STOP ||
                def->type == TEXTPARSER_TOKEN_TYPE_START_OPT_STOP ||
                def->type == TEXTPARSER_TOKEN_TYPE_GROUP ||
                def->type == TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER ||
                def->type == TEXTPARSER_TOKEN_TYPE_SEQUENCE) {
                size_t curr_pos = textparser_get_token_position(curr);
                if (position >= curr_pos && position < curr_pos + curr->len) {
                    return curr;
                }
            }
        }
        curr = curr->parent;
    }
    return nullptr;
}

int textparser_parse(textparser_t handle, const textparser_language_definition *definition)
{
    if (handle == nullptr || definition == nullptr)
        return -1;
    return textparser_parse_incremental(handle, definition, 0, textparser_get_total_units(handle), handle->text_addr, textparser_get_total_units(handle), nullptr);
}

EXPORT_TEXTPARSER int textparser_parse_incremental(textparser_t handle,
                                                   const textparser_language_definition *definition,
                                                   size_t edit_offset,
                                                   size_t old_len,
                                                   const void *new_text,
                                                   size_t new_len,
                                                   textparser_dirty_range *out_range)
{
    if (handle == nullptr || definition == nullptr)
        return -1;

    // Reset error state
    handle->error = nullptr;
    handle->error_offset = 0;

    size_t unit_size = 1;
    switch (handle->text_format) {
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
        unit_size = sizeof(uint16_t);
        break;
    case TEXTPARSER_ENCODING_UTF_32:
        unit_size = sizeof(uint32_t);
        break;
    default:
        unit_size = 1;
        break;
    }

    size_t old_total_units = textparser_get_total_units(handle);
    if (edit_offset > old_total_units || edit_offset + old_len > old_total_units)
        return -1;

    size_t byte_offset = edit_offset * unit_size;
    size_t old_byte_len = old_len * unit_size;
    size_t new_byte_len = new_len * unit_size;
    ssize_t delta_units = (ssize_t)new_len - (ssize_t)old_len;
    ssize_t delta_bytes = (ssize_t)new_byte_len - (ssize_t)old_byte_len;
    size_t new_total_bytes = (size_t)((ssize_t)handle->text_size + delta_bytes);

    if (new_total_bytes >= MAX_PARSE_SIZE)
        return -1;

    // Splicing the text buffer if this is an actual edit
    if (new_text != handle->text_addr || delta_bytes != 0) {
        if (handle->owned_buffer == nullptr) {
            size_t cap = (new_total_bytes + unit_size + 1024) * 2;
            void *buf = malloc(cap);
            if (buf == nullptr) return -1;
            if (handle->text_addr && handle->text_size > 0) {
                memcpy(buf, handle->text_addr, handle->text_size);
            }
            handle->owned_buffer = buf;
            handle->owned_buffer_capacity = cap;
            handle->text_addr = (const char *)buf;
        } else if (new_total_bytes + unit_size > handle->owned_buffer_capacity) {
            size_t cap = (new_total_bytes + unit_size + 1024) * 2;
            void *buf = realloc(handle->owned_buffer, cap);
            if (buf == nullptr) return -1;
            handle->owned_buffer = buf;
            handle->owned_buffer_capacity = cap;
            handle->text_addr = (const char *)buf;
        }

        char *buf = (char *)handle->owned_buffer;
        size_t suffix_bytes = handle->text_size - (byte_offset + old_byte_len);
        if (suffix_bytes > 0 && delta_bytes != 0) {
            memmove(buf + byte_offset + new_byte_len, buf + byte_offset + old_byte_len, suffix_bytes);
        }
        if (new_byte_len > 0 && new_text != nullptr) {
            memcpy(buf + byte_offset, new_text, new_byte_len);
        }
        memset(buf + new_total_bytes, 0, unit_size);
        handle->text_size = new_total_bytes;
        handle->text_addr = buf;
    }

    size_t start_pos = edit_offset;
    size_t end_pos = edit_offset + new_len;
    size_t old_end_bound = edit_offset + old_len;
    size_t total = textparser_get_total_units(handle);

    // If doing a full parse from offset 0 to EOF, reset existing arena tree
    if (start_pos == 0 && end_pos >= total)
    {
        free_arena(handle);
        handle->first_item = nullptr;
    }

    if (handle->language != definition)
    {
        textparser_free_regex(handle);
        handle->language = definition;
        if (textparser_init_regex(handle) != 0)
            return -1;
    }

    // Resolve active token from existing AST
    const textparser_token_item *active_token = nullptr;
    if (handle->first_item != nullptr && start_pos > 0) {
        active_token = find_token_at_position_internal(handle->first_item, start_pos - 1, 0);
    }

    const textparser_token_item *open_container = nullptr;
    if (active_token != nullptr) {
        open_container = find_open_container(active_token, start_pos, definition);
    }

    const int *effective_starts_with = definition->starts_with;
    textparser_token_item *parent_container = (textparser_token_item *)open_container;

    if (open_container) {
        effective_starts_with = get_effective_nested_tokens(handle, open_container->token_id, open_container);
    } else if (definition->override_start_tokens && handle->filename) {
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
                    bool matched = adv_regex_find_pattern_ctx(handle->regex_ctx, rule->regex, &rule_regex, handle->text_format, handle->text_addr, handle->text_size, &found_at, &found_len, !definition->case_sensitivity, true);
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

    textparser_token_item *prev_item = nullptr;
    textparser_token_item *sibling_list = parent_container ? parent_container->child : handle->first_item;

    if (start_pos > 0 && sibling_list != nullptr)
    {
        textparser_token_item *curr = sibling_list;
        size_t curr_pos = parent_container ? textparser_get_token_position(parent_container) : 0;
        while (curr)
        {
            if (curr_pos + curr->len <= start_pos)
            {
                prev_item = curr;
            }
            else
            {
                break;
            }
            curr_pos += curr->len;
            curr = curr->next;
        }
    }

    textparser_token_item *tail_first = nullptr;
    if (sibling_list != nullptr && old_end_bound < old_total_units)
    {
        textparser_token_item *curr = prev_item ? prev_item->next : sibling_list;
        size_t curr_pos = prev_item ? (textparser_get_token_position(prev_item) + prev_item->len) : (parent_container ? textparser_get_token_position(parent_container) : 0);
        while (curr)
        {
            if (curr_pos >= old_end_bound)
            {
                tail_first = curr;
                break;
            }
            curr_pos += curr->len;
            curr = curr->next;
        }
    }

    size_t pos = start_pos;
    textparser_token_item *first_new_token = nullptr;
    textparser_token_item *last_new_token = nullptr;

    while(pos < end_pos) {
        size_t ws_skipped = textparser_skip_whitespace(handle, pos) - pos;
        if (ws_skipped > 0) {
            textparser_token_item **head_ptr = parent_container ? &parent_container->child : &handle->first_item;
            append_unprocessed_if_needed(handle, parent_container, head_ptr, &prev_item, ws_skipped);
            if (first_new_token == nullptr) first_new_token = prev_item;
            last_new_token = prev_item;
            pos += ws_skipped;
        }
        if (pos >= end_pos)
            break;

        textparser_token_item *token_item = nullptr;
        textparser_token_item *error_token_item = nullptr;
        for (int c = 0; effective_starts_with && effective_starts_with[c] != TextParser_END; c++) {
            int token_id = effective_starts_with[c];
            ssize_t offset = textparser_find_token(handle, token_id, pos, definition->other_text_inside, parent_container, prev_item);
            if (offset == 0)
            {
                textparser_checkpoint_t cp = textparser_checkpoint_save(handle);
                const char *saved_err = handle->error;
                size_t saved_err_off = handle->error_offset;

                int parent_tok_id = parent_container ? parent_container->token_id : TextParser_END;
                textparser_token_item *attempt = textparser_parse_token(handle, token_id, parent_tok_id, TEXTPARSER_SEARCH_END_TOKEN, pos, parent_container, prev_item);
                if (attempt != nullptr && handle->error == nullptr && attempt->len > 0) {
                    token_item = attempt;
                    break;
                }

                if (attempt != nullptr && handle->error != nullptr && error_token_item == nullptr) {
                    error_token_item = attempt;
                } else {
                    textparser_checkpoint_restore(handle, &cp);
                    handle->error = saved_err;
                    handle->error_offset = saved_err_off;
                }
            }
        }

        if (token_item == nullptr && error_token_item != nullptr) {
            token_item = error_token_item;
        }

        if (token_item != nullptr) {
            if (parent_container) {
                token_item->parent = parent_container;
            }

            if (first_new_token == nullptr)
                first_new_token = token_item;

            if (prev_item) {
                prev_item->next = token_item;
                token_item->prev = prev_item;
            } else if (parent_container) {
                parent_container->child = token_item;
            } else {
                handle->first_item = token_item;
            }

            size_t token_advance = token_item->len;
            maybe_merge_sign(handle, token_item);

            if ((handle->error)||(token_item->len <= 0))
                return -1;

            pos += token_advance;
            prev_item = token_item;
            last_new_token = token_item;
        } else {
            if (definition->other_text_inside) {
                size_t char_l = textparser_char_len(handle, pos);
                textparser_token_item **head_ptr = parent_container ? &parent_container->child : &handle->first_item;
                append_unprocessed_if_needed(handle, parent_container, head_ptr, &prev_item, char_l);
                if (first_new_token == nullptr) first_new_token = prev_item;
                last_new_token = prev_item;
                pos += char_l;
            } else {
                break;
            }
        }
    }

    if (pos < end_pos) {
        textparser_token_item **head_ptr = parent_container ? &parent_container->child : &handle->first_item;
        append_unprocessed_if_needed(handle, parent_container, head_ptr, &prev_item, end_pos - pos);
        if (first_new_token == nullptr) first_new_token = prev_item;
        last_new_token = prev_item;
        pos = end_pos;
    }

    textparser_token_item *stitch_point = last_new_token ? last_new_token : prev_item;
    if (tail_first) {
        if (stitch_point) {
            stitch_point->next = tail_first;
            tail_first->prev = stitch_point;
        } else if (parent_container) {
            parent_container->child = tail_first;
            tail_first->prev = nullptr;
        } else {
            handle->first_item = tail_first;
            tail_first->prev = nullptr;
        }
    } else if (stitch_point) {
        stitch_point->next = nullptr;
    } else if (parent_container) {
        parent_container->child = nullptr;
    } else {
        handle->first_item = nullptr;
    }

    if (parent_container && delta_units != 0) {
        textparser_token_item *p = parent_container;
        while (p) {
            p->len = (size_t)((ssize_t)p->len + delta_units);
            p = p->parent;
        }
    }

    if (out_range != nullptr) {
        size_t d_start = start_pos;
        if (first_new_token != nullptr) {
            d_start = textparser_get_token_position(first_new_token);
        } else if (prev_item != nullptr) {
            d_start = textparser_get_token_position(prev_item);
        }
        size_t d_end = end_pos;
        if (tail_first != nullptr) {
            d_end = textparser_get_token_position(tail_first);
        } else {
            d_end = textparser_get_total_units(handle);
        }
        out_range->dirty_start = d_start;
        out_range->dirty_end = d_end;
    }

    if (handle->lines && (delta_units != 0 || start_pos == 0)) {
        free(handle->lines);
        handle->lines = nullptr;
        handle->no_lines = 0;
    }

    return 0;
}

static bool get_operator_info(
    const textparser_language_definition *language,
    int token_id,
    int *out_precedence,
    enum textparser_associativity *out_assoc,
    bool *out_is_prefix
)
{
    if (language == nullptr || language->operator_precedence == nullptr || token_id < 0) {
        return false;
    }

    const textparser_operator_precedence *op_prec = language->operator_precedence;
    for (size_t r = 0; r < op_prec->count; r++) {
        const textparser_precedence_rule *rule = &op_prec->rules[r];
        if (rule->operators == nullptr) continue;
        for (int i = 0; rule->operators[i] != TextParser_END; i++) {
            if (rule->operators[i] == token_id) {
                if (out_precedence) *out_precedence = (int)(r + 1);
                if (out_assoc) *out_assoc = rule->associativity;
                if (out_is_prefix) {
                    const char *name = (language->tokens != nullptr) ? language->tokens[token_id].name : nullptr;
                    *out_is_prefix = (name != nullptr && (strstr(name, "Not") != nullptr || strstr(name, "Unary") != nullptr));
                }
                return true;
            }
        }
    }
    return false;
}

static textparser_token_item *make_unary_node(
    textparser_token_item *op_token,
    textparser_token_item *operand
)
{
    if (op_token == nullptr) return operand;

    textparser_token_item *parent_node = calloc(1, sizeof(textparser_token_item));
    if (parent_node == nullptr) return op_token;

    parent_node->token_id = op_token->token_id;
    parent_node->text_color = op_token->text_color;
    parent_node->text_background = op_token->text_background;
    parent_node->text_flags = op_token->text_flags | 0x80000000;

    op_token->prev = nullptr;
    op_token->next = operand;
    if (operand != nullptr) {
        operand->prev = op_token;
    }

    parent_node->child = op_token;

    size_t total_len = 0;
    for (textparser_token_item *c = op_token; c != nullptr; c = c->next) {
        c->parent = parent_node;
        total_len += c->len;
    }
    parent_node->len = total_len;

    return parent_node;
}

static textparser_token_item *make_binary_node(
    textparser_token_item *left,
    textparser_token_item *op_token,
    textparser_token_item *right
)
{
    if (op_token == nullptr) return left ? left : right;
    if (left == nullptr) return make_unary_node(op_token, right);

    textparser_token_item *parent_node = calloc(1, sizeof(textparser_token_item));
    if (parent_node == nullptr) return op_token;

    parent_node->token_id = op_token->token_id;
    parent_node->text_color = op_token->text_color;
    parent_node->text_background = op_token->text_background;
    parent_node->text_flags = op_token->text_flags | 0x80000000;

    textparser_token_item *left_last = left;
    while (left_last->next != nullptr) {
        left_last = left_last->next;
    }

    left_last->next = op_token;
    op_token->prev = left_last;

    if (right != nullptr) {
        op_token->next = right;
        right->prev = op_token;
    } else {
        op_token->next = nullptr;
    }

    parent_node->child = left;

    size_t total_len = 0;
    for (textparser_token_item *c = left; c != nullptr; c = c->next) {
        c->parent = parent_node;
        total_len += c->len;
    }
    parent_node->len = total_len;

    return parent_node;
}

static textparser_token_item *pratt_parse_expression_stream(
    const textparser_language_definition *language,
    textparser_token_item **items,
    int count,
    int *idx,
    int min_precedence
)
{
    if (*idx >= count) return nullptr;

    textparser_token_item *left = nullptr;

    // Collect any leading trivia before first operand/operator
    textparser_token_item *leading_trivia_head = nullptr;
    textparser_token_item *leading_trivia_tail = nullptr;
    while (*idx < count && items[*idx]->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED) {
        textparser_token_item *t = items[(*idx)++];
        t->prev = leading_trivia_tail;
        t->next = nullptr;
        if (leading_trivia_tail) leading_trivia_tail->next = t;
        else leading_trivia_head = t;
        leading_trivia_tail = t;
    }

    if (*idx >= count) {
        return leading_trivia_head;
    }

    textparser_token_item *token = items[(*idx)++];

    int op_prec = 0;
    enum textparser_associativity assoc = TEXTPARSER_ASSOC_LEFT;
    bool is_prefix = false;

    if (get_operator_info(language, token->token_id, &op_prec, &assoc, &is_prefix) && is_prefix) {
        token->prev = nullptr;
        token->next = nullptr;
        textparser_token_item *operand = pratt_parse_expression_stream(language, items, count, idx, op_prec);
        left = make_unary_node(token, operand);
    } else {
        token->prev = nullptr;
        token->next = nullptr;
        left = token;
    }

    if (leading_trivia_head != nullptr) {
        leading_trivia_tail->next = left;
        left->prev = leading_trivia_tail;
        left = leading_trivia_head;
    }

    while (*idx < count) {
        int scan_idx = *idx;
        while (scan_idx < count && items[scan_idx]->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED) {
            scan_idx++;
        }
        if (scan_idx >= count) break;

        textparser_token_item *op_cand = items[scan_idx];
        int cand_prec = 0;
        enum textparser_associativity cand_assoc = TEXTPARSER_ASSOC_LEFT;
        bool cand_is_prefix = false;

        if (!get_operator_info(language, op_cand->token_id, &cand_prec, &cand_assoc, &cand_is_prefix) || cand_is_prefix) {
            break;
        }

        if (cand_prec < min_precedence) {
            break;
        }

        while (*idx < scan_idx) {
            textparser_token_item *trivia = items[(*idx)++];
            trivia->prev = nullptr;
            trivia->next = nullptr;
            textparser_token_item *left_last = left;
            while (left_last->next != nullptr) left_last = left_last->next;
            left_last->next = trivia;
            trivia->prev = left_last;
        }

        textparser_token_item *op_token = items[(*idx)++];
        op_token->prev = nullptr;
        op_token->next = nullptr;

        int next_min_prec = (cand_assoc == TEXTPARSER_ASSOC_LEFT) ? (cand_prec + 1) : cand_prec;
        textparser_token_item *right = pratt_parse_expression_stream(language, items, count, idx, next_min_prec);

        left = make_binary_node(left, op_token, right);
    }

    return left;
}

static void textparser_post_process_expressions(textparser_token_item **head, const textparser_language_definition *language)
{
    if (head == nullptr || *head == nullptr || language == nullptr || language->operator_precedence == nullptr) return;

    bool has_operator = false;
    int count = 0;
    for (textparser_token_item *curr = *head; curr != nullptr; curr = curr->next) {
        count++;
        if (curr->token_id >= 0) {
            int prec = 0;
            if (get_operator_info(language, curr->token_id, &prec, nullptr, nullptr)) {
                has_operator = true;
            }
        }
    }

    if (!has_operator || count <= 1) return;

    textparser_token_item **items = malloc(count * sizeof(textparser_token_item *));
    if (items == nullptr) return;

    int i = 0;
    for (textparser_token_item *curr = *head; curr != nullptr; curr = curr->next) {
        items[i++] = curr;
    }

    int idx = 0;
    textparser_token_item *new_head = nullptr;
    textparser_token_item *new_tail = nullptr;

    while (idx < count) {
        textparser_token_item *parsed = pratt_parse_expression_stream(language, items, count, &idx, 0);
        if (parsed == nullptr) break;

        if (new_head == nullptr) {
            new_head = parsed;
            new_tail = parsed;
        } else {
            new_tail->next = parsed;
            parsed->prev = new_tail;
            new_tail = parsed;
        }
        while (new_tail->next != nullptr) {
            new_tail = new_tail->next;
        }
    }

    textparser_token_item *orig_parent = (*head)->parent;
    for (textparser_token_item *c = new_head; c != nullptr; c = c->next) {
        c->parent = orig_parent;
    }

    *head = new_head;
    free(items);
}

static bool is_operator_group(const textparser_token_item *node, const textparser_language_definition *language)
{
    if (node == nullptr || node->token_id < 0 || language == nullptr || language->tokens == nullptr) return false;
    enum textparser_token_type ttype = language->tokens[node->token_id].type;
    if (ttype != TEXTPARSER_TOKEN_TYPE_GROUP &&
        ttype != TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER &&
        ttype != TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY) {
        return false;
    }
    const char *name = language->tokens[node->token_id].name;
    if (name != nullptr && strcmp(name, "Operator") == 0) return true;

    if (node->child == nullptr) return false;
    for (const textparser_token_item *c = node->child; c != nullptr; c = c->next) {
        if (c->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED) continue;
        int prec = 0;
        if (!get_operator_info(language, c->token_id, &prec, nullptr, nullptr)) {
            return false;
        }
    }
    return true;
}

static void unwrap_node(textparser_token_item **root, textparser_token_item *curr)
{
    if (curr == nullptr || curr->child == nullptr) return;
    textparser_token_item *first_child = curr->child;
    textparser_token_item *last_child = curr->child;
    while (last_child->next) {
        last_child->parent = curr->parent;
        last_child = last_child->next;
    }
    last_child->parent = curr->parent;
    first_child->prev = curr->prev;
    last_child->next = curr->next;

    if (curr->prev) {
        curr->prev->next = first_child;
    } else if (curr->parent) {
        curr->parent->child = first_child;
    } else if (root) {
        *root = first_child;
    }

    if (curr->next) {
        curr->next->prev = last_child;
    }
}

static void textparser_disambiguate_casts(textparser_token_item **root, const textparser_language_definition *language)
{
    if (root == nullptr || *root == nullptr || language == nullptr || language->cast_disambiguation == nullptr) return;
    const textparser_cast_disambiguation *cst = language->cast_disambiguation;
    if (cst->cast_token_id < 0) return;

    for (textparser_token_item *curr = *root; curr != nullptr; curr = curr->next) {
        if (curr->token_id < 0) continue;
        const char *name = language->tokens[curr->token_id].name;
        if (name == nullptr || (strcmp(name, "Parenthesis") != 0 && strcmp(name, "parenthesis") != 0)) continue;

        if (curr->child == nullptr) continue;

        bool has_type_token = false;
        bool all_valid_types = true;

        for (textparser_token_item *c = curr->child; c != nullptr; c = c->next) {
            if (c->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED) continue;
            if (c->token_id < 0) {
                all_valid_types = false;
                break;
            }
            const char *c_name = language->tokens[c->token_id].name;
            if (c_name == nullptr) {
                all_valid_types = false;
                break;
            }

            if (strcmp(c_name, "Operator") == 0 || strcmp(c_name, "operator") == 0) {
                continue;
            }

            if (strcmp(c_name, "Keyword") == 0 || strcmp(c_name, "keyword") == 0) {
                has_type_token = true;
                continue;
            }

            if (textparser_token_in_id_list(cst->type_tokens, c->token_id)) {
                has_type_token = true;
                continue;
            }

            all_valid_types = false;
            break;
        }

        if (has_type_token && all_valid_types) {
            textparser_token_item *after = curr->next;
            while (after && after->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED) {
                after = after->next;
            }
            if (after != nullptr && after->token_id >= 0) {
                curr->token_id = cst->cast_token_id;
                curr->text_color = language->tokens[cst->cast_token_id].text_color;
            }
        }
    }
}

static void textparser_disambiguate_templates(textparser_token_item **root, const textparser_language_definition *language)
{
    if (root == nullptr || *root == nullptr || language == nullptr || language->template_disambiguation == nullptr) return;
    const textparser_template_disambiguation *tpl = language->template_disambiguation;
    if (tpl->template_group_token_id < 0 && tpl->template_open_tokens == nullptr) return;

    textparser_token_item *curr = *root;
    while (curr) {
        textparser_token_item *next_item = curr->next;

        if (curr->token_id >= 0 && textparser_token_in_id_list(tpl->template_open_tokens, curr->token_id)) {
            textparser_token_item *prev = curr->prev;
            while (prev && prev->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED) {
                prev = prev->prev;
            }

            bool prev_is_qualifying = false;
            if (prev != nullptr && prev->token_id >= 0) {
                const char *p_name = language->tokens[prev->token_id].name;
                if (p_name && (strcmp(p_name, "Variable") == 0 || strcmp(p_name, "variable") == 0 ||
                               strcmp(p_name, "Keyword") == 0 || strcmp(p_name, "keyword") == 0 ||
                               strcmp(p_name, "TemplateGroup") == 0)) {
                    prev_is_qualifying = true;
                }
            }

            if (prev_is_qualifying) {
                int depth = 1;
                textparser_token_item *scan = curr->next;
                textparser_token_item *end_bracket = nullptr;
                bool is_valid_template = true;

                while (scan && depth > 0) {
                    if (scan->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED) {
                        scan = scan->next;
                        continue;
                    }
                    if (scan->token_id < 0) {
                        is_valid_template = false;
                        break;
                    }

                    const char *s_name = language->tokens[scan->token_id].name;
                    if (s_name && (strcmp(s_name, "CodeBlock") == 0 || strcmp(s_name, "ArrayIndex") == 0)) {
                        is_valid_template = false;
                        break;
                    }

                    if (textparser_token_in_id_list(tpl->template_open_tokens, scan->token_id)) {
                        depth++;
                    } else if (textparser_token_in_id_list(tpl->template_close_tokens, scan->token_id)) {
                        depth--;
                        if (depth == 0) {
                            end_bracket = scan;
                            break;
                        }
                    } else if (tpl->valid_inner_tokens && !textparser_token_in_id_list(tpl->valid_inner_tokens, scan->token_id)) {
                        is_valid_template = false;
                        break;
                    }

                    scan = scan->next;
                }

                if (depth == 0 && end_bracket != nullptr && is_valid_template && tpl->template_group_token_id >= 0) {
                    textparser_token_item *grp = calloc(1, sizeof(textparser_token_item));
                    if (grp != nullptr) {
                        grp->token_id = tpl->template_group_token_id;
                        grp->text_color = language->tokens[grp->token_id].text_color;
                        grp->text_flags = language->tokens[grp->token_id].text_flags | 0x80000000;
                        grp->parent = curr->parent;
                        grp->prev = curr->prev;
                        grp->next = end_bracket->next;

                        if (curr->prev) {
                            curr->prev->next = grp;
                        } else if (curr->parent) {
                            curr->parent->child = grp;
                        } else if (root) {
                            *root = grp;
                        }

                        if (end_bracket->next) {
                            end_bracket->next->prev = grp;
                        }

                        grp->child = curr;
                        curr->prev = nullptr;
                        end_bracket->next = nullptr;

                        size_t total_len = 0;
                        for (textparser_token_item *c = grp->child; c != nullptr; c = c->next) {
                            c->parent = grp;
                            total_len += c->len;
                        }
                        grp->len = total_len;

                        next_item = grp->next;
                        curr = grp;
                    }
                }
            }
        }
        curr = next_item;
    }
}

void textparser_post_process(textparser_token_item **root, const textparser_language_definition *language)
{
    if (root == nullptr || *root == nullptr || language == nullptr) return;

    /* First recursively process child subtrees */
    for (textparser_token_item *c = *root; c != nullptr; c = c->next) {
        if (c->child) {
            textparser_post_process(&c->child, language);
        }
    }

    /* Apply Type Cast disambiguation if configured */
    if (language->cast_disambiguation != nullptr) {
        textparser_disambiguate_casts(root, language);
    }

    /* Apply Template / Generics disambiguation if configured */
    if (language->template_disambiguation != nullptr) {
        textparser_disambiguate_templates(root, language);
    }

    /* Unwrap operator groups before Pratt expression parsing so all operators are direct siblings */
    textparser_token_item *it = *root;
    while (it) {
        textparser_token_item *next_sibling = it->next;
        if (is_operator_group(it, language) && it->child) {
            textparser_token_item *last_c = it->child;
            while (last_c->next) last_c = last_c->next;
            unwrap_node(root, it);
            it = last_c;
        }
        it = next_sibling;
    }

    /* Apply operator precedence Pratt parsing to current sibling list if applicable */
    if (language->operator_precedence != nullptr && language->operator_precedence->count > 0) {
        textparser_post_process_expressions(root, language);
    }

    textparser_token_item *curr = *root;

    while (curr) {
        textparser_token_item *next_sibling = curr->next;

        /* Check if this node has delete_if_only_one_child condition */
        if (curr->token_id >= 0 && language->tokens[curr->token_id].delete_if_only_one_child &&
            curr->child && textparser_get_semantic_children_count(curr) == 1) {
            textparser_token_item *first_child = curr->child;
            textparser_token_item *last_child = curr->child;
            while (last_child->next) {
                last_child->parent = curr->parent;
                last_child = last_child->next;
            }
            last_child->parent = curr->parent;
            first_child->prev = curr->prev;
            last_child->next = curr->next;

            if (curr->prev) {
                curr->prev->next = first_child;
            } else if (curr->parent) {
                curr->parent->child = first_child;
            } else {
                *root = first_child;
            }

            if (next_sibling) {
                next_sibling->prev = last_child;
            }

            curr = last_child;
        }

        curr = next_sibling;
    }
}

EXPORT_TEXTPARSER const char *textparser_parse_error(textparser_t handle)
{
    if (handle == nullptr)
        return nullptr;

    return handle->error;
}

EXPORT_TEXTPARSER size_t textparser_parse_error_position(textparser_t handle)
{
    if (handle == nullptr)
        return 0;

    return handle->error_offset;
}

const char *textparser_strerror(int error_code)
{
    switch (error_code) {
    case TEXTPARSER_OK:
        return "Success";
    case TEXTPARSER_ERROR_FILE_OPEN:
        return "Failed to open or map file";
    case TEXTPARSER_ERROR_INVALID_ARGUMENT:
        return "Invalid argument";
    case TEXTPARSER_ERROR_OUT_OF_MEMORY:
        return "Out of memory";
    case TEXTPARSER_ERROR_PARSE_FAILED:
        return "Parsing failed";
    case TEXTPARSER_ERROR_UNSUPPORTED_BOM:
        return "Unsupported byte order mark (BOM)";
    case TEXTPARSER_ERROR_BYTE_ORDER_CONVERSION:
        return "Byte order conversion failed";
    case TEXTPARSER_ERROR_INVALID_ENCODING:
        return "Invalid text encoding";
    case TEXTPARSER_ERROR_FILE_TOO_LARGE:
        return "File exceeds maximum parse size (16 MB)";
    case TEXTPARSER_ERROR_INVALID_UTF16_SIZE:
        return "Invalid UTF-16 size (not a multiple of 2 bytes)";
    case TEXTPARSER_ERROR_INVALID_UTF32_SIZE:
        return "Invalid UTF-32 size (not a multiple of 4 bytes)";
    default:
        return "Unknown error";
    }
}

void textparser_set_callback(textparser_t handle, void (*callback)(textparser_t, textparser_token_item *, enum textparser_callback_type callback_type, void *user_data), void *user_data)
{
    if (handle) {
        handle->callback = callback;
        handle->user_data = user_data;
    }
}

const char *textparser_get_text(textparser_t handle)
{
    if (handle == nullptr)
        return nullptr;

    return handle->text_addr;
}

size_t textparser_get_text_size(textparser_t handle)
{
    if (handle == nullptr)
        return 0;

    return handle->text_size;
}

textparser_token_item *textparser_get_first_token(const textparser_t handle)
{
    if (handle == nullptr)
        return nullptr;

    return handle->first_item;
}

static inline size_t encode_utf8_codepoint(uint32_t cp, char *out)
{
    if (cp <= 0x7F) {
        if (out) out[0] = (char)cp;
        return 1;
    } else if (cp <= 0x7FF) {
        if (out) {
            out[0] = (char)(0xC0 | (cp >> 6));
            out[1] = (char)(0x80 | (cp & 0x3F));
        }
        return 2;
    } else if (cp <= 0xFFFF) {
        if (out) {
            out[0] = (char)(0xE0 | (cp >> 12));
            out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[2] = (char)(0x80 | (cp & 0x3F));
        }
        return 3;
    } else if (cp <= 0x10FFFF) {
        if (out) {
            out[0] = (char)(0xF0 | (cp >> 18));
            out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
            out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[3] = (char)(0x80 | (cp & 0x3F));
        }
        return 4;
    }
    // Replacement character U+FFFD
    if (out) {
        out[0] = (char)0xEF;
        out[1] = (char)0xBF;
        out[2] = (char)0xBD;
    }
    return 3;
}

char *textparser_get_token_text(const textparser_t handle, const textparser_token_item *item)
{
    if ((handle == nullptr)||(item == nullptr)||(item->len <= 0))
        return nullptr;

    char *ret = nullptr;

    size_t total_units = textparser_get_total_units(handle);
    size_t item_pos = textparser_get_token_position(item);
    if (item->len > total_units || item_pos > total_units - item->len)
        return nullptr;

    if (handle->text_format == TEXTPARSER_ENCODING_UNICODE || handle->text_format == TEXTPARSER_ENCODING_UTF_16) {
        const uint16_t *src = (const uint16_t *)(handle->text_addr + textparser_get_byte_offset(handle, item_pos));
        size_t utf8_len = 0;
        for (size_t i = 0; i < item->len; i++) {
            uint32_t cp = src[i];
            if (cp >= 0xD800 && cp <= 0xDBFF && (i + 1 < item->len) && (src[i + 1] >= 0xDC00 && src[i + 1] <= 0xDFFF)) {
                cp = 0x10000 + (((src[i] - 0xD800) << 10) | (src[i + 1] - 0xDC00));
                i++;
            } else if (cp >= 0xD800 && cp <= 0xDFFF) {
                cp = 0xFFFD;
            }
            utf8_len += encode_utf8_codepoint(cp, nullptr);
        }

        ret = malloc(utf8_len + 1);
        if (ret) {
            size_t out_idx = 0;
            for (size_t i = 0; i < item->len; i++) {
                uint32_t cp = src[i];
                if (cp >= 0xD800 && cp <= 0xDBFF && (i + 1 < item->len) && (src[i + 1] >= 0xDC00 && src[i + 1] <= 0xDFFF)) {
                    cp = 0x10000 + (((src[i] - 0xD800) << 10) | (src[i + 1] - 0xDC00));
                    i++;
                } else if (cp >= 0xD800 && cp <= 0xDFFF) {
                    cp = 0xFFFD;
                }
                out_idx += encode_utf8_codepoint(cp, ret + out_idx);
            }
            ret[out_idx] = '\0';
        }
    } else if (handle->text_format == TEXTPARSER_ENCODING_UTF_32) {
        const uint32_t *src = (const uint32_t *)(handle->text_addr + textparser_get_byte_offset(handle, item_pos));
        size_t utf8_len = 0;
        for (size_t i = 0; i < item->len; i++) {
            uint32_t cp = src[i];
            if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
                cp = 0xFFFD;
            }
            utf8_len += encode_utf8_codepoint(cp, nullptr);
        }

        ret = malloc(utf8_len + 1);
        if (ret) {
            size_t out_idx = 0;
            for (size_t i = 0; i < item->len; i++) {
                uint32_t cp = src[i];
                if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
                    cp = 0xFFFD;
                }
                out_idx += encode_utf8_codepoint(cp, ret + out_idx);
            }
            ret[out_idx] = '\0';
        }
    } else {
        size_t byte_len = textparser_get_byte_len(handle, item->len);
        if (byte_len > SIZE_MAX - 1)
            return nullptr;
        ret = malloc(byte_len + 1);
        if (ret) {
            memcpy(ret, handle->text_addr + textparser_get_byte_offset(handle, item_pos), byte_len);
            ret[byte_len] = '\0';
        }
    }

    return ret;
}

uint16_t *textparser_get_token_text16(const textparser_t handle, const textparser_token_item *item)
{
    if ((handle == nullptr)||(item == nullptr)||(item->len <= 0))
        return nullptr;

    if (handle->text_format != TEXTPARSER_ENCODING_UNICODE && handle->text_format != TEXTPARSER_ENCODING_UTF_16)
        return nullptr;

    size_t total_units = textparser_get_total_units(handle);
    size_t item_pos = textparser_get_token_position(item);
    if (item->len > total_units || item_pos > total_units - item->len)
        return nullptr;

    if (item->len > (SIZE_MAX / sizeof(uint16_t)) - 1)
        return nullptr;

    uint16_t *ret = malloc((item->len + 1) * sizeof(uint16_t));
    if (ret) {
        const uint16_t *src = (const uint16_t *)(handle->text_addr + textparser_get_byte_offset(handle, item_pos));
        memcpy(ret, src, item->len * sizeof(uint16_t));
        ret[item->len] = 0;
    }
    return ret;
}

uint32_t *textparser_get_token_text32(const textparser_t handle, const textparser_token_item *item)
{
    if ((handle == nullptr)||(item == nullptr)||(item->len <= 0))
        return nullptr;

    if (handle->text_format != TEXTPARSER_ENCODING_UTF_32)
        return nullptr;

    size_t total_units = textparser_get_total_units(handle);
    size_t item_pos = textparser_get_token_position(item);
    if (item->len > total_units || item_pos > total_units - item->len)
        return nullptr;

    if (item->len > (SIZE_MAX / sizeof(uint32_t)) - 1)
        return nullptr;

    uint32_t *ret = malloc((item->len + 1) * sizeof(uint32_t));
    if (ret) {
        const uint32_t *src = (const uint32_t *)(handle->text_addr + textparser_get_byte_offset(handle, item_pos));
        memcpy(ret, src, item->len * sizeof(uint32_t));
        ret[item->len] = 0;
    }
    return ret;
}

void textparser_free_token_text(void *text)
{
    if (text) {
        free(text);
    }
}

const textparser_language_definition *textparser_get_language(const textparser_t handle)
{
    if (handle == nullptr)
        return nullptr;

    return handle->language;
}

size_t textparser_get_token_children_count(const textparser_token_item *token)
{
    if (token == nullptr)
        return 0;

    size_t ret = 0;

    const struct textparser_token_item * child = token->child;

    while (child)
    {
        ret++;
        child = child->next;
    }

    return ret;
}

const textparser_token_item *textparser_get_token_child(const textparser_token_item *token)
{
    if (token == nullptr)
        return nullptr;

    return token->child;
}

const textparser_token_item *textparser_get_token_next(const textparser_token_item *token)
{
    if (token == nullptr)
        return nullptr;

    return token->next;
}

const textparser_token_item *textparser_get_token_prev(const textparser_token_item *token)
{
    if (token == nullptr)
        return nullptr;

    return token->prev;
}

const char *textparser_get_token_type_str(const textparser_language_definition *language, const textparser_token_item *token)
{
    int token_id = 0;

    if (token == nullptr)
        return nullptr;

    if (token->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED)
        return "Unprocessed";

    if (token->token_id == TEXTPARSER_TOKEN_ID_ERROR)
        return "Error";

    if (language == nullptr)
        return nullptr;

    token_id = token->token_id;
    if (token_id < 0)
        return nullptr;

    for (int c = 0; c <= token_id; c++)
    {
        if (language->tokens[c].name == nullptr)
        {
            return nullptr;
        }

        if (c == token_id)
        {
            return language->tokens[c].name;
        }
    }

    return nullptr;
}

int textparser_get_token_type(const textparser_token_item *token)
{
    if (token == nullptr)
        return -1;

    return token->token_id;
}

size_t textparser_get_token_position(const textparser_token_item *token)
{
    if (token == nullptr)
        return 0;

    size_t offset = 0;
    const textparser_token_item *curr = token;

    while (curr != nullptr)
    {
        const textparser_token_item *sibling = curr->prev;
        while (sibling != nullptr)
        {
            offset += sibling->len;
            sibling = sibling->prev;
        }
        curr = curr->parent;
    }

    return offset;
}

size_t textparser_get_token_length(const textparser_token_item *token)
{
    if (token == nullptr)
        return 0;

    return token->len;
}

uint32_t textparser_get_token_text_color(const textparser_token_item *token)
{
    if (token == nullptr)
        return 0;

    return token->text_color;
}

uint32_t textparser_get_token_text_background(const textparser_token_item *token)
{
    if (token == nullptr)
        return 0;

    return token->text_background;
}

uint32_t textparser_get_token_text_flags(const textparser_token_item *token)
{
    if (token == nullptr)
        return 0;

    return token->text_flags;
}

const char *textparser_get_token_error(const textparser_token_item *token)
{
    if (token == nullptr)
        return nullptr;

    return token->error;
}

static void textparser_parse_state_recursively_fill_internal(const textparser_token_item *token, const textparser_token_item **state, size_t max_units, int depth)
{
    if (depth >= MAX_RECURSION_DEPTH) {
        return;
    }
    while (token != nullptr)
    {
        size_t pos = textparser_get_token_position(token);
        size_t len = token->len;

        for (size_t c = 0; c < len; c++)
        {
            if (pos + c < max_units) {
                state[pos + c] = token;
            }
        }

        if (token->child != nullptr)
        {
            textparser_parse_state_recursively_fill_internal(token->child, state, max_units, depth + 1);
        }
        token = token->next;
    }
}

static void textparser_parse_state_recursively_fill(const textparser_token_item *token, const textparser_token_item **state, size_t max_units)
{
    textparser_parse_state_recursively_fill_internal(token, state, max_units, 0);
}

textparser_parser_state *textparser_state_new(const textparser_t handle)
{
    textparser_parser_state *ret = nullptr;
    size_t to_allocate = 0;
    size_t allocated = 0;
    size_t size = 0;

    size = textparser_get_total_units(handle);

    if (size >= MAX_PARSE_SIZE)
        return nullptr;

    allocated = (size * sizeof(const textparser_token_item *));
    to_allocate = offsetof(textparser_parser_state, state) + allocated;

    ret = malloc(to_allocate);
    if (ret)
    {
        ret->len = size;
        memset(ret->state, 0, allocated);

        textparser_parse_state_recursively_fill(handle->first_item, ret->state, size);
    }

    return ret;
}

EXPORT_TEXTPARSER textparser_parser_state *textparser_state_generate(const textparser_t handle, size_t position)
{
    if (handle == nullptr)
        return nullptr;

    size_t total = textparser_get_total_units(handle);
    if (total >= MAX_PARSE_SIZE)
        return nullptr;

    if (position > total)
        position = total;

    size_t len = position;
    size_t allocated = len * sizeof(const textparser_token_item *);
    size_t to_allocate = offsetof(textparser_parser_state, state) + allocated;

    textparser_parser_state *ret = malloc(to_allocate);
    if (ret) {
        ret->len = len;
        if (allocated > 0) {
            memset(ret->state, 0, allocated);
        }
        if (position > 0) {
            const textparser_token_item *active = find_token_at_position_internal(handle->first_item, position - 1, 0);
            if (active) {
                ret->state[position - 1] = active;
            }
        }
    }
    return ret;
}

void textparser_state_free(textparser_parser_state *state)
{
    if (state)
    {
        free(state);
    }
}

void textparser_state_cleanup(textparser_parser_state **state)
{
    if (state)
    {
        textparser_state_free(*state);
        *state = nullptr;
    }
}

int textparser_build_line_map(textparser_t handle)
{
    if (handle == nullptr)
        return -1;

    if (handle->lines) {
        free(handle->lines);
        handle->lines = nullptr;
    }
    handle->no_lines = 0;

    if (handle->text_addr == nullptr || handle->text_size == 0) {
        return 0;
    }

    size_t total_units = textparser_get_total_units(handle);
    size_t count = 0;

    for (size_t ch = 0; ch < total_units; ch++) {
        if (textparser_get_unit_at(handle, ch) == '\n') {
            count++;
        }
    }

    if (count == 0) {
        return 0;
    }

    handle->lines = malloc(sizeof(size_t) * count);
    if (handle->lines == nullptr) {
        handle->no_lines = 0;
        return -1;
    }
    handle->no_lines = count;

    size_t cur_line_pos = 0;

    for (size_t ch = 0; ch < total_units; ch++) {
        if (textparser_get_unit_at(handle, ch) == '\n') {
            handle->lines[cur_line_pos++] = ch;
        }
    }

    return 0;
}

size_t textparser_get_line_count(const textparser_t handle)
{
    if (handle == nullptr)
        return 0;

    if (handle->text_addr == nullptr || handle->text_size == 0)
        return 0;

    return handle->no_lines + 1;
}

size_t textparser_get_line_start_position(const textparser_t handle, size_t line_index)
{
    if (handle == nullptr)
        return 0;

    if (line_index == 0)
        return 0;

    if (line_index > handle->no_lines) {
        return textparser_get_total_units(handle);
    }

    return handle->lines[line_index - 1] + 1;
}

size_t textparser_get_line_number_at_position(const textparser_t handle, size_t position)
{
    if (handle == nullptr || handle->no_lines == 0 || handle->lines == nullptr)
        return 0;

    size_t low = 0;
    size_t high = handle->no_lines;

    while (low < high) {
        size_t mid = low + (high - low) / 2;
        if (handle->lines[mid] >= position) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }

    return low;
}

typedef enum {
    QUERY_COMB_NONE = 0,
    QUERY_COMB_CHILD,      // '>'
    QUERY_COMB_DESCENDANT  // ' '
} query_combinator_t;

typedef struct {
    int token_id;           // Target token_id (-1 if unknown type name, -2 if '*')
    query_combinator_t comb; // Combinator connecting this step to step on its right in AST
} query_step_t;

typedef struct {
    query_step_t *steps;
    size_t step_count;
} query_sequence_t;

typedef struct {
    query_sequence_t *sequences;
    size_t sequence_count;
    size_t sequence_capacity;
} query_selector_t;

static int query_get_token_id_by_name(const textparser_language_definition *language, const char *name)
{
    if (!language || !language->tokens || !name)
        return -1;

    if (strcmp(name, "*") == 0)
        return -2;

    for (int c = 0; language->tokens[c].name != nullptr; c++)
    {
        if (strcmp(language->tokens[c].name, name) == 0)
            return c;
        if (!language->case_sensitivity && strcasecmp(language->tokens[c].name, name) == 0)
            return c;
    }

    return -1;
}

static void query_free_selector(query_selector_t *sel)
{
    if (!sel) return;
    if (sel->sequences) {
        for (size_t i = 0; i < sel->sequence_count; i++) {
            if (sel->sequences[i].steps) {
                free(sel->sequences[i].steps);
            }
        }
        free(sel->sequences);
    }
    sel->sequences = nullptr;
    sel->sequence_count = 0;
    sel->sequence_capacity = 0;
}

typedef struct {
    int token_id;
    query_combinator_t comb_after;
} temp_element_t;

static bool query_parse_sequence(const textparser_language_definition *language, const char *seq_str, size_t seq_len, query_sequence_t *out_seq)
{
    temp_element_t elements[128];
    size_t elem_count = 0;

    const char *p = seq_str;
    const char *end = seq_str + seq_len;

    while (p < end) {
        while (p < end && isspace((unsigned char)*p)) p++;
        if (p >= end) break;

        if (*p == '>') {
            if (elem_count == 0) return false;
            elements[elem_count - 1].comb_after = QUERY_COMB_CHILD;
            p++;
            continue;
        }

        const char *name_start = p;
        while (p < end && !isspace((unsigned char)*p) && *p != '>' && *p != ',') {
            p++;
        }
        size_t name_len = p - name_start;
        if (name_len == 0) break;

        if (elem_count > 0 && elements[elem_count - 1].comb_after == QUERY_COMB_NONE) {
            elements[elem_count - 1].comb_after = QUERY_COMB_DESCENDANT;
        }

        char name_buf[256];
        if (name_len >= sizeof(name_buf)) name_len = sizeof(name_buf) - 1;
        memcpy(name_buf, name_start, name_len);
        name_buf[name_len] = '\0';

        if (elem_count >= 128) return false;

        elements[elem_count].token_id = query_get_token_id_by_name(language, name_buf);
        elements[elem_count].comb_after = QUERY_COMB_NONE;
        elem_count++;
    }

    if (elem_count == 0) return false;

    out_seq->steps = malloc(elem_count * sizeof(query_step_t));
    if (!out_seq->steps) return false;
    out_seq->step_count = elem_count;

    for (size_t k = 0; k < elem_count; k++) {
        size_t step_idx = (elem_count - 1) - k;
        out_seq->steps[step_idx].token_id = elements[k].token_id;
        if (k == elem_count - 1) {
            out_seq->steps[step_idx].comb = QUERY_COMB_NONE;
        } else {
            out_seq->steps[step_idx].comb = elements[k].comb_after;
        }
    }

    return true;
}

static bool query_parse_selector(const textparser_language_definition *language, const char *selector, query_selector_t *out_sel)
{
    out_sel->sequences = nullptr;
    out_sel->sequence_count = 0;
    out_sel->sequence_capacity = 0;

    const char *p = selector;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        const char *seq_start = p;
        while (*p && *p != ',') p++;
        size_t seq_len = p - seq_start;

        query_sequence_t seq = {0};
        if (query_parse_sequence(language, seq_start, seq_len, &seq)) {
            if (out_sel->sequence_count >= out_sel->sequence_capacity) {
                size_t new_cap = out_sel->sequence_capacity == 0 ? 4 : out_sel->sequence_capacity * 2;
                query_sequence_t *new_seqs = realloc(out_sel->sequences, new_cap * sizeof(query_sequence_t));
                if (!new_seqs) {
                    if (seq.steps) free(seq.steps);
                    query_free_selector(out_sel);
                    return false;
                }
                out_sel->sequences = new_seqs;
                out_sel->sequence_capacity = new_cap;
            }
            out_sel->sequences[out_sel->sequence_count++] = seq;
        }

        if (*p == ',') p++;
    }

    return out_sel->sequence_count > 0;
}

static bool query_match_sequence(const textparser_token_item *candidate, const query_sequence_t *seq, const textparser_token_item *scope_root)
{
    if (seq->step_count == 0) return false;

    const textparser_token_item *curr = candidate;

    for (size_t i = 0; i < seq->step_count; i++) {
        const query_step_t *step = &seq->steps[i];

        if (i == 0) {
            if (step->token_id == -1) return false;
            if (step->token_id != -2 && curr->token_id != step->token_id) return false;
        } else {
            if (step->comb == QUERY_COMB_CHILD) {
                curr = curr->parent;
                if (!curr) return false;
                if (scope_root && curr == scope_root->parent) return false;
                if (step->token_id == -1) return false;
                if (step->token_id != -2 && curr->token_id != step->token_id) return false;
            } else if (step->comb == QUERY_COMB_DESCENDANT) {
                curr = curr->parent;
                bool found = false;
                while (curr) {
                    if (scope_root && curr == scope_root->parent) break;
                    if (step->token_id != -1 && (step->token_id == -2 || curr->token_id == step->token_id)) {
                        found = true;
                        break;
                    }
                    curr = curr->parent;
                }
                if (!found) return false;
            }
        }
    }
    return true;
}

static bool query_match_candidate(const textparser_token_item *candidate, const query_selector_t *sel, const textparser_token_item *scope_root)
{
    for (size_t s = 0; s < sel->sequence_count; s++) {
        if (query_match_sequence(candidate, &sel->sequences[s], scope_root)) {
            return true;
        }
    }
    return false;
}

EXPORT_TEXTPARSER const textparser_token_item **textparser_query(
    const textparser_t handle,
    const textparser_token_item *root,
    const char *selector,
    size_t *out_count
) {
    if (!out_count) return nullptr;
    *out_count = 0;

    if (!handle || !selector || strlen(selector) == 0) {
        return nullptr;
    }

    const textparser_language_definition *language = textparser_get_language(handle);
    if (!language) {
        return nullptr;
    }

    const textparser_token_item *start_node = root;
    if (!start_node) {
        start_node = textparser_get_first_token(handle);
    }
    if (!start_node) {
        return nullptr;
    }

    query_selector_t sel = {0};
    if (!query_parse_selector(language, selector, &sel) || sel.sequence_count == 0) {
        query_free_selector(&sel);
        return nullptr;
    }

    size_t capacity = 16;
    size_t count = 0;
    const textparser_token_item **results = malloc(capacity * sizeof(const textparser_token_item *));
    if (!results) {
        query_free_selector(&sel);
        return nullptr;
    }

    const textparser_token_item *curr = start_node;
    while (curr != nullptr) {
        if (query_match_candidate(curr, &sel, root)) {
            if (count >= capacity) {
                size_t new_cap = capacity * 2;
                const textparser_token_item **new_res = realloc((void *)results, new_cap * sizeof(const textparser_token_item *));
                if (!new_res) {
                    free((void *)results);
                    query_free_selector(&sel);
                    return nullptr;
                }
                results = new_res;
                capacity = new_cap;
            }
            results[count++] = curr;
        }

        if (curr->child != nullptr) {
            curr = curr->child;
        } else if (curr->next != nullptr) {
            curr = curr->next;
        } else {
            while (curr != nullptr && curr->next == nullptr) {
                curr = curr->parent;
                if (curr == start_node) {
                    curr = nullptr;
                    break;
                }
            }
            if (curr != nullptr) {
                if (curr == start_node) {
                    curr = nullptr;
                } else {
                    curr = curr->next;
                }
            }
        }
    }

    query_free_selector(&sel);

    *out_count = count;
    if (count == 0) {
        free((void *)results);
        return nullptr;
    }

    return results;
}

EXPORT_TEXTPARSER void textparser_free_query_result(const textparser_token_item **results)
{
    if (results) {
        free((void *)results);
    }
}

