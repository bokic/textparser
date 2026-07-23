#include <textparser-json.h>
#include <adv_regex.h>

#include <json.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>


#ifdef _MSC_VER
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
    (*definition)->name = strdup(name_str);

    found = json_object_object_get_ex(root_obj, "version", &value);
    if (found)
        (*definition)->version = json_object_get_double(value);
    else
        (*definition)->version = 0.;

    found = json_object_object_get_ex(root_obj, "emptySegmentLanguage", &value);
    if (found) {
        const char *empty_lang = json_object_get_string(value);
        (*definition)->empty_segment_language = empty_lang ? strdup(empty_lang) : nullptr;
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
            (*definition)->default_file_extensions[i] = ext_str ? strdup(ext_str) : nullptr;
        }

        (*definition)->default_file_extensions[array_length] = nullptr;
    }

    found = json_object_object_get_ex(root_obj, "defaultTextEncoding", &value);
    if (found) {
        const char *encoding = json_object_get_string(value);
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
            (*definition)->tokens[token_idx].name = strdup(str_val ? str_val : key);

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
            (*definition)->tokens[token_idx].start_regex = str_val ? strdup(str_val) : nullptr;

            key_value = nullptr;
            if (!json_object_object_get_ex(token_item, "endRegex", &key_value)) {
                json_object_object_get_ex(token_item, "end_regex", &key_value);
            }
            str_val = json_object_get_string(key_value);
            (*definition)->tokens[token_idx].end_regex = str_val ? strdup(str_val) : nullptr;

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

                (*definition)->tokens[token_idx].nested_tokens = malloc(sizeof(int) * (nested_cnt + 1));
                if (!(*definition)->tokens[token_idx].nested_tokens) {
                    (*definition)->error_string = "malloc for nested_tokens FAILED!";
                    ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
                    goto err;
                }

                for(int i = 0; i < nested_cnt; i++) {
                     json_object *item = json_object_array_get_idx(nested_tokens_json, i);
                     if (!item || !json_object_is_type(item, json_type_string)) {
                         (*definition)->error_string = "`nestedTokens` array element is not a string!";
                         ret_code = TEXTPARSER_JSON_NESTED_TOKENS_ELEMENT_NOT_STRING;
                         goto err;
                     }
                     const char *name = json_object_get_string(item);

                     int found_idx = TextParser_END;
                     for(int j = 0; j < (int)tokens_cnt; j++) {
                          if ((*definition)->tokens[j].name && strcmp((*definition)->tokens[j].name, name) == 0) {
                               found_idx = j;
                               break;
                          }
                     }
                     (*definition)->tokens[token_idx].nested_tokens[i] = found_idx;
                }
                 (*definition)->tokens[token_idx].nested_tokens[nested_cnt] = TextParser_END;
            } else {
                 (*definition)->tokens[token_idx].nested_tokens = nullptr;
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

    (*definition)->starts_with = malloc(sizeof(int) * (starts_with_cnt + 1));
    if ((*definition)->starts_with == nullptr) {
        (*definition)->error_string = "malloc for starts_with FAILED!";
        ret_code = TEXTPARSER_JSON_OUT_OF_MEMORY;
        goto err;
    }

    for(int i = 0; i < starts_with_cnt; i++) {
        json_object *item = json_object_array_get_idx(start_tokens_arr, i);
        if (!item || !json_object_is_type(item, json_type_string)) {
            (*definition)->error_string = "`startTokens` array element is not a string!";
            ret_code = TEXTPARSER_JSON_STARTS_WITH_ELEMENT_NOT_STRING;
            goto err;
        }
        const char *name = json_object_get_string(item);

        int found_idx = TextParser_END;
        for(size_t j = 0; j < tokens_cnt; j++) {
             if ((*definition)->tokens[j].name && strcmp((*definition)->tokens[j].name, name) == 0) {
                 found_idx = j;
                 break;
             }
        }
        (*definition)->starts_with[i] = found_idx;
    }
    (*definition)->starts_with[starts_with_cnt] = TextParser_END;

    found = json_object_object_get_ex(root_obj, "otherTextInside", &value);
    if (found) {
        (*definition)->other_text_inside = json_object_get_boolean(value);
    }

    found = json_object_object_get_ex(root_obj, "signAmbiguityFix", &value);
    if (found) {
        (*definition)->sign_ambiguity_fix = json_object_get_boolean(value);
    } else {
        (*definition)->sign_ambiguity_fix = false;
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
