#include <textparser-json.h>
#include <adv_regex.h>

#include <json.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>


#define json_object_defer(var) struct json_object * var __attribute__((cleanup(json_object_cleanup))) = nullptr

static void json_object_cleanup(struct json_object **handle)
{
    if (handle)
    {
        json_object_put(*handle);
        *handle = nullptr;
    }
}

static int textparser_json_load_language_definition_internal(struct json_object *root_obj, textparser_language_definition **definition)
{
    size_t array_length = 0;
    json_object *tokens = nullptr;
    json_object *value = nullptr;
    json_bool found = false;

    if (root_obj == nullptr) {
        return TEXTPARSER_JSON_ROOT_OBJ_IS_NULL;
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
        return TEXTPARSER_JSON_NAME_NOT_FOUND;
    }

    (*definition)->name = strdup(json_object_get_string(value));

    found = json_object_object_get_ex(root_obj, "version", &value);
    if (found)
        (*definition)->version = json_object_get_double(value);
    else
        (*definition)->version = 0.;

    found = json_object_object_get_ex(root_obj, "emptySegmentLanguage", &value);
    if (found)
        (*definition)->empty_segment_language = strdup(json_object_get_string(value));

    found = json_object_object_get_ex(root_obj, "caseSensitivity", &value);
    if (!found) {
        (*definition)->error_string = "Mandatory field `caseSensitivity` not set!";
        return TEXTPARSER_JSON_CASE_SENSITIVITY_NOT_FOUND;
    }
    (*definition)->case_sensitivity = json_object_get_boolean(value);

    found = json_object_object_get_ex(root_obj, "defaultFileExtensions", &value);
    if (!found) {
        (*definition)->error_string = "Mandatory field `defaultFileExtensions` not set!";
        return TEXTPARSER_JSON_FILE_EXTENSIONS_NOT_FOUND;
    }
    array_length = json_object_array_length(value);
    if (array_length > 0) {
        (*definition)->default_file_extensions = malloc((array_length + 1) * sizeof(char *));
        if ((*definition)->default_file_extensions == nullptr) {
            return TEXTPARSER_JSON_OUT_OF_MEMORY;
        }

        memset((*definition)->default_file_extensions, 0, (array_length + 1) * sizeof(char *));

        for(size_t i = 0; i < array_length; i++) {
            json_object *array_item = json_object_array_get_idx(value, i);
            (*definition)->default_file_extensions[i] = strdup(json_object_get_string(array_item));
        }

        (*definition)->default_file_extensions[array_length] = nullptr;
    }

    found = json_object_object_get_ex(root_obj, "defaultTextEncoding", &value);
    if (found) {
        const char *encoding = json_object_get_string(value);
        if(strcmp(encoding, "latin1") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_ENCODING_LATIN1;
        else if(strcmp(encoding, "utf8") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_ENCODING_UTF_8;
        else if(strcmp(encoding, "unicode") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_ENCODING_UNICODE;
        else {
            (*definition)->error_string = "Invalid `defaultTextEncoding` encoding! Should be one of the following: latin1, utf8, unicode.";
            return TEXTPARSER_JSON_ENCODING_NOT_FOUND;
        }
    }
    else
    {
        (*definition)->default_text_encoding = TEXTPARSER_ENCODING_LATIN1;
    }

    found = json_object_object_get_ex(root_obj, "startTokens", &value);
    if (!found) {
        (*definition)->error_string = "Mandatory field `startTokens` is missing!";
        return TEXTPARSER_JSON_STARTS_WITH_NOT_FOUND;
    }

    if (!json_object_is_type(value, json_type_array)) {
        (*definition)->error_string = "`startTokens` is not array!";
        return TEXTPARSER_JSON_STARTS_WITH_NOT_ARRAY;
    }
    
    // Save startTokens json object for later processing
    json_object *start_tokens_arr = value;

    found = json_object_object_get_ex(root_obj, "tokens", &tokens);
    if (!found) {
        (*definition)->error_string = "Mandatory field `tokens` is missing!";
        return TEXTPARSER_JSON_TOKENS_NOT_FOUND;
    }

    if (!json_object_is_type(tokens, json_type_object)) {
        (*definition)->error_string = "`tokens` is not object!";
        return TEXTPARSER_JSON_TOKENS_NOT_OBJECT;
    }

    size_t tokens_cnt = json_object_object_length(tokens);

    (*definition)->tokens = malloc(sizeof(textparser_token) * (tokens_cnt + 1));
    if ((*definition)->tokens == nullptr) {
        (*definition)->error_string = "malloc for tokens list FAILED!";
        return TEXTPARSER_JSON_OUT_OF_MEMORY;
    }

    memset((*definition)->tokens, 0, sizeof(textparser_token) * (tokens_cnt + 1));

    // Pass 1: Load basic token data
    {
        size_t token_idx = 0;
        json_object_object_foreach(tokens, key, val) {
            json_object *token_item = val;
            struct json_object *key_value = nullptr;
            const char *str_val = nullptr;

            // Use the key as the name if "name" field is missing? 
            // The JSON definition usually includes "name" inside.
            json_object_object_get_ex(token_item, "name", &key_value);
            str_val = json_object_get_string(key_value);
            (*definition)->tokens[token_idx].name = strdup(str_val ? str_val : key);

            json_object_object_get_ex(token_item, "type", &key_value);
            str_val = json_object_get_string(key_value);
            if (str_val) {
                 if (strcasecmp(str_val, "Group") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_GROUP;
                 else if (strcasecmp(str_val, "GroupAllChildrenInSameOrder") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER;
                 else if (strcasecmp(str_val, "GroupOneChildOnly") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY;
                 else if (strcasecmp(str_val, "SimpleToken") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN;
                 else if (strcasecmp(str_val, "StartStop") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_START_STOP;
                 else if (strcasecmp(str_val, "StartOptStop") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_START_OPT_STOP;
                 else if (strcasecmp(str_val, "DualStartAndStop") == 0) (*definition)->tokens[token_idx].type = TEXTPARSER_TOKEN_TYPE_DUAL_START_AND_STOP;
                 else return TEXTPARSER_JSON_INVALID_TOKEN_TYPE;
            } else {
                return TEXTPARSER_JSON_TOKEN_TYPE_NOT_FOUND;
            }

            json_object_object_get_ex(token_item, "start_regex", &key_value);
            str_val = json_object_get_string(key_value);
            (*definition)->tokens[token_idx].start_regex = str_val ? strdup(str_val) : nullptr;

            json_object_object_get_ex(token_item, "end_regex", &key_value);
            str_val = json_object_get_string(key_value);
            (*definition)->tokens[token_idx].end_regex = str_val ? strdup(str_val) : nullptr;

            json_object_object_get_ex(token_item, "other_text_inside", &key_value);
            (*definition)->tokens[token_idx].other_text_inside = key_value ? json_object_get_boolean(key_value) : false;

            json_object_object_get_ex(token_item, "delete_if_only_one_child", &key_value);
            (*definition)->tokens[token_idx].delete_if_only_one_child = key_value ? json_object_get_boolean(key_value) : false;

            json_object_object_get_ex(token_item, "must_have_one_child", &key_value);
            (*definition)->tokens[token_idx].must_have_one_child = key_value ? json_object_get_boolean(key_value) : false;

            json_object_object_get_ex(token_item, "multi_line", &key_value);
            (*definition)->tokens[token_idx].multi_line = key_value ? json_object_get_boolean(key_value) : false;

            json_object_object_get_ex(token_item, "search_parent_end_token_last", &key_value);
            (*definition)->tokens[token_idx].search_parent_end_token_last = key_value ? json_object_get_boolean(key_value) : false;

            json_object_object_get_ex(token_item, "text_color", &key_value);
            (*definition)->tokens[token_idx].text_color = key_value ? ((uint32_t)json_object_get_int64(key_value)) : TEXTPARSER_NOCOLOR;

            json_object_object_get_ex(token_item, "text_background", &key_value);
            (*definition)->tokens[token_idx].text_background = key_value ? ((uint32_t)json_object_get_int64(key_value)) : TEXTPARSER_NOCOLOR;

            json_object_object_get_ex(token_item, "text_flags", &key_value);
            (*definition)->tokens[token_idx].text_flags = key_value ? ((uint32_t)json_object_get_int64(key_value)) : 0;

            token_idx++;
        }
    }
    
    // Pass 2: Resolve nested tokens names to indices
    {
        size_t token_idx = 0;
        json_object_object_foreach(tokens, key, val) {
            json_object *token_item = val;
            struct json_object *nested_tokens_json = nullptr;

            json_object_object_get_ex(token_item, "nested_tokens", &nested_tokens_json);
            if (nested_tokens_json) {
                if (!json_object_is_type(nested_tokens_json, json_type_array)) {
                    (*definition)->error_string = "`nested_tokens` is not array!";
                    return TEXTPARSER_JSON_NESTED_TOKENS_NOT_ARRAY;
                }

                size_t nested_cnt = json_object_array_length(nested_tokens_json);
                // Allow empty array? Existing code checked if empty.
                // Assuming empty is fine, but code returned TEXTPARSER_JSON_NESTED_TOKENS_IS_EMPTY.
                // We keep original behavior for error code, but maybe empty array is valid (just no children).
                if (nested_cnt == 0) {
                     (*definition)->error_string = "`nested_tokens` array is empty!";
                     return TEXTPARSER_JSON_NESTED_TOKENS_IS_EMPTY;
                }

                (*definition)->tokens[token_idx].nested_tokens = malloc(sizeof(int) * (nested_cnt + 1));
                if (!(*definition)->tokens[token_idx].nested_tokens) {
                     return TEXTPARSER_JSON_OUT_OF_MEMORY;
                }
                
                for(size_t i = 0; i < nested_cnt; i++) {
                     json_object *item = json_object_array_get_idx(nested_tokens_json, i);
                     const char *name = json_object_get_string(item);
                     
                     int found_idx = TextParser_END;
                     for(size_t j = 0; j < tokens_cnt; j++) {
                         if ((*definition)->tokens[j].name && strcmp((*definition)->tokens[j].name, name) == 0) {
                             found_idx = (int)j;
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
    size_t starts_with_cnt = json_object_array_length(start_tokens_arr);
    if (starts_with_cnt == 0) {
        (*definition)->error_string = "`startTokens` array is empty!";
        return TEXTPARSER_JSON_STARTS_WITH_IS_EMPTY;
    }

    (*definition)->starts_with = malloc(sizeof(int) * (starts_with_cnt + 1));
    if ((*definition)->starts_with == nullptr) {
        return TEXTPARSER_JSON_OUT_OF_MEMORY;
    }

    for(size_t i = 0; i < starts_with_cnt; i++) {
        json_object *item = json_object_array_get_idx(start_tokens_arr, i);
        const char *name = json_object_get_string(item);
        
        int found_idx = TextParser_END;
        for(size_t j = 0; j < tokens_cnt; j++) {
             if ((*definition)->tokens[j].name && strcmp((*definition)->tokens[j].name, name) == 0) {
                 found_idx = (int)j;
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
    (*definition)->tokens[tokens_cnt].name = nullptr;

    return 0;
}

int textparser_json_load_language_definition_from_json_file(const char *pathname, textparser_language_definition **definition)
{
    return textparser_json_load_language_definition_internal(json_object_from_file(pathname), definition);
}

int textparser_json_load_language_definition_from_string(const char *text, textparser_language_definition **definition)
{
    return textparser_json_load_language_definition_internal(json_tokener_parse(text), definition);
}
