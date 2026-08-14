#include <textparser-json.h>
#include "adv_regex.h"

#include <json.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>


#ifdef _WIN32
#define strcasecmp _stricmp
#endif

#define json_object_defer(var) struct json_object * var __attribute__((cleanup(json_object_cleanup))) = nullptr

static void json_object_cleanup(struct json_object **handle)
{
    if (handle)
    {
        json_object_put(*handle);
        *handle = nullptr;
    }
}

/* --- JSON String Pool Arena Allocator ------------------------------------ */

typedef struct json_string_pool_chunk {
    struct json_string_pool_chunk *next;
    size_t used;
    size_t capacity;
    char buffer[];
} json_string_pool_chunk;

typedef struct {
    json_string_pool_chunk *head;
} json_string_pool;

static json_string_pool *json_pool_create(void)
{
    return (json_string_pool *)calloc(1, sizeof(json_string_pool));
}

static char *json_pool_strdup(json_string_pool *pool, const char *str)
{
    if (!pool || !str) return nullptr;

    size_t len = strlen(str) + 1;
    json_string_pool_chunk *chunk = pool->head;

    if (!chunk || (chunk->used + len > chunk->capacity)) {
        size_t cap = len > 4096 ? len : 4096;
        json_string_pool_chunk *new_chunk = (json_string_pool_chunk *)malloc(sizeof(json_string_pool_chunk) + cap);
        if (!new_chunk) return nullptr;
        new_chunk->next = pool->head;
        new_chunk->used = 0;
        new_chunk->capacity = cap;
        pool->head = new_chunk;
        chunk = new_chunk;
    }

    char *dest = chunk->buffer + chunk->used;
    memcpy(dest, str, len);
    chunk->used += len;
    return dest;
}

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

    *definition = malloc(sizeof(textparser_language_definition));
    if (*definition == nullptr) {
        return TEXTPARSER_JSON_OUT_OF_MEMORY;
    }

    memset(*definition, 0, sizeof(textparser_language_definition));

    json_string_pool *pool = json_pool_create();
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
    (*definition)->name = json_pool_strdup(pool, name_str);
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

    if (found) {
        const char *empty_lang = json_object_get_string(value);
        if (empty_lang) {
            (*definition)->empty_segment_language = json_pool_strdup(pool, empty_lang);
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
                (*definition)->default_file_extensions[i] = json_pool_strdup(pool, ext_str);
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

    found = json_object_object_get_ex(root_obj, "startTokens", &value);
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

    found = json_object_object_get_ex(root_obj, "tokens", &tokens);
    if (!found) {
        (*definition)->error_string = "Mandatory field `tokens` is missing!";
        ret_code = TEXTPARSER_JSON_TOKENS_NOT_FOUND;
        goto err;
    }

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
        json_object_object_foreach(tokens, key, val) {
            json_object *token_item = val;
            struct json_object *key_value = nullptr;
            const char *str_val = nullptr;

            key_value = nullptr;
            json_object_object_get_ex(token_item, "name", &key_value);
            str_val = json_object_get_string(key_value);
            const char *target_name = str_val ? str_val : key;
            if (target_name) {
                (*definition)->tokens[token_idx].name = json_pool_strdup(pool, target_name);
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
                 else {
                     ret_code = TEXTPARSER_JSON_INVALID_TOKEN_TYPE;
                     goto err;
                 }
            } else {
                ret_code = TEXTPARSER_JSON_TOKEN_TYPE_NOT_FOUND;
                goto err;
            }

            key_value = nullptr;
            if (!json_object_object_get_ex(token_item, "startRegex", &key_value)) {
                if (!json_object_object_get_ex(token_item, "regex", &key_value)) {
                    json_object_object_get_ex(token_item, "start_regex", &key_value);
                }
            }
            str_val = json_object_get_string(key_value);
            if (str_val) {
                (*definition)->tokens[token_idx].start_regex = json_pool_strdup(pool, str_val);
                if ((*definition)->tokens[token_idx].start_regex == nullptr) {
                    (*definition)->error_string = "strdup for token start_regex FAILED!";
                    ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                    goto err;
                }
            } else {
                (*definition)->tokens[token_idx].start_regex = nullptr;
            }

            key_value = nullptr;
            if (!json_object_object_get_ex(token_item, "endRegex", &key_value)) {
                json_object_object_get_ex(token_item, "end_regex", &key_value);
            }
            str_val = json_object_get_string(key_value);
            if (str_val) {
                (*definition)->tokens[token_idx].end_regex = json_pool_strdup(pool, str_val);
                if ((*definition)->tokens[token_idx].end_regex == nullptr) {
                    (*definition)->error_string = "strdup for token end_regex FAILED!";
                    ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                    goto err;
                }
            } else {
                (*definition)->tokens[token_idx].end_regex = nullptr;
            }

            key_value = nullptr;
            if (!json_object_object_get_ex(token_item, "otherTextInside", &key_value)) {
                json_object_object_get_ex(token_item, "other_text_inside", &key_value);
            }
            (*definition)->tokens[token_idx].other_text_inside = key_value ? json_object_get_boolean(key_value) : false;

            key_value = nullptr;
            if (!json_object_object_get_ex(token_item, "deleteIfOnlyOneChild", &key_value)) {
                json_object_object_get_ex(token_item, "delete_if_only_one_child", &key_value);
            }
            (*definition)->tokens[token_idx].delete_if_only_one_child = key_value ? json_object_get_boolean(key_value) : false;

            key_value = nullptr;
            if (!json_object_object_get_ex(token_item, "mustHaveOneChild", &key_value)) {
                json_object_object_get_ex(token_item, "must_have_one_child", &key_value);
            }
            (*definition)->tokens[token_idx].must_have_one_child = key_value ? json_object_get_boolean(key_value) : false;

            key_value = nullptr;
            if (!json_object_object_get_ex(token_item, "multiLine", &key_value)) {
                json_object_object_get_ex(token_item, "multi_line", &key_value);
            }
            (*definition)->tokens[token_idx].multi_line = key_value ? json_object_get_boolean(key_value) : false;

            key_value = nullptr;
            if (!json_object_object_get_ex(token_item, "searchParentEndTokenLast", &key_value)) {
                json_object_object_get_ex(token_item, "search_parent_end_token_last", &key_value);
            }
            (*definition)->tokens[token_idx].search_parent_end_token_last = key_value ? json_object_get_boolean(key_value) : false;

            key_value = nullptr;
            if (!json_object_object_get_ex(token_item, "textColor", &key_value)) {
                json_object_object_get_ex(token_item, "text_color", &key_value);
            }
            (*definition)->tokens[token_idx].text_color = get_color_or_flag_value(key_value, TEXTPARSER_NOCOLOR);

            key_value = nullptr;
            if (!json_object_object_get_ex(token_item, "textBackground", &key_value)) {
                json_object_object_get_ex(token_item, "text_background", &key_value);
            }
            (*definition)->tokens[token_idx].text_background = get_color_or_flag_value(key_value, TEXTPARSER_NOCOLOR);

            key_value = nullptr;
            if (!json_object_object_get_ex(token_item, "textFlags", &key_value)) {
                json_object_object_get_ex(token_item, "text_flags", &key_value);
            }
            (*definition)->tokens[token_idx].text_flags = get_color_or_flag_value(key_value, 0);

            token_idx++;
        }
    }

    // Pass 2: Resolve nested tokens names to indices
    {
        size_t token_idx = 0;
        json_object_object_foreach(tokens, key, val) {
            json_object *token_item = val;
            struct json_object *nested_tokens_json = nullptr;
            (void)key;

            nested_tokens_json = nullptr;
            if (!json_object_object_get_ex(token_item, "nestedTokens", &nested_tokens_json)) {
                json_object_object_get_ex(token_item, "nested_tokens", &nested_tokens_json);
            }
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
            if (!json_object_object_get_ex(token_item, "contextNestedTokens", &cnt_json)) {
                json_object_object_get_ex(token_item, "context_nested_tokens", &cnt_json);
            }
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
                        if (!json_object_object_get_ex(rule_obj, "whenParentIn", &wpi_arr)) {
                            json_object_object_get_ex(rule_obj, "when_parent_in", &wpi_arr);
                        }

                        json_object *nt_arr = nullptr;
                        if (!json_object_object_get_ex(rule_obj, "nestedTokens", &nt_arr)) {
                            json_object_object_get_ex(rule_obj, "nested_tokens", &nt_arr);
                        }

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
                                    ext_list[e] = json_pool_strdup(pool, ext_val);
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
                            rules[r].regex = json_pool_strdup(pool, reg_val);
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
