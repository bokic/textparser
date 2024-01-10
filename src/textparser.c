#include <stdio.h>
#include <textparser.h>
#include <adv_regex.h>

#include <json-c/json.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>


#define MAX_PARSE_SIZE (16 * 1024 * 1024)

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

typedef struct {
    const language_definition *language;
    void *mmap_addr;
    size_t mmap_size;
    enum textparser_bom bom;
    enum textparser_text_format text_format;
    textparser_token_item *first_item;
    bool fatal_error;
    const char *text_addr;
    size_t text_size;
    size_t no_lines;
    size_t lines[];
} textparser_handle;


static void free_item_tree(textparser_token_item *item)
{
    textparser_token_item *next = NULL;

    if (item == NULL)
        return;

    if (item->child)
    {
        free_item_tree(item->child);
        item->child = NULL;
    }

    next = item->next;
    if (next)
    {
        free_item_tree(next);

        item->next = NULL;
    }

    free(item);
}

static void textparser_skipwhitespace(const textparser_handle *int_handle, int *index)
{
    if (int_handle->text_format == TEXTPARSER_UNICODE)
    {
        while(1)
        {
            u_int16_t ch = *(u_int16_t *)(int_handle->text_addr + (*index));

            if ((ch != ' ')&&(ch != '\t')&&(ch != '\r')&&(ch != '\n'))
                break;

            *index = *index + 2;
        }
    }
    else
    {
        while(1)
        {
            char ch = int_handle->text_addr[*index];

            if ((ch != ' ')&&(ch != '\t')&&(ch != '\r')&&(ch != '\n'))
                break;

            *index = *index + 1;
        }
    }
}

static int textparser_find_token(const textparser_handle *int_handle, const language_definition *definition, int token, int offset)
{
    size_t ret = 0;

    bool res = adv_regex_find_pattern(definition->tokens[token].start_string, ADV_REGEX_TEXT_LATIN1, int_handle->text_addr + offset, int_handle->text_size - offset, &ret, NULL);
    if (!res)
        return -1;

    return ret;
}

static textparser_token_item *textparser_parse_token(textparser_handle *int_handle, const language_definition *definition, int token_id, int offset)
{
    textparser_token_item *ret = NULL;

    /*int len = 0;
    int token_start = 0;
    int token_end = 0;
    const int *nested_tokens = NULL;
    int current_offset = offset;

    ret = malloc(sizeof(textparser_token_item));
    memset(ret, 0, sizeof(textparser_token_item));

    const textparser_token *token_def = &definition->tokens[token_id];

    ret->token_id = token_id;

    printf("%s\n", token_def->start_string); fflush(stdout); usleep(100000);

    token_start = adv_regex_find_pattern(token_def->start_string, ADV_REGEX_TEXT_LATIN1, int_handle->text_addr + current_offset, int_handle->text_size - current_offset, &len);
    if (token_start < 0)
    {
        ret->error = "Can't find start of the token!";
        ret->position = current_offset;
        int_handle->fatal_error = true;

        return ret;
    }

    ret->position = current_offset + token_start;
    current_offset = ret->position + len;

    if (token_def->nested_tokens)
    {
        textparser_token_item *last_child = NULL;
        int child_token_id = 0;

        nested_tokens = definition->tokens[token_id].nested_tokens;

        do {
            child_token_id = -1;

            textparser_skipwhitespace(int_handle, &current_offset);

            int max_end_token = int_handle->mmap_size - current_offset;
            if (!token_def->only_start_tag)
            {
                int max_end_token_len = -1;
                max_end_token = adv_regex_find_pattern(token_def->end_string, ADV_REGEX_TEXT_LATIN1, int_handle->text_addr + current_offset, int_handle->text_size - current_offset, &max_end_token_len);
                if (max_end_token < 0) {
                    max_end_token = int_handle->mmap_size - current_offset;
                }
            }

            for(int c = 0; nested_tokens[c] != -1; c++)
            {
                int pos = textparser_find_token(int_handle, definition, nested_tokens[c], current_offset);
                if (pos < 0)
                    continue;

                if ((token_def->search_end_tag_first == false)||(pos < max_end_token))
                {
                    if (pos == 0)
                    {
                        child_token_id = nested_tokens[c];
                        break;
                    }

                    if (token_def->can_have_other_text_inside)
                    {
                        if (pos < max_end_token)
                        {
                            max_end_token = pos;
                            child_token_id = nested_tokens[c];
                        }
                    }
                }
            }

            if (child_token_id >= 0)
            {
                textparser_token_item *child = textparser_parse_token(int_handle, definition, child_token_id, current_offset);

                if (ret->child == NULL)
                    ret->child = child;

                if (last_child)
                    last_child->next = child;
                last_child = child;

                if (int_handle->fatal_error)
                    return ret;

                current_offset += child->len;
            }
        } while (child_token_id >= 0);
    }

    if (!token_def->only_start_tag)
    {
        textparser_skipwhitespace(int_handle, &current_offset);
        token_end = adv_regex_find_pattern(token_def->end_string, ADV_REGEX_TEXT_LATIN1, int_handle->text_addr + current_offset, int_handle->text_size - current_offset, &len);
        if (token_end >= 0)
        {
            current_offset += len;
            ret->len = current_offset - ret->position;
        }
    }
    else
    {
        ret->len = len;
    }

    if ((token_def->only_start_tag == false)&&(token_def->end_tag_is_optional == false)&&(token_end < 0))
    {
        ret->error = "Can't find end of the token!";
        ret->position = current_offset;
        int_handle->fatal_error = true;
    }*/

    return ret;
}

static int textparser_load_language_definition_internal(struct json_object *root_obj, language_definition **definition)
{
    int ret = 0;

    size_t arary_length = 0;
    json_object *tokens = NULL;
    json_object *value = NULL;
    json_bool found = false;

    if (root_obj == NULL) {
        fputs(json_util_get_last_err(), stderr);
            ret = 1;
        goto cleanup;
    }

    *definition = malloc(sizeof(language_definition));
    if (*definition == NULL) {
        ret = 2;
        goto cleanup;
    }

    memset(*definition, 0, sizeof(language_definition));

    found = json_object_object_get_ex(root_obj, "name", &value);
    if (!found){
        (*definition)->error_string = "Mandatory field `name` not set!";
        ret = 3;
        goto cleanup;
    }

    (*definition)->name = strdup(json_object_get_string(value));

    found = json_object_object_get_ex(root_obj, "version", &value);
    if (found)
        (*definition)->version = json_object_get_double(value);
    else
        (*definition)->version = 0.;

    found = json_object_object_get_ex(root_obj, "empty_segment_language", &value);
    if (found)
        (*definition)->empty_segment_language = strdup(json_object_get_string(value));

    found = json_object_object_get_ex(root_obj, "case_sensitivity", &value);
    if (!found) {
        (*definition)->error_string = "Mandatory field `case_sensitivity` not set!";
        ret = 4;
        goto cleanup;
    }
    (*definition)->case_sensitivity = json_object_get_boolean(value);

    found = json_object_object_get_ex(root_obj, "file_extensions", &value);
    if (!found) {
        (*definition)->error_string = "Mandatory field `file_extensions` not set!";
        ret = 5;
        goto cleanup;
    }
    arary_length = json_object_array_length(value);
    if (arary_length > 0) {
        (*definition)->default_file_extensions = malloc((arary_length + 1) * sizeof(char *));

        for(size_t i = 0; i < arary_length; i++) {
            json_object *array_item = json_object_array_get_idx(value, i);
            (*definition)->default_file_extensions[i] = strdup(json_object_get_string(array_item));
        }

        (*definition)->default_file_extensions[arary_length] = NULL;
    }

    found = json_object_object_get_ex(root_obj, "encoding", &value);
    if (found) {
        const char *encoding = json_object_get_string(value);
        if(strcmp(encoding, "latin1") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_LATIN_1;
        else if(strcmp(encoding, "utf8") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_UTF_8;
        else if(strcmp(encoding, "unicode") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_UNICODE;
        else {
            (*definition)->error_string = "Invalid `encoding` text encoding!";
            ret = 6;
            goto cleanup;
        }
    }
    else
        (*definition)->default_text_encoding = TEXTPARSER_LATIN_1;

    found = json_object_object_get_ex(root_obj, "starts_with", &value);
    if (!found) {
        (*definition)->error_string = "Mandatory field `starts_with` is missing!";
        ret = 7;
        goto cleanup;
    }

    if (!json_object_is_type(value, json_type_array)) {
        (*definition)->error_string = "`starts_with` is not array!";
        ret = 8;
        goto cleanup;
    }

    found = json_object_object_get_ex(root_obj, "tokens", &tokens);
    if (!found) {
        (*definition)->error_string = "Mandatory field `tokens` is missing!";
        ret = 9;
        goto cleanup;
    }

    if (!json_object_is_type(tokens, json_type_array)) {
        (*definition)->error_string = "`tokens` is not array!";
        ret = 9;
        goto cleanup;
    }

    size_t tokens_cnt = json_object_array_length(tokens);

    size_t starts_with_cnt = json_object_array_length(value);
    if (starts_with_cnt == 0) {
        (*definition)->error_string = "`starts_with` array is empty!";
        ret = 9;
        goto cleanup;
    }

    (*definition)->starts_with = malloc(sizeof(int) * (starts_with_cnt + 1));

    memset((*definition)->starts_with, 0, sizeof(int) * starts_with_cnt);
    (*definition)->starts_with[starts_with_cnt] = -1;

    for(size_t c = 0; c < starts_with_cnt; c++) {
        json_object *array_item = json_object_array_get_idx(value, c);

        if (!json_object_is_type(array_item, json_type_string)) {
            (*definition)->error_string = "`starts_with` element not string!";
            ret = 10;
            goto cleanup;
        }

        const char *name = json_object_get_string(array_item);

        for(size_t c2 = 0; c2 < tokens_cnt; c2++) {
            json_object *token_item = json_object_array_get_idx(tokens, c2);

            struct json_object *key_value = NULL;
            json_object_object_get_ex(token_item, "name", &key_value);
            const char *other_name = json_object_get_string(key_value);

            if (strcmp(name, other_name) == 0) {
                (*definition)->starts_with[c] = c2;
                break;
            }
        }
    }

    found = json_object_object_get_ex(root_obj, "other_text_inside", &value);
    if (found) {
        (*definition)->other_text_inside = json_object_get_boolean(value);
    }

    (*definition)->tokens = malloc(sizeof(textparser_token) * (tokens_cnt + 1));
    if ((*definition)->tokens == NULL) {
        (*definition)->error_string = "malloc for tokens list FAILED!";
        ret = 11;
        goto cleanup;
    }

    if (tokens_cnt > 0) {
        for(size_t token_idx = 0; token_idx < tokens_cnt; token_idx++) {
            json_object *token_item = json_object_array_get_idx(tokens, token_idx);

            struct json_object *key_value = NULL;
            const char *other_name = NULL;

            json_object_object_get_ex(token_item, "name", &key_value);
            other_name = json_object_get_string(key_value);
            (*definition)->tokens[token_idx].name = strdup(other_name);

            json_object_object_get_ex(token_item, "start_string", &key_value);
            other_name = json_object_get_string(key_value);
            (*definition)->tokens[token_idx].start_string = strdup(other_name);

            json_object_object_get_ex(token_item, "end_string", &key_value);
            other_name = json_object_get_string(key_value);
            (*definition)->tokens[token_idx].end_string = strdup(other_name);

            json_object_object_get_ex(token_item, "only_start_tag", &key_value);
            if (key_value)
                (*definition)->tokens[token_idx].only_start_tag = json_object_get_boolean(key_value);
            else
                (*definition)->tokens[token_idx].only_start_tag = false;

            json_object_object_get_ex(token_item, "multi_line", &key_value);
            if (key_value)
                (*definition)->tokens[token_idx].multi_line = json_object_get_boolean(key_value);
            else
                (*definition)->tokens[token_idx].multi_line = false;

            json_object_object_get_ex(token_item, "can_have_other_text_inside", &key_value);
            if (key_value)
                (*definition)->tokens[token_idx].can_have_other_text_inside = json_object_get_boolean(key_value);
            else
                (*definition)->tokens[token_idx].can_have_other_text_inside = false;

            json_object_object_get_ex(token_item, "end_tag_is_optional", &key_value);
            if (key_value)
                (*definition)->tokens[token_idx].end_tag_is_optional = json_object_get_boolean(key_value);
            else
                (*definition)->tokens[token_idx].end_tag_is_optional = false;

            json_object_object_get_ex(token_item, "nested_tokens", &key_value);
            if (key_value) {
                if (!json_object_is_type(key_value, json_type_array)) {
                    (*definition)->error_string = "`nested_tokens` is not array!";
                    ret = 12;
                    goto cleanup;
                }

                size_t nested_tokens_cnt = json_object_array_length(key_value);
                if (nested_tokens_cnt == 0) {
                    (*definition)->error_string = "`nested_tokens` array is empty!";
                    ret = 13;
                    goto cleanup;
                }

                (*definition)->tokens[token_idx].nested_tokens = malloc(sizeof(int) * (nested_tokens_cnt + 1));

                for(size_t c2 = 0; token_idx < nested_tokens_cnt; c2++) {
                    json_object *array_item = json_object_array_get_idx(key_value, c2);

                    if (!json_object_is_type(array_item, json_type_string)) {
                        (*definition)->error_string = "`nested_tokens` element not string!";
                        ret = 14;
                        goto cleanup;
                    }

                    const char *name = json_object_get_string(array_item);
                    (*definition)->tokens[token_idx].nested_tokens[c2] = -1;

                    for(size_t c3 = 0; c3 < tokens_cnt; c3++) {
                        token_item = json_object_array_get_idx(tokens, c3);

                        struct json_object *key_value2 = NULL;
                        json_object_object_get_ex(token_item, "name", &key_value2);
                        other_name = json_object_get_string(key_value2);

                        if (strcmp(name, other_name) == 0) {
                            (*definition)->tokens[token_idx].nested_tokens[c2] = c3;
                            break;
                        }
                    }
                }

                (*definition)->tokens[token_idx].nested_tokens[nested_tokens_cnt] = -1;
            } else {
                (*definition)->tokens[token_idx].nested_tokens = NULL;
            }
        }
    }
    (*definition)->tokens[tokens_cnt].name = NULL;


cleanup:
    if (root_obj) {
        json_object_put(root_obj);
    }

    return ret;
}

static void textparser_dump_item(void *handle, textparser_token_item *item, int level)
{
    textparser_handle *int_handle = (textparser_handle *)handle;

    int token_id = item->token_id;
    int position = item->position;
    int len = item->len;
    const char *error = item->error;

    for(int c = 0; c < level; c++)
        write(1, "    ", 4);

    if (!item->child) {
        write(1, int_handle->language->tokens[token_id].name, strlen(int_handle->language->tokens[token_id].name));
        write(1, " ->", 3);
        write(1, int_handle->text_addr + position, len);
        write(1, "<-", 2);
        if (error) write(1, error, strlen(error));
        write(1, "\n", 1);

    } else {
        printf("%s, %d, %d, %s\n", int_handle->language->tokens[token_id].name, position, len, error);
    }
    fflush(stdout);

    textparser_token_item *child = item->child;
    while(child)
    {
        textparser_dump_item(handle, child, level + 1);
        child = child->next;
    }
}

EXPORT_TEXT_PARSER int textparser_load_language_definition_from_json_file(const char *pathname, language_definition **definition)
{
    return textparser_load_language_definition_internal(json_object_from_file(pathname), definition);
}

EXPORT_TEXT_PARSER int textparser_load_language_definition_from_string(const char *text, language_definition **definition)
{
    return textparser_load_language_definition_internal(json_tokener_parse(text), definition);
}

EXPORT_TEXT_PARSER void textparser_free_language_definition(language_definition *definition)
{
    if (definition == NULL)
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

EXPORT_TEXT_PARSER int textparser_openfile(const char *pathname, int text_format, void **handle)
{
    textparser_handle local_hnd;
    struct stat fd_stat;
    int err = 0;

    //uint8_t bytes[] = {0xd0, 0x91, 0xd0, 0xbe, 0xd1, 0x80, 0xd0, 0xbe};
    //uint32_t ch = 0;
    //int ret = utf8proc_iterate(bytes, 6, &ch);

    memset(&local_hnd, 0, sizeof(local_hnd));

    int fd = open(pathname, O_RDONLY);
    if (fd <= 0) {
        err = 1;
        goto err;
    }

    if (fstat(fd, &fd_stat)) {
        err = 2;
        goto err;
    }

    if (fd_stat.st_size > MAX_PARSE_SIZE) {
        err = 3;
        goto err;
    }

    local_hnd.mmap_size = fd_stat.st_size;

    local_hnd.mmap_addr = mmap(NULL, local_hnd.mmap_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (local_hnd.mmap_addr == NULL) {
        err = 4;
        goto err;
    }

    close(fd);

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

    // TODO: Currently don't support BOM
    if (local_hnd.bom != NO_BOM) {
        err = 5;
        goto err;
    }

    local_hnd.no_lines = 0;
    int cur_line_pos = 0;

    local_hnd.text_format = text_format;

    switch(text_format) {
    case TEXTPARSER_LATIN_1:
        for(int ch = 0; ch < local_hnd.text_size; ch++) {
            if (local_hnd.text_addr[ch] == '\n')
                local_hnd.no_lines++;
        }

        *handle = malloc(sizeof(textparser_handle) + (sizeof(size_t) * (local_hnd.no_lines)));
        if (*handle == NULL) {
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
    case TEXTPARSER_UTF_8:
        break;
    case TEXTPARSER_UNICODE:
        for(int ch = 0; ch < local_hnd.text_size / sizeof(u_int16_t); ch++) {
            if (((u_int16_t *)local_hnd.text_addr)[ch] == '\n')
                local_hnd.no_lines++;
        }

        *handle = malloc(sizeof(textparser_handle) + (sizeof(size_t) * (local_hnd.no_lines)));
        if (*handle == NULL) {
            err = 6;
            goto err;
        }

        memcpy(*handle, &local_hnd, sizeof(textparser_handle));

        for(int ch = 0; ch < local_hnd.text_size / sizeof(u_int16_t); ch++) {
            if (((u_int16_t *)local_hnd.text_addr)[ch] == '\n') {
                ((textparser_handle*)*handle)->lines[cur_line_pos++] = ch;
            }
        }
        break;
    default:
        err = 7;
        goto err;
        break;
    }

    return 0;

err:
    if (local_hnd.mmap_addr)
        munmap(local_hnd.mmap_addr, local_hnd.mmap_size);

    return err;
}

EXPORT_TEXT_PARSER int textparser_openmem(const char *text, int len, int text_format, void **handle)
{
    textparser_handle *ret = NULL;

    ret = malloc(sizeof(textparser_handle));
    if (ret == NULL) {
        return 6;
    }

    memset(ret, 0, sizeof(textparser_handle));

    ret->text_format = text_format;
    ret->text_addr = text;
    ret->text_size = len;

    *handle = ret;

    return 0;
}

EXPORT_TEXT_PARSER void textparser_close(void *handle)
{
    textparser_handle *int_handle = (textparser_handle *)handle;
    textparser_token_item *item = NULL;
    void *mmap_addr = NULL;
    size_t mmap_size = 0;

    if (int_handle == NULL)
        return;

    mmap_addr = int_handle->mmap_addr;
    mmap_size = int_handle->mmap_size;

    if (mmap_addr)
        munmap(mmap_addr, mmap_size);

    item = int_handle->first_item;
    if (item)
    {
        free_item_tree(item);
        item = NULL;
    }

    free(handle);
}

EXPORT_TEXT_PARSER void textparser_dump(void *handle)
{
    textparser_handle *int_handle = (textparser_handle *)handle;

    if (int_handle->fatal_error) {
        printf("int_handle->fatal_error = TRUE!");
        return;
    }

    textparser_token_item *item = int_handle->first_item;
    while(item)
    {
        textparser_dump_item(handle, item, 0);
        item = item->next;
    }
}

EXPORT_TEXT_PARSER int textparser_parse(void *handle, const language_definition *definition)
{
    textparser_handle *int_handle = (textparser_handle *)handle;

    textparser_token_item *prev_item = NULL;

    const char *text = int_handle->text_addr;
    size_t size = int_handle->text_size;
    size_t pos = 0;

    int_handle->language = definition;

    while(pos < size) {
        int closesed_token = -1;
        int closesed_offset = size - pos;

        for (int c = 0; definition->starts_with[c] != -1; c++) {
            int token_id = definition->starts_with[c];
            int offset = textparser_find_token(handle, definition, token_id, pos);
            if ((offset >= 0)&&(offset < closesed_offset)) {
                closesed_token = token_id;
                closesed_offset = pos + offset;
            }

            if (closesed_offset == 0)
                break;
        }

        if ((closesed_offset < 0)||(closesed_token < 0))
            break;

        textparser_token_item *token_item = textparser_parse_token(handle, definition, closesed_token, closesed_offset);

        if (int_handle->first_item == NULL)
            int_handle->first_item = token_item;

        if (prev_item)
            prev_item->next = token_item;

        if ((int_handle->fatal_error)||(token_item->token_id == -1)||(token_item->len <= 0))
            return -1;

        pos = token_item->position + token_item->len;

        prev_item = token_item;
    }

    return 0;
}

EXPORT_TEXT_PARSER textparser_token_item *textparser_get_first_token(void *handle)
{
    textparser_handle *int_handle = (textparser_handle *)handle;

    if (int_handle == NULL)
        return NULL;

    return int_handle->first_item;
}

EXPORT_TEXT_PARSER const char *textparser_get_token_id_name(void *handle, int token_id)
{
    textparser_handle *int_handle = (textparser_handle *)handle;

    if ((int_handle == NULL)||(token_id < 0))
        return NULL;

    for (int c = 0; c <= token_id; c++)
    {
        if (int_handle->language->tokens[c].name == NULL)
            return NULL;

        if (c == token_id)
            return int_handle->language->tokens[c].name;
    }

    return NULL;
}

EXPORT_TEXT_PARSER char *textparser_get_token_text(void *handle, textparser_token_item *item)
{    
    char *ret = NULL;

    textparser_handle *int_handle = (textparser_handle *)handle;

    if ((int_handle == NULL)||(item == NULL)||(item->len <= 0))
        return NULL;

    ret = malloc(item->len + 1);
    memcpy(ret, int_handle->text_addr + item->position, item->len);
    ret[item->len] = '\0';

    return ret;
}

EXPORT_TEXT_PARSER textparser_parser_state *textparser_parse_state_new(void *state, int size)
{
    textparser_parser_state *ret = NULL;
    int to_allocate = 0;
    int allocated = 0;

    allocated = ((size / 16) + 2) * 16;
    to_allocate = offsetof(textparser_parser_state, state) + allocated;

    ret = malloc(to_allocate);
    if (ret)
    {
        ret->allocated = allocated;
        ret->len = size;
        if (size)
        {
            memcpy(ret->state, state, size);
        }
        memset(&ret->state[size], 0, allocated - size);
    }

    return ret;
}

EXPORT_TEXT_PARSER void textparser_parse_state_free(textparser_parser_state *state)
{
    if (state)
    {
        free(state);
    }
}

EXPORT_TEXT_PARSER textparser_line_parser_item *textparser_parse_line(const char *line, enum textparser_text_format text_format, textparser_parser_state *state, const language_definition *definition)
{
    return NULL;
}
