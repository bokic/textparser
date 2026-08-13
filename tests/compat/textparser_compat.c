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
#include <stdatomic.h>
#include <ctype.h>
#if defined(_WIN32)
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

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

enum textparser_internal_bom {
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
    adv_regex_context *regex_ctx;
    void *start_regex;
    void *end_regex;
    void *mmap_addr;
    size_t mmap_size;
    void *owned_buffer;
    enum textparser_internal_bom bom;
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
    size_t chunk_size = 4096; // 4KB minimum
    while (chunk_size <= filesize && chunk_size < 16777216) {
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
            }
            break;
        default:
            LOGF("textparser_find_token() - unknown!!!!!");
            break;
    }

    ((struct textparser_handle *)handle)->recursion_depth--;
    return result;
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

static textparser_token_item *textparser_parse_token(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling);

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

    ret = allocate_token(handle);
    if (ret == nullptr) {
        return nullptr;
    }

    ret->parent = (textparser_token_item *)parent_item;
    offset = textparser_skip_whitespace(handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    LOGV("id: %d - [%s]  at offset: %zu", token_id, token_def->name, offset);

    int count = 0;
    while (effective_nested[count] != TextParser_END) {
        count++;
    }

    size_t closest = SIZE_MAX;
    int current_token_id = TextParser_END;
    {
        int adjusted_list[count + 1];
        adjust_search_order(effective_nested, adjusted_list);

        for (int c = 0; adjusted_list[c] != TextParser_END; c++)
        {
            ssize_t current_closest = textparser_find_token(handle, adjusted_list[c], offset, token_def->other_text_inside, parent_item, prev_sibling);
            if ((current_closest >= 0)&&((size_t)current_closest < closest))
            {
                closest = (size_t)current_closest;
                current_token_id = adjusted_list[c];
            }
        }
    }

    if (current_token_id == TextParser_END)
    {
        exit_with_error(handle, "Search for group_one_child token type failed. Can't find one child.", offset);
    }

    child = textparser_parse_token(handle, current_token_id, parent_token_id, parent_start_stop, offset, ret, prev_sibling);
    if (child == nullptr) {
        exit_with_error(handle, "Search for group_one_child token type failed. Child token parsing failed.", offset);
    }
    child->parent = ret;
    LOGV("TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY - Found [%s] at %zu", handle->language->tokens[child->token_id].name, child->position);
    ret->position = child->position;
    ret->len = child->len;
    ret->child = child;
    check_and_exit_on_fatal_parsing_error(handle, child, offset);

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

static textparser_token_item *parse_token_group(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
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

    LOGV("enter TEXTPARSER_TOKEN_TYPE_GROUP");
    const int *effective_nested = get_effective_nested_tokens(handle, token_id, parent_item);
    if (!effective_nested) {
        exit_with_error(handle, "nested_tokens list is empty!", offset);
    }

    ret = allocate_token(handle);
    if (ret == nullptr) {
        return nullptr;
    }

    ret->parent = (textparser_token_item *)parent_item;
    offset = textparser_skip_whitespace(handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    LOGV("id: %d - [%s]  at offset: %zu", token_id, token_def->name, offset);

    int current_token_id;
    while(1) {
        offset = textparser_skip_whitespace(handle, offset);
        if (offset >= textparser_get_total_units(handle))
        {
            if (child)
            {
                ret->len = offset - ret->position;
                goto exit;
            }
            exit_with_error(handle, "Search for group token type failed. Can't find any child.", offset);
        }

        const textparser_token_item *current_prev = (child == nullptr) ? prev_sibling : child;

        const char *parent_regex_pattern = nullptr;
        void **parent_regex_compiled_ptr = nullptr;

        if (parent_token_id != TextParser_END) {
            switch(parent_start_stop)
            {
            case TEXTPARSER_SEARCH_END_TOKEN:
                parent_regex_pattern = definition->tokens[parent_token_id].end_regex;
                parent_regex_compiled_ptr = (void **)handle->end_regex + parent_token_id;
                break;
            case TEXTPARSER_SEARCH_START_TOKEN:
                parent_regex_pattern = definition->tokens[parent_token_id].start_regex;
                parent_regex_compiled_ptr = (void **)handle->start_regex + parent_token_id;
                break;
            default:
                exit_with_error(handle, "Unknown parent_start_stop value!", offset);
            }
        }

        if (parent_regex_pattern) {
            size_t parent_match_len = 0;
            bool found_parent_token = adv_regex_find_pattern_ctx(handle->regex_ctx, parent_regex_pattern, parent_regex_compiled_ptr, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &parent_match_len, !handle->language->case_sensitivity, true);

            if (found_parent_token)
            {
                ret->len = offset - ret->position;
                break;
            }
        }

        current_token_id = TextParser_END;

        const int *loop_effective_nested = get_effective_nested_tokens(handle, token_id, ret);
        int count = 0;
        while (loop_effective_nested[count] != TextParser_END) {
            count++;
        }
        {
            int adjusted_list[count + 1];
            adjust_search_order(loop_effective_nested, adjusted_list);

            for (int c = 0; adjusted_list[c] != TextParser_END; c++)
            {
                ssize_t current_closest = textparser_find_token(handle, adjusted_list[c], offset, token_def->other_text_inside, ret, current_prev);
                if (current_closest == 0)
                {
                    current_token_id = adjusted_list[c];
                    break;
                }
            }
        }

        if (current_token_id != TextParser_END)
        {
            textparser_token_item *new_child = textparser_parse_token(handle, current_token_id, parent_token_id, parent_start_stop, offset, ret, current_prev);
            if (new_child == nullptr) {
                exit_with_error(handle, "Parsing child token failed", offset);
            }
            if (handle->error) {
                // A token matched at this offset but failed to parse completely.
                // When the group allows arbitrary text, treat it as other text
                // instead of aborting the whole parse.
                if (token_def->other_text_inside && offset < textparser_get_total_units(handle)) {
                    handle->error = nullptr;
                    handle->error_offset = 0;
                    offset += textparser_char_len(handle, offset);
                    continue;
                }
                goto exit;
            }

            if (child == nullptr) {
                child = new_child;
                child->parent = ret;
                ret->child = child;
            } else {
                new_child->parent = ret;
                new_child->prev = child;
                child->next = new_child;
                child = new_child;
            }

            maybe_merge_sign(handle, child);

            if (child->len == 0) {
                exit_with_error(handle, "0-length child token match caused infinite loop", offset);
            }

            offset = child->position + child->len;
            ret->len = child->position + child->len - ret->position;
        }
        else
        {
            if (token_def->other_text_inside && offset < textparser_get_total_units(handle))
            {
                offset += textparser_char_len(handle, offset);
            }
            else
            {
                if (child)
                {
                    ret->len = offset - ret->position;
                    goto exit;
                }
                exit_with_error(handle, "Unrecognized token inside group", offset);
            }
        }
    }

exit:
    return ret;
}

static textparser_token_item *parse_token_group_all_children_in_same_order(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
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

    ret = allocate_token(handle);
    if (ret == nullptr) {
        return nullptr;
    }

    ret->parent = (textparser_token_item *)parent_item;
    offset = textparser_skip_whitespace(handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    ssize_t start_pos = textparser_find_token(handle, start_token_id, offset, definition->other_text_inside, ret, prev_sibling);
    if (start_pos != 0) {
        exit_with_error(handle, "Expected start token!", offset);
    }

    child = textparser_parse_token(handle, start_token_id, parent_token_id, parent_start_stop, offset, ret, prev_sibling);
    if (child == nullptr) {
        exit_with_error(handle, "Parsing start token failed", offset);
    }
    child->parent = ret;
    check_and_exit_on_fatal_parsing_error(handle, child, offset);

    ret->child = child;
    last_child = child;

    offset = child->position + child->len;

    while(1)
    {
        offset = textparser_skip_whitespace(handle, offset);
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
            child->parent = ret;
            child->prev = last_child;
            last_child->next = child;
            last_child = child;
            check_and_exit_on_fatal_parsing_error(handle, child, offset);

            if (child->len == 0) {
                exit_with_error(handle, "0-length child token match caused infinite loop", offset);
            }

            offset = child->position + child->len;
            continue;
        }

        if (definition->other_text_inside) {
            offset += textparser_char_len(handle, offset);
        } else {
            exit_with_error(handle, "Expected inner or end token!", offset);
        }
    }

    offset = textparser_skip_whitespace(handle, offset);
    child = textparser_parse_token(handle, end_token_id, parent_token_id, parent_start_stop, offset, ret, last_child);
    if (child == nullptr) {
        exit_with_error(handle, "Parsing end token failed", offset);
    }
    child->parent = ret;
    child->prev = last_child;
    last_child->next = child;
    check_and_exit_on_fatal_parsing_error(handle, child, offset);

    ret->len = child->position + child->len - ret->position;

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
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

    ret = allocate_token(handle);
    if (ret == nullptr) {
        exit_with_error(handle, "Can't allocate memory!", offset);
    }

    offset = textparser_skip_whitespace(handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    size_t len = 0;
    if (!adv_regex_find_pattern_ctx(handle->regex_ctx, token_def->start_regex, (void **)handle->start_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &len, !handle->language->case_sensitivity, true)) {
        exit_with_error(handle, "Can't find start of the token!", offset);
    }

    LOGV("TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN - Found [%s] at %zu", handle->language->tokens[ret->token_id].name, ret->position);
    ret->position = offset;
    ret->len = len;

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

static textparser_token_item *parse_token_start_stop(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, bool stop_required, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    (void)parent_start_stop;
    (void)parent_item;
    (void)prev_sibling;
    (void)parent_token_id;
    textparser_token_item *ret = nullptr;

    if (handle == nullptr) {
        exit_with_error(handle, "handle == nullptr!", offset);
    }

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];
    textparser_token_item *child = nullptr;

    size_t token_end = 0;
    size_t len = 0;

    if (stop_required) {
        LOGV("enter TEXTPARSER_TOKEN_TYPE_START_STOP");
    } else {
        LOGV("enter TEXTPARSER_TOKEN_TYPE_START_OPT_STOP");
    }

    if (offset >= textparser_get_total_units(handle)) {
        exit_with_error(handle, "offset >= total units count!", offset);
    }

    ret = allocate_token(handle);
    if (ret == nullptr) {
        exit_with_error(handle, "Can't allocate memory!", offset);
    }

    offset = textparser_skip_whitespace(handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    // Search for start token
    if (!adv_regex_find_pattern_ctx(handle->regex_ctx, token_def->start_regex, (void **)handle->start_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &len, !handle->language->case_sensitivity, true)) {
        exit_with_error(handle, "Can't find start of the token!", offset);
    }

    ret->position = offset;
    offset = ret->position + len;
    ret->len = len; // Temporarily set len to start token length so it can be used for prev_sibling context

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_START, handle->user_data);
    }

    if (offset > textparser_get_total_units(handle)) {
        exit_with_error(handle, "offset >= total units count!", offset);
    }

    if (offset == textparser_get_total_units(handle)) {
        exit_with_error(handle, "reached end of text!", offset);
    }

    const int *effective_nested = get_effective_nested_tokens(handle, token_id, ret);
    if (effective_nested)
    {
        textparser_token_item *last_child = nullptr;
        const int *nested_tokens = effective_nested;

        while (1) {
            offset = textparser_skip_whitespace(handle, offset);

            if (offset >= textparser_get_total_units(handle))
            {
                exit_with_error(handle, "Reached end of text before finding end token!", offset);
            }

            // 1. If search_parent_end_token_last is false, check end token first
            if (token_def->search_parent_end_token_last == false)
            {
                size_t end_match_len = 0;
                bool found_end = adv_regex_find_pattern_ctx(handle->regex_ctx, token_def->end_regex, (void **)handle->end_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &end_match_len, !handle->language->case_sensitivity, true);
                if (found_end)
                {
                    break;
                }
            }

            // 2. Check if any nested token matches at offset
            int child_token_id = TextParser_END;
            const textparser_token_item *current_prev = (last_child == nullptr) ? ret : last_child;

            int nested_count = 0;
            while (nested_tokens[nested_count] != TextParser_END) {
                nested_count++;
            }
            {
                int adjusted_list[nested_count + 1];
                adjust_search_order(nested_tokens, adjusted_list);

                for(int c = 0; adjusted_list[c] != TextParser_END; c++)
                {
                    ssize_t pos = textparser_find_token(handle, adjusted_list[c], offset, token_def->other_text_inside, ret, current_prev);
                    if (pos == 0)
                    {
                        child_token_id = adjusted_list[c];
                        break;
                    }
                }
            }

            if (child_token_id != TextParser_END)
            {
                textparser_token_item *new_child = textparser_parse_token(handle, child_token_id, token_id, TEXTPARSER_SEARCH_END_TOKEN, offset, ret, current_prev);
                if (new_child == nullptr) {
                    exit_with_error(handle, "Parsing nested child token failed", offset);
                }
                if (handle->error) {
                    // A nested token matched at this offset but failed to parse
                    // completely. When this token allows arbitrary text, treat it
                    // as other text instead of aborting the whole parse.
                    if (token_def->other_text_inside && offset < textparser_get_total_units(handle)) {
                        handle->error = nullptr;
                        handle->error_offset = 0;
                        offset += textparser_char_len(handle, offset);
                        continue;
                    }
                    goto exit;
                }

                child = new_child;
                child->parent = ret;
                if (last_child) {
                    child->prev = last_child;
                }
                if (ret->child == nullptr)
                    ret->child = child;

                if (last_child)
                    last_child->next = child;
                last_child = child;

                maybe_merge_sign(handle, child);

                if (child->len == 0) {
                    exit_with_error(handle, "0-length child token match caused infinite loop", offset);
                }

                offset = child->position + child->len;
                check_and_exit_on_fatal_parsing_error(handle, child, offset);
                continue;
            }

            // 3. If search_parent_end_token_last is true, check end token after trying nested tokens
            if (token_def->search_parent_end_token_last == true)
            {
                size_t end_match_len = 0;
                bool found_end = adv_regex_find_pattern_ctx(handle->regex_ctx, token_def->end_regex, (void **)handle->end_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &end_match_len, !handle->language->case_sensitivity, true);
                if (found_end)
                {
                    break;
                }
            }

            // 4. Skip unrecognized text if allowed
            if (token_def->other_text_inside) {
                offset += textparser_char_len(handle, offset);
            } else {
                exit_with_error(handle, "Unexpected token inside start-stop block!", offset);
            }
        }
    }

    offset = textparser_skip_whitespace(handle, offset);

    if (offset >= textparser_get_total_units(handle)) {
        if (token_def->type == TEXTPARSER_TOKEN_TYPE_START_STOP) {
            exit_with_error(handle, "offset >= total units count!", offset);
        } else {
            ret->len = offset - ret->position;
            goto exit;
        }
    }

    size_t end_len = 0;
    bool found_end = adv_regex_find_pattern_ctx(handle->regex_ctx, token_def->end_regex, (void **)handle->end_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, &token_end, &end_len, !handle->language->case_sensitivity, false);

    if (!found_end) {
        if (token_def->type == TEXTPARSER_TOKEN_TYPE_START_STOP) {
            LOGE("Can't find [%s] at %zu. Text: [%s]", token_def->end_regex, offset, handle->text_addr + textparser_get_byte_offset(handle, offset));
            exit_with_error(handle, "Can't find end of the token!", offset);
        } else {
            ret->len = offset - ret->position;
            goto exit;
        }
    }

    LOGV("TEXTPARSER_TOKEN_TYPE_START_(OPT)_STOP - Found [%s] at %zu", handle->language->tokens[ret->token_id].name, ret->position);
    ret->len = offset + token_end + end_len - ret->position;
    offset += token_end + end_len;

    if (offset != ret->position + ret->len) {
        exit_with_error(handle, "offset != ret->position + ret->len!", offset);
    }

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

static textparser_token_item *textparser_parse_token(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    if (handle == nullptr)
    {
        LOGF("handle == nullptr");
        return nullptr;
    }

    const textparser_language_definition *definition = handle->language;

    if (definition == nullptr) {
        LOGE("definition == nullptr");
        return nullptr;
    }

    if (token_id < TextParser_START || (size_t)token_id >= handle->token_count) {
        LOGE("token_id out of bounds");
        return nullptr;
    }

    if (parent_token_id < TextParser_END || (parent_token_id != TextParser_END && (size_t)parent_token_id >= handle->token_count)) {
        LOGE("parent_token_id out of bounds");
        return nullptr;
    }

    if (offset >= textparser_get_total_units(handle)) {
        LOGE("offset >= textparser_get_total_units(handle)");
        return nullptr;
    }

    if (handle->recursion_depth >= MAX_RECURSION_DEPTH) {
        LOGE("Maximum recursion depth exceeded during parsing!");
        handle->error = "Maximum recursion depth exceeded during parsing!";
        handle->error_offset = offset;
        return nullptr;
    }
    handle->recursion_depth++;

    const textparser_token *token_def = &definition->tokens[token_id];

    if (parent_token_id != TextParser_END) {
        LOGI("Searching for token type [%s] with parent token type [%s] at %zu",  definition->tokens[token_id].name, definition->tokens[parent_token_id].name, offset);
    } else {
        LOGI("Searching for token type [%s] at %zu",  definition->tokens[token_id].name, offset);
    }

    LOGV("-----------");

    // Check if current token has end token string, if so, search for it instead parent one!
    if (token_def->end_regex)
    {
        LOGI("Override parent_token_id with [%s] with end regex[%s]", definition->tokens[token_id].name, token_def->end_regex);
        parent_token_id = token_id;
    }

    offset = textparser_skip_whitespace(handle, offset);

    LOGV("id: %d - [%s]  at offset: %zu", token_id, token_def->name, offset);
    textparser_token_item *ret = nullptr;
    switch(token_def->type)
    {
        case TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY:             ret = parse_token_group_one_child_only(handle, token_id, parent_token_id, parent_start_stop, offset, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_GROUP:                            ret = parse_token_group(handle, token_id, parent_token_id, parent_start_stop, offset, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER: ret = parse_token_group_all_children_in_same_order(handle, token_id, parent_token_id, parent_start_stop, offset, parent_item, prev_sibling); break;
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

        if (!token_def->multi_line && textparser_has_newline(handle, ret->position, ret->len)) {
            exit_with_error(handle, "Token spans multiple lines but multi_line flag is not set!", ret->position);
        }

        if (token_def->must_have_one_child && textparser_get_token_children_count(ret) != 1) {
            exit_with_error(handle, "Token must have exactly one child token!", ret->position);
        }

        if (token_def->delete_if_only_one_child && textparser_get_token_children_count(ret) == 1) {
            textparser_token_item *only_child = ret->child;
            only_child->parent = ret->parent;
            ret = only_child;
        }
    }

exit:
    handle->recursion_depth--;
    return ret;
}

static int textparser_init_regex(struct textparser_handle *handle)
{
    int token_cnt = 0;

    if (handle == nullptr)
        return -1;

    if (handle->regex_ctx == nullptr) {
        handle->regex_ctx = adv_regex_context_create();
    }

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

    if (handle->regex_ctx) {
        adv_regex_context_free(handle->regex_ctx);
        handle->regex_ctx = nullptr;
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
        typedef struct json_string_pool_chunk {
            struct json_string_pool_chunk *next;
            size_t used;
            size_t capacity;
            char buffer[];
        } json_string_pool_chunk;

        typedef struct {
            json_string_pool_chunk *head;
        } json_string_pool;

        json_string_pool *pool = (json_string_pool *)definition->string_pool;
        if (pool) {
            json_string_pool_chunk *curr = pool->head;
            while (curr) {
                json_string_pool_chunk *next = curr->next;
                free(curr);
                curr = next;
            }
            free(pool);
        }
    }

    free(definition);
}
#include "textparser_compat.h"

int textparser_parse_compat(textparser_t handle, const textparser_language_definition *definition)
{
    if (handle == nullptr)
        return -1;

    if (definition == nullptr)
        return -1;

    if (handle->text_size >= MAX_PARSE_SIZE)
        return -1;

    // Reset error state
    handle->error = nullptr;
    handle->error_offset = 0;

    // Free any previously parsed token tree to prevent leaks and AST corruption
    free_arena(handle);
    handle->first_item = nullptr;

    textparser_token_item *prev_item = nullptr;
    size_t size = textparser_get_total_units(handle);
    size_t pos = 0;

    if (handle->language != definition)
    {
        textparser_free_regex(handle);
        handle->language = definition;
        if (textparser_init_regex(handle) != 0)
            return -1;
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

    while(pos < size) {
        pos = textparser_skip_whitespace(handle, pos);
        if (pos >= size)
            break;

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
                if (offset == 0)
                {
                    matched_token_id = token_id;
                    break;
                }
            }
        }

        if (matched_token_id != TextParser_END) {
            textparser_token_item *token_item = textparser_parse_token(handle, matched_token_id, TextParser_END, TEXTPARSER_SEARCH_END_TOKEN, pos, nullptr, prev_item);
            if (token_item == nullptr) {
                LOGE("token_item == nullptr");
                return -1;
            }

            if (handle->first_item == nullptr)
                handle->first_item = token_item;

            if (prev_item) {
                prev_item->next = token_item;
                token_item->prev = prev_item;
            }

            maybe_merge_sign(handle, token_item);

            if ((handle->error)||(token_item->len <= 0))
                return -1;

            pos = token_item->position + token_item->len;
            prev_item = token_item;
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
    }

    return 0;
}
