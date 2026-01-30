#include <textparser.h>
#include <adv_regex.h>
#include <logger.h>
#include <os.h>

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>

#ifndef SSIZE_MAX
#define SSIZE_MAX ((ssize_t)((((size_t)-1) << 1) >> 1))
#endif

#define MAX_PARSE_SIZE (16 * 1024 * 1024)

#define TOKEN_NOT_FOUND -1

#define exit_with_error(error_text, offset)                    \
    LOGE(error_text ". Error: %s at %zu", error_text, offset); \
    int_handle->error = error_text;                            \
    int_handle->error_offset = offset;                         \
    goto exit;

#define check_and_exit_on_fatal_parsing_error(offset)                                      \
    if (int_handle->error) {                                                               \
        LOGW("Fatal error detected(%s) at offset %zu. exiting..", ret->error, offset);     \
        goto exit;                                                                         \
    }                                                                                      \
    if (child->len == 0) {                                                                 \
        LOGW("child->len == 0 detected(%s) at offset %zu. exiting..", ret->error, offset); \
        goto exit;                                                                         \
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

typedef struct {
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
    const char *text_addr;
    size_t text_size;
    size_t no_lines;
    size_t lines[];
} textparser_handle;

static void free_item_tree(textparser_token_item *item)
{
    textparser_token_item *next = nullptr;

    if (item == nullptr) {
        return;
    }

    if (item->child)
    {
        free_item_tree(item->child);
        item->child = nullptr;
    }

    next = item->next;
    if (next)
    {
        free_item_tree(next);

        item->next = nullptr;
    }

    free(item);
}

static size_t textparser_skip_whitespace(const textparser_handle *int_handle, size_t pos)
{
    size_t maxPos = 0;

    if (int_handle->text_format == TEXTPARSER_ENCODING_UNICODE)
    {
        maxPos = int_handle->text_size / 2;

        const uint16_t *text = (const uint16_t *)int_handle->text_addr;

        for (size_t c = pos; c < maxPos; c++)
        {
            uint8_t ch = text[c];

            if ((ch != ' ') && (ch != '\t') && (ch != '\n') && (ch != '\r'))
            {
                return c;
            }
        }
    }
    else
    {
        maxPos = int_handle->text_size;

        const char *text = int_handle->text_addr;

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

static ssize_t textparser_find_token(const textparser_handle *int_handle, int token_id, size_t pos, bool other_text_inside)
{
    if (pos >= int_handle->text_size) {
        return TOKEN_NOT_FOUND;
    }

    const textparser_language_definition *definition = nullptr;
    const textparser_token *token = nullptr;
    const char *text = nullptr;
    size_t found_at = 0;
    size_t len = 0;

    definition = int_handle->language;
    token = &definition->tokens[token_id];
    text = int_handle->text_addr + pos;
    len = int_handle->text_size - pos;

    LOGV("textparser_find_token token->type [%s] pos %zu", token->name, pos);

    switch(token->type)
    {
        case TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY");
        case TEXTPARSER_TOKEN_TYPE_GROUP:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_GROUP");
            if (token->nested_tokens)
            {
                ssize_t closest_child_pos = SSIZE_MAX;
                for(int c = 0; token->nested_tokens[c] != TextParser_END; c++)
                {
                    ssize_t child_token_pos = textparser_find_token(int_handle, token->nested_tokens[c], pos, token->other_text_inside);
                    if (child_token_pos == TOKEN_NOT_FOUND) continue;
                    if (child_token_pos == 0) {
                        return child_token_pos;
                    }

                    if (child_token_pos < closest_child_pos) {
                        closest_child_pos = child_token_pos;
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
                return textparser_find_token(int_handle, token->nested_tokens[0], pos, other_text_inside);
            }
            LOGE("token->nested_tokens = nullptr for TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER");
            break;
        case TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN");
        case TEXTPARSER_TOKEN_TYPE_START_STOP:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_START_STOP");
        case TEXTPARSER_TOKEN_TYPE_START_OPT_STOP:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_START_OPT_STOP");
        case TEXTPARSER_TOKEN_TYPE_DUAL_START_AND_STOP:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_DUAL_START_AND_STOP");
            if (adv_regex_find_pattern(token->start_regex, (void **)int_handle->start_regex + token_id, int_handle->text_format, text, len, &found_at, nullptr, !int_handle->language->case_sensitivity, !other_text_inside)) {
                LOGI("found_at token type: [%s] at %zu",  int_handle->language->tokens[token_id].name, pos + found_at);
                return found_at;
            }
            break;
        default:
            LOGF("textparser_find_token() - unknown!!!!!");
            break;
    }

    return TOKEN_NOT_FOUND;
}

static  void parse_token_error_error(textparser_handle *int_handle, const char *error, size_t offset)
{
    if (int_handle == nullptr)
    {
        LOGF("int_handle == nullptr");
        return;
    }

    LOGF("parse_token_error_error(). error %s.", error);

    int_handle->error_offset = offset;
    int_handle->error = error;
}

static textparser_token_item *textparser_parse_token(textparser_handle *int_handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset);

static textparser_token_item *parse_token_group_one_child_only(textparser_handle *int_handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset)
{
    textparser_token_item *ret = nullptr;

    if (int_handle == nullptr)
    {
        LOGF("int_handle == nullptr");
        return nullptr;
    }

    const textparser_language_definition *definition = int_handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];
    textparser_token_item *child = nullptr;

    LOGV("enter TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY");
    if (!token_def->nested_tokens) {
        exit_with_error("group_one_child token type nested_tokens list is empty!", offset);
    }

    ret = malloc(sizeof(textparser_token_item));
    if (ret == nullptr) {
        int_handle->error = "Can't allocate memory!";
        return nullptr;
    }

    memset(ret, 0, sizeof(textparser_token_item));

    offset = textparser_skip_whitespace(int_handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    LOGV("id: %d - [%s]  at offset: %zu", token_id, token_def->name, offset);

    size_t closest = SIZE_MAX;
    int current_token_id = TextParser_END;
    for (int c = 0; token_def->nested_tokens[c] != TextParser_END; c++)
    {
        ssize_t current_closest = textparser_find_token(int_handle, token_def->nested_tokens[c], offset, token_def->other_text_inside);
        if ((current_closest >= 0)&&(current_closest < closest))
        {
            closest = current_closest;
            current_token_id = token_def->nested_tokens[c];
        }
    }

    if (current_token_id == TextParser_END)
    {
        exit_with_error("Search for group_one_child token type failed. Can't find one child.", offset);
    }

    child = textparser_parse_token(int_handle, current_token_id, parent_token_id, parent_start_stop, offset);
    LOGV("TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY - Found [%s] at %zd", int_handle->language->tokens[child->token_id].name, child->position);
    ret->position = child->position;
    ret->len = child->len;
    ret->child = child;
    check_and_exit_on_fatal_parsing_error(offset);

exit:
    return ret;
}

static textparser_token_item *parse_token_group(textparser_handle *int_handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset)
{
    textparser_token_item *ret = nullptr;

    if (int_handle == nullptr)
    {
        LOGF("int_handle == nullptr");
        return nullptr;
    }

    const textparser_language_definition *definition = int_handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];
    textparser_token_item *child = nullptr;

    LOGV("enter TEXTPARSER_TOKEN_TYPE_GROUP");
    if (!token_def->nested_tokens) {
        exit_with_error("nested_tokens list is empty!", offset);
    }

    ret = malloc(sizeof(textparser_token_item));
    if (ret == nullptr) {
        int_handle->error = "Can't allocate memory!";
        return nullptr;
    }

    memset(ret, 0, sizeof(textparser_token_item));

    offset = textparser_skip_whitespace(int_handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    LOGV("id: %d - [%s]  at offset: %zu", token_id, token_def->name, offset);

    size_t closest = SIZE_MAX;
    size_t closest_parent = SIZE_MAX;
    int current_token_id;
    while(1) {
        offset = textparser_skip_whitespace(int_handle, offset);
        closest = SIZE_MAX;
        current_token_id = TextParser_END;
        for (int c = 0; token_def->nested_tokens[c] != TextParser_END; c++)
        {
            ssize_t current_closest = textparser_find_token(int_handle, token_def->nested_tokens[c], offset, token_def->other_text_inside);
            if ((current_closest >= 0)&&(current_closest < closest))
            {
                closest = current_closest;
                current_token_id = token_def->nested_tokens[c];
            }
        }

        const char *parent_regex_pattern = nullptr;
        void **parent_regex_compiled_ptr = nullptr;

        switch(parent_start_stop)
        {
        case TEXTPARSER_SEARCH_END_TOKEN:
            parent_regex_pattern = definition->tokens[parent_token_id].end_regex;
            parent_regex_compiled_ptr = (void **)int_handle->end_regex + parent_token_id;
            break;
        case TEXTPARSER_SEARCH_START_TOKEN:
            parent_regex_pattern = definition->tokens[parent_token_id].start_regex;
            parent_regex_compiled_ptr = (void **)int_handle->start_regex + parent_token_id;
            break;
        default:
            exit_with_error("Unknown parent_start_stop value!", offset);
        }

        if (parent_regex_pattern) {
            bool found_parent_token = adv_regex_find_pattern(parent_regex_pattern, parent_regex_compiled_ptr, int_handle->text_format, int_handle->text_addr + offset, int_handle->text_size - offset, &closest_parent, nullptr, !int_handle->language->case_sensitivity, !token_def->other_text_inside);

            if ((found_parent_token)&&(closest_parent <= closest))
            {
                ret->len = offset + closest_parent - ret->position;
                break;
            }
        }

        if (closest != SIZE_MAX) {
             LOGI("Found child [%s] at offset %zu (child offset from current %zu). Parsing...", definition->tokens[current_token_id].name, offset, closest);
        } else {
             LOGI("No child found at offset %zu.", offset);
        }

        if (current_token_id == TextParser_END)
        {
            if (child)
            {
                if (closest_parent == SIZE_MAX) {
                    ret->len = int_handle->text_size - ret->position;
                } else {
                    ret->len = offset + closest_parent - ret->position;
                }
                goto exit;
            }

            exit_with_error("Search for group_one_child token type failed. Can't find one child.", offset);
        }

        offset += closest;

        if (child == nullptr) {
            child = textparser_parse_token(int_handle, current_token_id, parent_token_id, parent_start_stop, offset);
            ret->child = child;
            check_and_exit_on_fatal_parsing_error(closest);
        } else {
            child->next = textparser_parse_token(int_handle, current_token_id, parent_token_id, parent_start_stop, offset);
            child = child->next;
            check_and_exit_on_fatal_parsing_error(closest);
        }

        offset = child->position + child->len;
        ret->len = child->position + child->len - ret->position;
    }

exit:
    return ret;
}

static textparser_token_item *parse_token_group_all_children_in_same_order(textparser_handle *int_handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset)
{
    textparser_token_item *ret = nullptr;

    if (int_handle == nullptr)
    {
        LOGF("int_handle == nullptr");
        return nullptr;
    }

    const textparser_language_definition *definition = int_handle->language;
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

    ret = malloc(sizeof(textparser_token_item));
    if (ret == nullptr) {
        int_handle->error = "Can't allocate memory!";
        return nullptr;
    }

    memset(ret, 0, sizeof(textparser_token_item));

    offset = textparser_skip_whitespace(int_handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    ssize_t start_pos = textparser_find_token(int_handle, start_token_id, offset, definition->other_text_inside);
    if (start_pos == TOKEN_NOT_FOUND) {
        exit_with_error("Expected start token!", offset);
    }
    offset += start_pos;

    child = textparser_parse_token(int_handle, start_token_id, parent_token_id, parent_start_stop, offset);
    check_and_exit_on_fatal_parsing_error(offset);

    ret->child = child;
    last_child = child;

    offset = child->position + child->len;

    while(1)
    {
        offset = textparser_skip_whitespace(int_handle, offset);

        ssize_t inner_pos = textparser_find_token(int_handle, inner_token_id, offset, definition->other_text_inside);
        ssize_t end_pos   = textparser_find_token(int_handle, end_token_id,   offset, definition->other_text_inside);

        if (end_pos == TOKEN_NOT_FOUND) {
            exit_with_error("GroupAllChildrenInSameOrder end token not found!", offset);
        }

        if (inner_pos == TOKEN_NOT_FOUND) {
            break;
        }

        if (end_pos < inner_pos) {
            break;
        }

        if ((end_pos == inner_pos) && (!token_def->search_parent_end_token_last)) {
            break;
        }


        offset += inner_pos;

        child = textparser_parse_token(int_handle, inner_token_id, end_token_id, TEXTPARSER_SEARCH_START_TOKEN, offset);
        last_child->next = child;
        last_child = child;
        check_and_exit_on_fatal_parsing_error(offset);

        offset = child->position + child->len;
    }

    offset = textparser_skip_whitespace(int_handle, offset);
    ssize_t final_end_pos = textparser_find_token(int_handle, end_token_id, offset, true);

    if (final_end_pos == TOKEN_NOT_FOUND) {
         exit_with_error("End token vanished!", offset);
    }

    offset += final_end_pos;

    child = textparser_parse_token(int_handle, end_token_id, parent_token_id, parent_start_stop, offset);
    last_child->next = child;
    check_and_exit_on_fatal_parsing_error(offset);

    ret->len = child->position + child->len - ret->position;

exit:
    return ret;
}

static textparser_token_item *parse_token_simple_token(textparser_handle *int_handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset)
{
    textparser_token_item *ret = nullptr;

    if (int_handle == nullptr)
    {
        LOGF("int_handle == nullptr");
        return nullptr;
    }

    const textparser_language_definition *definition = int_handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];

    LOGV("enter TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN");

    if (offset >= int_handle->text_size) {
        exit_with_error("offset >= int_handle->text_size!", offset);
    }

    ret = malloc(sizeof(textparser_token_item));
    if (ret == nullptr) {
        exit_with_error("Can't allocate memory!", offset);
    }

    memset(ret, 0, sizeof(textparser_token_item));

    offset = textparser_skip_whitespace(int_handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    size_t token_start = 0;
    size_t len = 0;
    if (!adv_regex_find_pattern(token_def->start_regex, (void **)int_handle->start_regex + token_id, int_handle->text_format, int_handle->text_addr + offset, int_handle->text_size - offset, &token_start, &len, !int_handle->language->case_sensitivity, !definition->tokens[parent_token_id].other_text_inside)) {
        exit_with_error("Can't find start of the token!", offset);
    }

    if ((!token_def->other_text_inside)&&(token_start > 0)) {
        exit_with_error("immediate_start not rule not!", offset);
    }

    LOGV("TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN - Found [%s] at %zd", int_handle->language->tokens[ret->token_id].name, ret->position);
    ret->position = offset + token_start;
    ret->len = len;
    //current_offset = ret->position + len;

exit:
    return ret;
}

static textparser_token_item *parse_token_start_stop(textparser_handle *int_handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, bool stop_required)
{
    textparser_token_item *ret = nullptr;

    if (int_handle == nullptr) {
        exit_with_error("int_handle == nullptr!", offset);
    }

    const textparser_language_definition *definition = int_handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];
    textparser_token_item *child = nullptr;

    size_t token_start = 0;
    size_t token_end = 0;
    size_t len = 0;

    if (stop_required) {
        LOGV("enter TEXTPARSER_TOKEN_TYPE_START_STOP");
    } else {
        LOGV("enter TEXTPARSER_TOKEN_TYPE_START_OPT_STOP");
    }

    if (offset >= int_handle->text_size) {
        exit_with_error("offset >= int_handle->text_size!", offset);
    }

    ret = malloc(sizeof(textparser_token_item));
    if (ret == nullptr) {
        exit_with_error("Can't allocate memory!", offset);
    }

    memset(ret, 0, sizeof(textparser_token_item));

    offset = textparser_skip_whitespace(int_handle, offset);

    ret->token_id = token_id;
    ret->position = offset;

    // Search for start token
    if (!adv_regex_find_pattern(token_def->start_regex, (void **)int_handle->start_regex + token_id, int_handle->text_format, int_handle->text_addr + offset, int_handle->text_size - offset, &token_start, &len, !int_handle->language->case_sensitivity, !definition->tokens[parent_token_id].other_text_inside)) {
        exit_with_error("Can't find start of the token!", offset);
    }

    if ((!token_def->other_text_inside)&&(token_start > 0)) {
        exit_with_error("immediate_start not rule not!", offset);
    }

    ret->position = offset + token_start;
    offset = ret->position + len;

    if (offset >= int_handle->text_size) {
        exit_with_error("offset >= int_handle->text_size!", offset);
    }

    if (offset == int_handle->text_size) {
        exit_with_error("reached end of text!", offset);
    }

    if (token_def->nested_tokens)
    {
        textparser_token_item *last_child = nullptr;
        int child_token_id = 0;

        const int *nested_tokens = definition->tokens[token_id].nested_tokens;

        do {
            child_token_id = TextParser_END;

            offset = textparser_skip_whitespace(int_handle, offset);

            if (offset >= int_handle->text_size)
            {
                exit_with_error("offset >= int_handle->text_size", offset);
            }

            if (token_def->search_parent_end_token_last == false)
            {
                bool found_end = adv_regex_find_pattern(token_def->end_regex, (void **)int_handle->end_regex + token_id, int_handle->text_format, int_handle->text_addr + offset, int_handle->text_size - offset, &token_end, nullptr, !int_handle->language->case_sensitivity, false);
                if ((found_end)&&(token_end == 0))
                {
                    break;
                }
            }
            else
            {
                token_end = INT_MAX;
            }

            for(int c = 0; nested_tokens[c] != TextParser_END; c++)
            {
                LOGV("Search for nested token type %d - [%s] from pos: %zu", nested_tokens[c], int_handle->language->tokens[nested_tokens[c]].name, offset);
                ssize_t pos = textparser_find_token(int_handle, nested_tokens[c], offset, token_def->other_text_inside);
                LOGV("textparser_find_token() returned %zd", pos);
                if (pos < 0)
                    continue;

                if (pos < token_end)
                {
                    if (pos == 0)
                    {
                        child_token_id = nested_tokens[c];
                        token_end = 0;
                        break;
                    }

                    if (token_def->other_text_inside)
                    {
                        token_end = pos;
                        child_token_id = nested_tokens[c];
                    }
                }
            }

            if (token_def->search_parent_end_token_last == true)
            {
                size_t parent_end = 0;
                bool found_end = adv_regex_find_pattern(token_def->end_regex, (void **)int_handle->end_regex + token_id, int_handle->text_format, int_handle->text_addr + offset, int_handle->text_size - offset, &parent_end, nullptr, !int_handle->language->case_sensitivity, false);
                if ((found_end)&&(parent_end < token_end))
                {
                    token_end = parent_end;
                    break;
                }
            }

            offset += token_end;

            if (child_token_id >= 0)
            {
                child = textparser_parse_token(int_handle,  child_token_id, token_id, TEXTPARSER_SEARCH_END_TOKEN, offset);
                if (ret->child == nullptr)
                    ret->child = child;

                if (last_child)
                    last_child->next = child;
                last_child = child;

                offset += child->len;

                check_and_exit_on_fatal_parsing_error(offset);
            }

            if (offset == int_handle->text_size)
            {
                ret->len = offset + token_end + len - ret->position;
                return ret;
            }
        } while (child_token_id >= 0);
    }

    offset = textparser_skip_whitespace(int_handle, offset);

    if (offset >= int_handle->text_size) {
        exit_with_error("offset >= int_handle->text_size!", offset);
    }

    if ((!adv_regex_find_pattern(token_def->end_regex, (void **)int_handle->end_regex + token_id, int_handle->text_format, int_handle->text_addr + offset, int_handle->text_size - offset, &token_end, &len, !int_handle->language->case_sensitivity, false))&&(token_def->type == TEXTPARSER_TOKEN_TYPE_START_STOP)) {
        LOGE("Can't find [%s] at %zd. Text: [%s]", token_def->end_regex, offset, int_handle->text_addr + offset);
        exit_with_error("Can't find end of the token!", offset);
    }

    LOGV("TEXTPARSER_TOKEN_TYPE_START_(OPT)_STOP - Found [%s] at %zd", int_handle->language->tokens[ret->token_id].name, ret->position);
    ret->len = offset + token_end + len - ret->position;
    offset += token_end + len;

    if (offset != ret->position + ret->len) {
        exit_with_error("offset != ret->position + ret->len!", offset);
    }

exit:
    return ret;
}

static textparser_token_item *parse_token_dual_start_and_stop(textparser_handle *int_handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset)
{
    textparser_token_item *ret = nullptr;

    if (int_handle == nullptr)
    {
        LOGF("int_handle == nullptr");
        return nullptr;
    }

    LOGF("enter TEXTPARSER_TOKEN_TYPE_DUAL_START_AND_STOP");
    exit_with_error("Not implemented(TEXTPARSER_TOKEN_TYPE_DUAL_START_AND_STOP)!", offset);

    // TODO: Implement TEXTPARSER_TOKEN_TYPE_DUAL_START_AND_STOP

exit:
    return ret;
}

static textparser_token_item *textparser_parse_token(textparser_handle *int_handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset)
{
    if (int_handle == nullptr)
    {
        LOGF("int_handle == nullptr");
        return nullptr;
    }

    const textparser_language_definition *definition = int_handle->language;

    if (definition == nullptr) {
        LOGE("definition == nullptr");
        return nullptr;
    }

    if (token_id < TextParser_START) {
        LOGE("token_id < TextParser_START");
        return nullptr;
    }

    if (parent_token_id < TextParser_END) {
        LOGE("parent_token_id < TextParser_END");
        return nullptr;
    }

    if (offset >= int_handle->text_size) {
        LOGE("offset >= int_handle->text_size");
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

    offset = textparser_skip_whitespace(int_handle, offset);

    LOGV("id: %d - [%s]  at offset: %zu", token_id, token_def->name, offset);
    switch(token_def->type)
    {
        case TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY:             return parse_token_group_one_child_only(int_handle, token_id, parent_token_id, parent_start_stop, offset);
        case TEXTPARSER_TOKEN_TYPE_GROUP:                            return parse_token_group(int_handle, token_id, parent_token_id, parent_start_stop, offset);
        case TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER: return parse_token_group_all_children_in_same_order(int_handle, token_id, parent_token_id, parent_start_stop, offset);
        case TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN:                     return parse_token_simple_token(int_handle, token_id, parent_token_id, parent_start_stop, offset);
        case TEXTPARSER_TOKEN_TYPE_START_STOP:                       return parse_token_start_stop(int_handle, token_id, parent_token_id, parent_start_stop, offset, true);
        case TEXTPARSER_TOKEN_TYPE_START_OPT_STOP:                   return parse_token_start_stop(int_handle, token_id, parent_token_id, parent_start_stop, offset, false);
        case TEXTPARSER_TOKEN_TYPE_DUAL_START_AND_STOP:              return parse_token_dual_start_and_stop(int_handle, token_id, parent_token_id, parent_start_stop, offset);
    }

    parse_token_error_error(int_handle, "Unknown token type!", offset);

    return nullptr;
}

static void textparser_init_regex(textparser_handle *int_handle)
{
    int token_cnt = 0;

    if (int_handle == nullptr)
        return;

    while(int_handle->language->tokens[token_cnt].name != nullptr)
        token_cnt++;

    if (token_cnt > 0)
    {
        int malloc_size = token_cnt * sizeof(void *);

        int_handle->start_regex = malloc(malloc_size);
        memset(int_handle->start_regex, 0, malloc_size);

        int_handle->end_regex = malloc(malloc_size);
        memset(int_handle->end_regex, 0, malloc_size);
    }
}

static void textparser_free_regex(textparser_handle *int_handle)
{
    enum textparser_encoding text_format = TEXTPARSER_ENCODING_ERROR;
    int token_cnt = 0;

    if ((int_handle == nullptr)||((int_handle->start_regex == nullptr)&&(int_handle->end_regex == nullptr)))
        return;

    text_format = int_handle->text_format;

    while(int_handle->language->tokens[token_cnt].name != nullptr) token_cnt++;

    if (int_handle->start_regex)
    {
        if (token_cnt > 0)
        {
            void **regex = (void **)int_handle->start_regex;

            for(int c = 0; c < token_cnt; c++)
            {
                adv_regex_free(&regex[c], text_format);
            }
        }
        free(int_handle->start_regex);
        int_handle->start_regex = nullptr;
    }

    if (int_handle->end_regex)
    {
        if (token_cnt > 0)
        {
            void **regex = (void **)int_handle->end_regex;

            for(int c = 0; c < token_cnt; c++)
            {
                adv_regex_free(&regex[c], text_format);
            }
        }
        free(int_handle->end_regex);
        int_handle->end_regex = nullptr;
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

    // TODO: Free token and it's related stuff!

    free(definition);
}

int textparser_openfile(const char *pathname, int default_text_format, textparser_t *handle)
{
    textparser_handle local_hnd;
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

    if ((local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\xfe')&&(local_hnd.text_addr[1] == '\xbb')&&(local_hnd.text_addr[2] == '\xbf')) {
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
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x00')&&(local_hnd.text_addr[1] == '\x00')&&(local_hnd.text_addr[2] == '\xfe')&&(local_hnd.text_addr[3] == '\xff')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_32_BE;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\xff')&&(local_hnd.text_addr[1] == '\xfe')&&(local_hnd.text_addr[2] == '\x00')&&(local_hnd.text_addr[3] == '\x00')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_32_LE;
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

    local_hnd.text_format = default_text_format;

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
    int cur_line_pos = 0;

    switch(local_hnd.text_format) {
    case TEXTPARSER_ENCODING_LATIN1:
        for(int ch = 0; ch < local_hnd.text_size; ch++) {
            if (local_hnd.text_addr[ch] == '\n')
                local_hnd.no_lines++;
        }

        *handle = malloc(sizeof(textparser_handle) + (sizeof(size_t) * (local_hnd.no_lines)));
        if (*handle == nullptr) {
            err = 6;
            goto err;
        }
        memcpy(*handle, &local_hnd, sizeof(textparser_handle));

        for(int ch = 0; ch < local_hnd.text_size; ch++) {
            if (local_hnd.text_addr[ch] == '\n') {
                ((textparser_handle*)*handle)->lines[cur_line_pos++] = ch;
            }
        }
        break;
    case TEXTPARSER_ENCODING_UTF_8:
        LOGW("no_lines for TEXTPARSER_ENCODING_UTF_8 type is not implemented!");
        break;
    case TEXTPARSER_ENCODING_UNICODE:
        for(int ch = 0; ch < local_hnd.text_size / sizeof(uint16_t); ch++) {
            if (((uint16_t *)local_hnd.text_addr)[ch] == '\n')
                local_hnd.no_lines++;
        }

        *handle = malloc(sizeof(textparser_handle) + (sizeof(size_t) * (local_hnd.no_lines)));
        if (*handle == nullptr) {
            err = 6;
            goto err;
        }

        memcpy(*handle, &local_hnd, sizeof(textparser_handle));

        for(int ch = 0; ch < local_hnd.text_size / sizeof(uint16_t); ch++) {
            if (((uint16_t *)local_hnd.text_addr)[ch] == '\n') {
                ((textparser_handle*)*handle)->lines[cur_line_pos++] = ch;
            }
        }
        break;
    default:
        err = 7;
        goto err;
    }

    return 0;

err:
    if (local_hnd.mmap_addr) {
        os_unmap(local_hnd.mmap_addr, local_hnd.mmap_size);
    }

    return err;
}

int textparser_openmem(const char *text, int len, int text_format, textparser_t *handle)
{
    textparser_handle *ret = nullptr;

    ret = malloc(sizeof(textparser_handle));
    if (ret == nullptr) {
        return 6;
    }

    memset(ret, 0, sizeof(textparser_handle));

    ret->text_format = text_format;
    ret->text_addr = text;
    ret->text_size = len;

    *handle = ret;

    return 0;
}

void textparser_close(textparser_t handle)
{
    textparser_handle *int_handle = (textparser_handle *)handle;
    textparser_token_item *item = nullptr;
    void *mmap_addr = nullptr;
    size_t mmap_size = 0;

    if (int_handle == nullptr)
        return;

    textparser_free_regex(int_handle);

    mmap_addr = int_handle->mmap_addr;
    mmap_size = int_handle->mmap_size;

    if (mmap_addr) {
        os_unmap(mmap_addr, mmap_size);
    }

    item = int_handle->first_item;
    if (item)
    {
        free_item_tree(item);
        item = nullptr;
    }

    free(handle);
    handle = nullptr;
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
    textparser_handle *int_handle = (textparser_handle *)handle;

    textparser_token_item *prev_item = nullptr;

    size_t size = int_handle->text_size;
    size_t pos = 0;

    if (int_handle->language != definition)
    {
        textparser_free_regex(int_handle);
        int_handle->language = definition;
        textparser_init_regex(int_handle);
    }

    while(pos < size) {
        int closest_token_id = TextParser_END;
        size_t closest_offset = size - pos;

        pos = textparser_skip_whitespace(handle, pos);

        for (int c = 0; definition->starts_with[c] != TextParser_END; c++) {
            int token_id = definition->starts_with[c];
            ssize_t offset = textparser_find_token(handle, token_id, pos, definition->other_text_inside);
            if (offset != TOKEN_NOT_FOUND)
            {
                if (offset < closest_offset) {
                    closest_token_id = token_id;
                    closest_offset = offset;
                    if (offset == 0)
                        break;
                }
            }
        }

        if (closest_token_id < 0)
            break;

        closest_offset += pos;

        textparser_token_item *token_item = textparser_parse_token(handle, closest_token_id, TextParser_END, TEXTPARSER_SEARCH_END_TOKEN, closest_offset);
        if (token_item == nullptr) {
            LOGE("token_item == nullptr");
            return -1;
        }

        if (int_handle->first_item == nullptr)
            int_handle->first_item = token_item;

        if (prev_item)
            prev_item->next = token_item;

        if ((int_handle->error)||(token_item->len <= 0))
            return -1;

        pos = token_item->position + token_item->len;

        prev_item = token_item;
    }

    return 0;
}

const char *textparser_parse_error(textparser_t handle)
{
    const textparser_handle *int_handle = (const textparser_handle *)handle;

    return int_handle->error;
}

size_t textparser_parse_error_position(textparser_t handle)
{
    const textparser_handle *int_handle = (const textparser_handle *)handle;

    return int_handle->error_offset;
}

const char *textparser_get_text(textparser_t handle)
{
    const textparser_handle *int_handle = (const textparser_handle *)handle;

    if (int_handle == nullptr)
        return nullptr;

    return int_handle->text_addr;
}

size_t textparser_get_text_size(textparser_t handle)
{
    const textparser_handle *int_handle = (const textparser_handle *)handle;

    if (int_handle == nullptr)
        return 0;

    return int_handle->text_size;
}

textparser_token_item *textparser_get_first_token(const textparser_t handle)
{
    const textparser_handle *int_handle = (const textparser_handle *)handle;

    if (int_handle == nullptr)
        return nullptr;

    return int_handle->first_item;
}

char *textparser_get_token_text(const textparser_t handle, const textparser_token_item *item)
{
    char *ret = nullptr;

    const textparser_handle *int_handle = (const textparser_handle *)handle;

    if ((int_handle == nullptr)||(item == nullptr)||(item->len <= 0))
        return nullptr;

    ret = malloc(item->len + 1);
    if (ret) {
        memcpy(ret, int_handle->text_addr + item->position, item->len);
        ret[item->len] = '\0';
    }

    return ret;
}

const textparser_language_definition *textparser_get_language(const textparser_t handle)
{
    const textparser_handle *int_handle = (textparser_handle *)handle;

    if (int_handle == nullptr)
        return nullptr;

    return int_handle->language;
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
        return -1;

    return token->position;
}

size_t textparser_get_token_length(const textparser_token_item *token)
{
    if (token == nullptr)
        return -1;

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

static void textparser_parse_state_recursively_fill(const textparser_token_item *token, int *state)
{
    if (token == nullptr)
    {
        return;
    }

    size_t pos = token->position;
    size_t len = token->len;

    for(size_t c = 0; c < len; c++)
    {
        state[pos + c] = token->token_id;
    }

    textparser_parse_state_recursively_fill(token->child, state);
    textparser_parse_state_recursively_fill(token->next, state);
}

textparser_parser_state *textparser_state_new(const textparser_t handle)
{
    const textparser_handle *int_handle = (textparser_handle *)handle;
    textparser_parser_state *ret = nullptr;
    int to_allocate = 0;
    int allocated = 0;
    size_t size = 0;

    size = int_handle->text_size;
    allocated = (size * sizeof(int));
    to_allocate = offsetof(textparser_parser_state, state) + allocated;

    ret = malloc(to_allocate);
    if (ret)
    {
        ret->len = size;
        memset(ret->state, 0xff, allocated);

        textparser_parse_state_recursively_fill(int_handle->first_item, ret->state);
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
