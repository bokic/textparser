#include <textparser.h>
#include <adv_regex.h>
#include <logger.h>
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

static int active_handle_count = 0;

#define TOKEN_NOT_FOUND -1

#define exit_with_error(error_text, offset)       \
    LOGE("Error: %s at %zu", error_text, offset); \
    if(handle) handle->error = error_text;        \
    if(handle) handle->error_offset = offset;     \
    goto exit;                                    \

#define check_and_exit_on_fatal_parsing_error(offset)                                         \
    if (handle->error) {                                                                      \
        LOGW("Fatal error detected(%s) at offset %zu. exiting..", handle->error, offset);     \
        goto exit;                                                                            \
    }                                                                                         \
    if (child->len == 0) {                                                                    \
        LOGW("child->len == 0 detected(%s) at offset %zu. exiting..", handle->error ? handle->error : "none", offset); \
        goto exit;                                                                            \
    }

enum textparser_bom {
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


static void free_item_tree(textparser_token_item *item)
{
    (void)item;
}

static size_t calculate_chunk_size(size_t filesize)
{
    size_t chunk_size = 4096; // 4KB minimum
    while (chunk_size <= filesize && chunk_size < 16777216) {
        chunk_size *= 2;
    }
    return chunk_size;
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
    size_t maxPos = 0;

    if (handle->text_format == TEXTPARSER_ENCODING_UNICODE || handle->text_format == TEXTPARSER_ENCODING_UTF_16)
    {
        maxPos = handle->text_size / sizeof(uint16_t);

        const uint16_t *text = (const uint16_t *)handle->text_addr;

        for (size_t c = pos; c < maxPos; c++)
        {
            uint16_t ch = text[c];

            if ((ch != ' ') && (ch != '\t') && (ch != '\n') && (ch != '\r'))
            {
                return c;
            }
        }
    }
    else if (handle->text_format == TEXTPARSER_ENCODING_UTF_32)
    {
        maxPos = handle->text_size / sizeof(uint32_t);

        const uint32_t *text = (const uint32_t *)handle->text_addr;

        for (size_t c = pos; c < maxPos; c++)
        {
            uint32_t ch = text[c];

            if ((ch != ' ') && (ch != '\t') && (ch != '\n') && (ch != '\r'))
            {
                return c;
            }
        }
    }
    else
    {
        maxPos = handle->text_size;

        const char *text = handle->text_addr;

        for (size_t c = pos; c < maxPos; c++)
        {
            char ch = text[c];

            if ((ch != ' ') && (ch != '\t') && (ch != '\n') && (ch != '\r'))
            {
                return c;
            }
        }
    }

    return maxPos;
}
static void adjust_search_order(const struct textparser_handle *handle, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling, const int *original_list, int *adjusted_list)
{
    int count = 0;
    while (original_list[count] != TextParser_END) {
        adjusted_list[count] = original_list[count];
        count++;
    }
    adjusted_list[count] = TextParser_END;

    const textparser_language_definition *definition = handle->language;
    if (definition->sign_ambiguity_fix) {
        int number_idx = -1;
        int operator_idx = -1;
        for (int j = 0; adjusted_list[j] != TextParser_END; j++) {
            if (adjusted_list[j] == definition->token_number_id) {
                number_idx = j;
            } else if (adjusted_list[j] == definition->token_operator_id) {
                operator_idx = j;
            }
        }

        if (number_idx != -1 && operator_idx != -1) {
            bool prev_is_operator_or_first = (prev_sibling == nullptr) || (prev_sibling == parent_item) || (prev_sibling->token_id == definition->token_operator_id);

            if (prev_is_operator_or_first) {
                // Search number before operator
                if (number_idx > operator_idx) {
                    int temp = adjusted_list[number_idx];
                    adjusted_list[number_idx] = adjusted_list[operator_idx];
                    adjusted_list[operator_idx] = temp;
                }
            } else {
                // Search operator before number
                if (operator_idx > number_idx) {
                    int temp = adjusted_list[number_idx];
                    adjusted_list[number_idx] = adjusted_list[operator_idx];
                    adjusted_list[operator_idx] = temp;
                }
            }
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
            if (token->nested_tokens)
            {
                int nested_count = 0;
                while (token->nested_tokens[nested_count] != TextParser_END) {
                    nested_count++;
                }

                ssize_t closest_child_pos = SSIZE_MAX;
                {
                    int adjusted_list[nested_count + 1];
                    adjust_search_order(handle, parent_item, prev_sibling, token->nested_tokens, adjusted_list);

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
            break;
        case TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER:
            if (token->nested_tokens) {
                LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER");
                return textparser_find_token(handle, token->nested_tokens[0], pos, other_text_inside, parent_item, prev_sibling);
            }
            LOGE("token->nested_tokens = nullptr for TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER");
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
    if (!token_def->nested_tokens) {
        exit_with_error("group_one_child token type nested_tokens list is empty!", offset);
    }

    ret = allocate_token(handle);
    if (ret == nullptr) {
        return nullptr;
    }

    offset = textparser_skip_whitespace(handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    LOGV("id: %d - [%s]  at offset: %zu", token_id, token_def->name, offset);

    int count = 0;
    while (token_def->nested_tokens[count] != TextParser_END) {
        count++;
    }

    size_t closest = SIZE_MAX;
    int current_token_id = TextParser_END;
    {
        int adjusted_list[count + 1];
        adjust_search_order(handle, parent_item, prev_sibling, token_def->nested_tokens, adjusted_list);

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
        exit_with_error("Search for group_one_child token type failed. Can't find one child.", offset);
    }

    child = textparser_parse_token(handle, current_token_id, parent_token_id, parent_start_stop, offset, ret, prev_sibling);
    if (child == nullptr) {
        exit_with_error("Search for group_one_child token type failed. Child token parsing failed.", offset);
    }
    child->parent = ret;
    LOGV("TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY - Found [%s] at %zd", handle->language->tokens[child->token_id].name, child->position);
    ret->position = child->position;
    ret->len = child->len;
    ret->child = child;
    check_and_exit_on_fatal_parsing_error(offset);

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
    if (!token_def->nested_tokens) {
        exit_with_error("nested_tokens list is empty!", offset);
    }

    ret = allocate_token(handle);
    if (ret == nullptr) {
        return nullptr;
    }

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
            exit_with_error("Search for group token type failed. Can't find any child.", offset);
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
                exit_with_error("Unknown parent_start_stop value!", offset);
            }
        }

        if (parent_regex_pattern) {
            size_t parent_match_len = 0;
            bool found_parent_token = adv_regex_find_pattern(parent_regex_pattern, parent_regex_compiled_ptr, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &parent_match_len, !handle->language->case_sensitivity, true);

            if (found_parent_token)
            {
                ret->len = offset - ret->position;
                break;
            }
        }

        current_token_id = TextParser_END;

        int count = 0;
        while (token_def->nested_tokens[count] != TextParser_END) {
            count++;
        }
        {
            int adjusted_list[count + 1];
            adjust_search_order(handle, parent_item, current_prev, token_def->nested_tokens, adjusted_list);

            for (int c = 0; adjusted_list[c] != TextParser_END; c++)
            {
                ssize_t current_closest = textparser_find_token(handle, adjusted_list[c], offset, token_def->other_text_inside, parent_item, current_prev);
                if (current_closest == 0)
                {
                    current_token_id = adjusted_list[c];
                    break;
                }
            }
        }

        if (current_token_id != TextParser_END)
        {
            if (child == nullptr) {
                child = textparser_parse_token(handle, current_token_id, parent_token_id, parent_start_stop, offset, ret, current_prev);
                if (child == nullptr) {
                    exit_with_error("Parsing child token failed", offset);
                }
                child->parent = ret;
                ret->child = child;
                check_and_exit_on_fatal_parsing_error(offset);
            } else {
                textparser_token_item *next_child = textparser_parse_token(handle, current_token_id, parent_token_id, parent_start_stop, offset, ret, current_prev);
                if (next_child == nullptr) {
                    exit_with_error("Parsing child token failed", offset);
                }
                next_child->parent = ret;
                next_child->prev = child;
                child->next = next_child;
                child = next_child;
                check_and_exit_on_fatal_parsing_error(offset);
            }

            if (child->len == 0) {
                exit_with_error("0-length child token match caused infinite loop", offset);
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
                exit_with_error("Unrecognized token inside group", offset);
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
        exit_with_error("nested_tokens list is empty!", offset);
    }

    int nested_count = 0;
    while(token_def->nested_tokens[nested_count] != TextParser_END) nested_count++;

    if (nested_count != 3) {
         exit_with_error("GroupAllChildrenInSameOrder should have exactly 3 nested tokens", offset);
    }

    int start_token_id = token_def->nested_tokens[0];
    int inner_token_id = token_def->nested_tokens[1];
    int end_token_id   = token_def->nested_tokens[2];

    ret = allocate_token(handle);
    if (ret == nullptr) {
        return nullptr;
    }

    offset = textparser_skip_whitespace(handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    ssize_t start_pos = textparser_find_token(handle, start_token_id, offset, definition->other_text_inside, parent_item, prev_sibling);
    if (start_pos != 0) {
        exit_with_error("Expected start token!", offset);
    }

    child = textparser_parse_token(handle, start_token_id, parent_token_id, parent_start_stop, offset, ret, prev_sibling);
    if (child) child->parent = ret;
    check_and_exit_on_fatal_parsing_error(offset);

    ret->child = child;
    last_child = child;

    offset = child->position + child->len;

    while(1)
    {
        offset = textparser_skip_whitespace(handle, offset);
        if (offset >= textparser_get_total_units(handle)) {
            exit_with_error("Expected end token, reached end of text!", offset);
        }

        ssize_t end_pos   = textparser_find_token(handle, end_token_id,   offset, definition->other_text_inside, ret, last_child);
        if (end_pos == 0) {
            break;
        }

        ssize_t inner_pos = textparser_find_token(handle, inner_token_id, offset, definition->other_text_inside, ret, last_child);
        if (inner_pos == 0) {
            child = textparser_parse_token(handle, inner_token_id, end_token_id, TEXTPARSER_SEARCH_START_TOKEN, offset, ret, last_child);
            if (child) {
                child->parent = ret;
                child->prev = last_child;
            }
            last_child->next = child;
            last_child = child;
            check_and_exit_on_fatal_parsing_error(offset);

            if (child->len == 0) {
                exit_with_error("0-length child token match caused infinite loop", offset);
            }

            offset = child->position + child->len;
            continue;
        }

        if (definition->other_text_inside) {
            offset += textparser_char_len(handle, offset);
        } else {
            exit_with_error("Expected inner or end token!", offset);
        }
    }

    offset = textparser_skip_whitespace(handle, offset);
    child = textparser_parse_token(handle, end_token_id, parent_token_id, parent_start_stop, offset, ret, last_child);
    if (child == nullptr) {
        exit_with_error("Parsing end token failed", offset);
    }
    child->parent = ret;
    child->prev = last_child;
    last_child->next = child;
    check_and_exit_on_fatal_parsing_error(offset);

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
        exit_with_error("offset >= total units count!", offset);
    }

    ret = allocate_token(handle);
    if (ret == nullptr) {
        exit_with_error("Can't allocate memory!", offset);
    }

    offset = textparser_skip_whitespace(handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    size_t len = 0;
    if (!adv_regex_find_pattern(token_def->start_regex, (void **)handle->start_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &len, !handle->language->case_sensitivity, true)) {
        exit_with_error("Can't find start of the token!", offset);
    }

    LOGV("TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN - Found [%s] at %zd", handle->language->tokens[ret->token_id].name, ret->position);
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
        exit_with_error("handle == nullptr!", offset);
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
        exit_with_error("offset >= total units count!", offset);
    }

    ret = allocate_token(handle);
    if (ret == nullptr) {
        exit_with_error("Can't allocate memory!", offset);
    }

    offset = textparser_skip_whitespace(handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    // Search for start token
    if (!adv_regex_find_pattern(token_def->start_regex, (void **)handle->start_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &len, !handle->language->case_sensitivity, true)) {
        exit_with_error("Can't find start of the token!", offset);
    }

    ret->position = offset;
    offset = ret->position + len;
    ret->len = len; // Temporarily set len to start token length so it can be used for prev_sibling context

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_START, handle->user_data);
    }

    if (offset > textparser_get_total_units(handle)) {
        exit_with_error("offset >= total units count!", offset);
    }

    if (offset == textparser_get_total_units(handle)) {
        exit_with_error("reached end of text!", offset);
    }

    if (token_def->nested_tokens)
    {
        textparser_token_item *last_child = nullptr;
        const int *nested_tokens = definition->tokens[token_id].nested_tokens;

        while (1) {
            offset = textparser_skip_whitespace(handle, offset);

            if (offset >= textparser_get_total_units(handle))
            {
                exit_with_error("Reached end of text before finding end token!", offset);
            }

            // 1. If search_parent_end_token_last is false, check end token first
            if (token_def->search_parent_end_token_last == false)
            {
                size_t end_match_len = 0;
                bool found_end = adv_regex_find_pattern(token_def->end_regex, (void **)handle->end_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &end_match_len, !handle->language->case_sensitivity, true);
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
                adjust_search_order(handle, ret, current_prev, nested_tokens, adjusted_list);

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
                child = textparser_parse_token(handle, child_token_id, token_id, TEXTPARSER_SEARCH_END_TOKEN, offset, ret, current_prev);
                if (child == nullptr) {
                    exit_with_error("Parsing nested child token failed", offset);
                }
                child->parent = ret;
                if (last_child) {
                    child->prev = last_child;
                }
                if (ret->child == nullptr)
                    ret->child = child;

                if (last_child)
                    last_child->next = child;
                last_child = child;

                if (child->len == 0) {
                    exit_with_error("0-length child token match caused infinite loop", offset);
                }

                offset += child->len;
                check_and_exit_on_fatal_parsing_error(offset);
                continue;
            }

            // 3. If search_parent_end_token_last is true, check end token after trying nested tokens
            if (token_def->search_parent_end_token_last == true)
            {
                size_t end_match_len = 0;
                bool found_end = adv_regex_find_pattern(token_def->end_regex, (void **)handle->end_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &end_match_len, !handle->language->case_sensitivity, true);
                if (found_end)
                {
                    break;
                }
            }

            // 4. Skip unrecognized text if allowed
            if (token_def->other_text_inside) {
                offset += textparser_char_len(handle, offset);
            } else {
                exit_with_error("Unexpected token inside start-stop block!", offset);
            }
        }
    }

    offset = textparser_skip_whitespace(handle, offset);

    if (offset >= textparser_get_total_units(handle)) {
        if (token_def->type == TEXTPARSER_TOKEN_TYPE_START_STOP) {
            exit_with_error("offset >= total units count!", offset);
        } else {
            ret->len = offset - ret->position;
            goto exit;
        }
    }

    size_t end_len = 0;
    bool found_end = adv_regex_find_pattern(token_def->end_regex, (void **)handle->end_regex + token_id, handle->text_format, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, &token_end, &end_len, !handle->language->case_sensitivity, false);

    if (!found_end) {
        if (token_def->type == TEXTPARSER_TOKEN_TYPE_START_STOP) {
            LOGE("Can't find [%s] at %zd. Text: [%s]", token_def->end_regex, offset, handle->text_addr + textparser_get_byte_offset(handle, offset));
            exit_with_error("Can't find end of the token!", offset);
        } else {
            ret->len = offset - ret->position;
            goto exit;
        }
    }

    LOGV("TEXTPARSER_TOKEN_TYPE_START_(OPT)_STOP - Found [%s] at %zd", handle->language->tokens[ret->token_id].name, ret->position);
    ret->len = offset + token_end + end_len - ret->position;
    offset += token_end + end_len;

    if (offset != ret->position + ret->len) {
        exit_with_error("offset != ret->position + ret->len!", offset);
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
    }

    return ret;
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

            for(int c = 0; c < token_cnt; c++)
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

            for(int c = 0; c < token_cnt; c++)
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

    if (definition->default_file_extensions) {
        int c = 0;
        while(definition->default_file_extensions[c]) {
            free((void *)definition->default_file_extensions[c]);
            c++;
        }

        free((void *)definition->default_file_extensions);
    }

    if (definition->name) {
        free((void *)definition->name);
    }

    if (definition->empty_segment_language) {
        free((void *)definition->empty_segment_language);
    }

    if (definition->starts_with) {
        free((void *)definition->starts_with);
    }

    if (definition->tokens) {
        int c = 0;
        while(definition->tokens[c].name != nullptr) {
            textparser_token *token = &definition->tokens[c];

            if (token->name) {
                free((void *)token->name);
            }
            if (token->start_regex) {
                free((void *)token->start_regex);
            }
            if (token->end_regex) {
                free((void *)token->end_regex);
            }
            if (token->nested_tokens) {
                free((void *)token->nested_tokens);
            }

            c++;
        }
        free(definition->tokens);
    }

    free(definition);
}

int textparser_openfile(const char *pathname, int default_text_format, textparser_t *handle)
{
    struct textparser_handle local_hnd;
    int err = 0;

    memset(&local_hnd, 0, sizeof(local_hnd));

    local_hnd.mmap_size = SIZE_MAX;

    local_hnd.mmap_addr = os_map(pathname, &local_hnd.mmap_size);
    if ((local_hnd.mmap_addr == nullptr)&&(local_hnd.mmap_size > 0)) {
        err = 1;
        goto err;
    }

    local_hnd.text_addr = local_hnd.mmap_addr;
    local_hnd.text_size = local_hnd.mmap_size;

    if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x00')&&(local_hnd.text_addr[1] == '\x00')&&(local_hnd.text_addr[2] == '\xfe')&&(local_hnd.text_addr[3] == '\xff')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_32_BE;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\xff')&&(local_hnd.text_addr[1] == '\xfe')&&(local_hnd.text_addr[2] == '\x00')&&(local_hnd.text_addr[3] == '\x00')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_32_LE;
    } else if ((local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\xef')&&(local_hnd.text_addr[1] == '\xbb')&&(local_hnd.text_addr[2] == '\xbf')) {
        local_hnd.text_addr += 3;
        local_hnd.text_size -= 3;
        local_hnd.bom = BOM_UTF_8;
    } else if ((local_hnd.text_size >= 2)&&(local_hnd.text_addr[0] == '\xfe')&&(local_hnd.text_addr[1] == '\xff')) {
        local_hnd.text_addr += 2;
        local_hnd.text_size -= 2;
        local_hnd.bom = BOM_UTF_16_BE;
    } else if ((local_hnd.text_size >= 2)&&(local_hnd.text_addr[0] == '\xff')&&(local_hnd.text_addr[1] == '\xfe')) {
        local_hnd.text_addr += 2;
        local_hnd.text_size -= 2;
        local_hnd.bom = BOM_UTF_16_LE;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x38')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_7_1;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x39')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_7_2;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x2b')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_7_3;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x2f')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_7_4;
    } else if ((local_hnd.text_size >= 5)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x38')&&(local_hnd.text_addr[4] == '\x2d')) {
        local_hnd.text_addr += 5;
        local_hnd.text_size -= 5;
        local_hnd.bom = BOM_UTF_7_5;
    } else if ((local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\xf7')&&(local_hnd.text_addr[1] == '\x64')&&(local_hnd.text_addr[2] == '\x4c')) {
        local_hnd.text_addr += 3;
        local_hnd.text_size -= 3;
        local_hnd.bom = BOM_UTF_1;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\xdd')&&(local_hnd.text_addr[1] == '\x73')&&(local_hnd.text_addr[2] == '\x66')&&(local_hnd.text_addr[3] == '\x73')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_EBCDIC;
    } else if ((local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\x0e')&&(local_hnd.text_addr[1] == '\xfe')&&(local_hnd.text_addr[2] == '\xff')) {
        local_hnd.text_addr += 3;
        local_hnd.text_size -= 3;
        local_hnd.bom = BOM_UTF_SCSU;
    } else if ((local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\xfb')&&(local_hnd.text_addr[1] == '\xee')&&(local_hnd.text_addr[2] == '\x28')) {
        local_hnd.text_addr += 3;
        local_hnd.text_size -= 3;
        local_hnd.bom = BOM_UTF_BOCU1;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x84')&&(local_hnd.text_addr[1] == '\x31')&&(local_hnd.text_addr[2] == '\x95')&&(local_hnd.text_addr[3] == '\x33')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_GB_18030;
    } else {
        local_hnd.bom = NO_BOM;
    }

    local_hnd.text_format = (enum textparser_encoding)default_text_format;

    switch(local_hnd.bom)
    {
        case NO_BOM:
            break;
        case BOM_UTF_8:
            local_hnd.text_format = TEXTPARSER_ENCODING_UTF_8;
            break;
        case BOM_UTF_16_LE:
            local_hnd.text_format = TEXTPARSER_ENCODING_UTF_16;
            break;
        case BOM_UTF_32_LE:
            local_hnd.text_format = TEXTPARSER_ENCODING_UTF_32;
            break;
        default:
            err = 5;
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
        err = 7;
        goto err;
    }

    *handle = malloc(sizeof(struct textparser_handle));
    if (*handle == nullptr) {
        err = 6;
        goto err;
    }
    memcpy(*handle, &local_hnd, sizeof(struct textparser_handle));

    active_handle_count++;
    return 0;

err:
    if (local_hnd.mmap_addr) {
        os_unmap(local_hnd.mmap_addr, local_hnd.mmap_size);
    }

    return err;
}


int textparser_openmem(const char *text, int len, int text_format, textparser_t *handle)
{
    if (len < 0 || handle == nullptr || text == nullptr) {
        return -1;
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

    *handle = (textparser_t)ret;
    active_handle_count++;

    return 0;
}

void textparser_close(textparser_t handle)
{
    textparser_token_item *item = nullptr;
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

    item = handle->first_item;
    if (item)
    {
        free_item_tree(item);
        item = nullptr;
    }

    free_arena(handle);

    if (handle->lines) {
        free(handle->lines);
        handle->lines = nullptr;
    }

    free(handle);
    handle = nullptr;

    active_handle_count--;
    if (active_handle_count == 0) {
        adv_regex_cleanup();
    }
}

void textparser_cleanup(textparser_t *handle)
{
    if (handle)
    {
        textparser_close(*handle);
        *handle = nullptr;
    }
}

int textparser_parse(textparser_t handle, const textparser_language_definition *definition)
{
    if (handle == nullptr)
        return -1;

    if (definition == nullptr)
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
        textparser_init_regex(handle);
    }

    while(pos < size) {
        pos = textparser_skip_whitespace(handle, pos);
        if (pos >= size)
            break;

        int matched_token_id = TextParser_END;

        int count = 0;
        while (definition->starts_with[count] != TextParser_END) {
            count++;
        }
        {
            int adjusted_list[count + 1];
            adjust_search_order(handle, nullptr, prev_item, definition->starts_with, adjusted_list);

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

            if ((handle->error)||(token_item->len <= 0))
                return -1;

            pos = token_item->position + token_item->len;
            prev_item = token_item;
        } else {
            if (definition->other_text_inside) {
                pos += textparser_char_len(handle, pos);
            } else {
                break;
            }
        }
    }

    return 0;
}

const char *textparser_parse_error(textparser_t handle)
{
    return handle->error;
}

size_t textparser_parse_error_position(textparser_t handle)
{
    return handle->error_offset;
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

char *textparser_get_token_text(const textparser_t handle, const textparser_token_item *item)
{
    char *ret = nullptr;

    if ((handle == nullptr)||(item == nullptr)||(item->len <= 0))
        return nullptr;

    if (handle->text_format == TEXTPARSER_ENCODING_UNICODE || handle->text_format == TEXTPARSER_ENCODING_UTF_16) {
        ret = malloc(item->len + 1);
        if (ret) {
            const uint16_t *src = (const uint16_t *)(handle->text_addr + textparser_get_byte_offset(handle, item->position));
            for (size_t i = 0; i < item->len; i++) {
                ret[i] = (src[i] < 256) ? (char)src[i] : '?';
            }
            ret[item->len] = '\0';
        }
    } else if (handle->text_format == TEXTPARSER_ENCODING_UTF_32) {
        ret = malloc(item->len + 1);
        if (ret) {
            const uint32_t *src = (const uint32_t *)(handle->text_addr + textparser_get_byte_offset(handle, item->position));
            for (size_t i = 0; i < item->len; i++) {
                ret[i] = (src[i] < 256) ? (char)src[i] : '?';
            }
            ret[item->len] = '\0';
        }
    } else {
        size_t byte_len = textparser_get_byte_len(handle, item->len);
        ret = malloc(byte_len + 1);
        if (ret) {
            memcpy(ret, handle->text_addr + textparser_get_byte_offset(handle, item->position), byte_len);
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

    uint16_t *ret = malloc((item->len + 1) * sizeof(uint16_t));
    if (ret) {
        const uint16_t *src = (const uint16_t *)(handle->text_addr + textparser_get_byte_offset(handle, item->position));
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

    uint32_t *ret = malloc((item->len + 1) * sizeof(uint32_t));
    if (ret) {
        const uint32_t *src = (const uint32_t *)(handle->text_addr + textparser_get_byte_offset(handle, item->position));
        memcpy(ret, src, item->len * sizeof(uint32_t));
        ret[item->len] = 0;
    }
    return ret;
}

const textparser_language_definition *textparser_get_language(const textparser_t handle)
{
    if (handle == nullptr)
        return nullptr;

    return handle->language;
}

size_t textparser_get_token_children_count(const textparser_token_item *token)
{
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

    if (language == nullptr)
        return nullptr;

    if (token == nullptr)
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

    return token->position;
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

static void textparser_parse_state_recursively_fill(const textparser_token_item *token, const textparser_token_item **state)
{
    if (token == nullptr)
    {
        return;
    }

    size_t pos = token->position;
    size_t len = token->len;

    for(size_t c = 0; c < len; c++)
    {
        state[pos + c] = token;
    }

    textparser_parse_state_recursively_fill(token->child, state);
    textparser_parse_state_recursively_fill(token->next, state);
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
        ret->len = (int)size;
        memset(ret->state, 0, allocated);

        textparser_parse_state_recursively_fill(handle->first_item, ret->state);
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

    size_t count = 0;
    size_t text_size = handle->text_size;
    const char *text_addr = handle->text_addr;

    if (text_addr == nullptr || text_size == 0) {
        return 0;
    }

    switch (handle->text_format)
    {
    case TEXTPARSER_ENCODING_LATIN1:
    case TEXTPARSER_ENCODING_UTF_8:
        for (size_t ch = 0; ch < text_size; ch++) {
            if (text_addr[ch] == '\n') {
                count++;
            }
        }
        break;
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16: {
        const uint16_t *text16 = (const uint16_t *)text_addr;
        size_t units16 = text_size / sizeof(uint16_t);
        for (size_t ch = 0; ch < units16; ch++) {
            if (text16[ch] == '\n') {
                count++;
            }
        }
        break;
    }
    case TEXTPARSER_ENCODING_UTF_32: {
        const uint32_t *text32 = (const uint32_t *)text_addr;
        size_t units32 = text_size / sizeof(uint32_t);
        for (size_t ch = 0; ch < units32; ch++) {
            if (text32[ch] == '\n') {
                count++;
            }
        }
        break;
    }
    default:
        return -1;
    }

    if (count == 0) {
        return 0;
    }

    handle->lines = malloc(sizeof(size_t) * count);
    if (handle->lines == nullptr) {
        return -1;
    }
    handle->no_lines = count;

    size_t cur_line_pos = 0;

    switch (handle->text_format)
    {
    case TEXTPARSER_ENCODING_LATIN1:
    case TEXTPARSER_ENCODING_UTF_8:
        for (size_t ch = 0; ch < text_size; ch++) {
            if (text_addr[ch] == '\n') {
                handle->lines[cur_line_pos++] = ch;
            }
        }
        break;
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16: {
        const uint16_t *text16 = (const uint16_t *)text_addr;
        size_t units16 = text_size / sizeof(uint16_t);
        for (size_t ch = 0; ch < units16; ch++) {
            if (text16[ch] == '\n') {
                handle->lines[cur_line_pos++] = ch;
            }
        }
        break;
    }
    case TEXTPARSER_ENCODING_UTF_32: {
        const uint32_t *text32 = (const uint32_t *)text_addr;
        size_t units32 = text_size / sizeof(uint32_t);
        for (size_t ch = 0; ch < units32; ch++) {
            if (text32[ch] == '\n') {
                handle->lines[cur_line_pos++] = ch;
            }
        }
        break;
    }
    default:
        break;
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
