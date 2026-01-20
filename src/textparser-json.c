#include <textparser-json.h>
#include <adv_regex.h>

#include <json-c/json.h>
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

    found = json_object_object_get_ex(root_obj, "empty_segment_language", &value);
    if (found)
        (*definition)->empty_segment_language = strdup(json_object_get_string(value));

    found = json_object_object_get_ex(root_obj, "case_sensitivity", &value);
    if (!found) {
        (*definition)->error_string = "Mandatory field `case_sensitivity` not set!";
        return TEXTPARSER_JSON_CASE_SENSITIVITY_NOT_FOUND;
    }
    (*definition)->case_sensitivity = json_object_get_boolean(value);

    found = json_object_object_get_ex(root_obj, "file_extensions", &value);
    if (!found) {
        (*definition)->error_string = "Mandatory field `file_extensions` not set!";
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

    found = json_object_object_get_ex(root_obj, "encoding", &value);
    if (found) {
        const char *encoding = json_object_get_string(value);
        if(strcmp(encoding, "latin1") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_ENCODING_LATIN1;
        else if(strcmp(encoding, "utf8") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_ENCODING_UTF_8;
        else if(strcmp(encoding, "unicode") == 0)
            (*definition)->default_text_encoding = TEXTPARSER_ENCODING_UNICODE;
        else {
            (*definition)->error_string = "Invalid `encoding` encoding! Should be one of the following: latin1, utf8, unicode.";
            return TEXTPARSER_JSON_ENCODING_NOT_FOUND;
        }
    }
    else
    {
        (*definition)->default_text_encoding = TEXTPARSER_ENCODING_LATIN1;
    }

    found = json_object_object_get_ex(root_obj, "starts_with", &value);
    if (!found) {
        (*definition)->error_string = "Mandatory field `starts_with` is missing!";
        return TEXTPARSER_JSON_STARTS_WITH_NOT_FOUND;
    }

    if (!json_object_is_type(value, json_type_array)) {
        (*definition)->error_string = "`starts_with` is not array!";
        return TEXTPARSER_JSON_STARTS_WITH_NOT_ARRAY;
    }

    found = json_object_object_get_ex(root_obj, "tokens", &tokens);
    if (!found) {
        (*definition)->error_string = "Mandatory field `tokens` is missing!";
        return TEXTPARSER_JSON_TOKENS_NOT_FOUND;
    }

    if (!json_object_is_type(tokens, json_type_array)) {
        (*definition)->error_string = "`tokens` is not array!";
        return TEXTPARSER_JSON_TOKENS_NOT_ARRAY;
    }

    size_t tokens_cnt = json_object_array_length(tokens);

    size_t starts_with_cnt = json_object_array_length(value);
    if (starts_with_cnt == 0) {
        (*definition)->error_string = "`starts_with` array is empty!";
        return TEXTPARSER_JSON_STARTS_WITH_IS_EMPTY;
    }

    (*definition)->starts_with = malloc(sizeof(int) * (starts_with_cnt + 1));
    if ((*definition)->starts_with == nullptr) {
        return TEXTPARSER_JSON_OUT_OF_MEMORY;
    }

    memset((*definition)->starts_with, 0, sizeof(int) * starts_with_cnt);
    (*definition)->starts_with[starts_with_cnt] = TextParser_END;

    for(size_t c = 0; c < starts_with_cnt; c++) {
        json_object *array_item = json_object_array_get_idx(value, c);

        if (!json_object_is_type(array_item, json_type_string)) {
            (*definition)->error_string = "`starts_with` element not string!";
            return TEXTPARSER_JSON_STARTS_WITH_ELEMENT_NOT_STRING;
        }

        const char *name = json_object_get_string(array_item);

        for(size_t c2 = 0; c2 < tokens_cnt; c2++) {
            json_object *token_item = json_object_array_get_idx(tokens, c2);

            struct json_object *key_value = nullptr;
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
    if ((*definition)->tokens == nullptr) {
        (*definition)->error_string = "malloc for tokens list FAILED!";
        return TEXTPARSER_JSON_OUT_OF_MEMORY;
    }

    memset((*definition)->tokens, 0, sizeof(textparser_token) * (tokens_cnt + 1));

    if (tokens_cnt > 0) {
        for(size_t token_idx = 0; token_idx < tokens_cnt; token_idx++) {
            json_object *token_item = json_object_array_get_idx(tokens, token_idx);

            struct json_object *key_value = nullptr;
            const char *other_name = nullptr;

            json_object_object_get_ex(token_item, "name", &key_value);
            other_name = json_object_get_string(key_value);
            (*definition)->tokens[token_idx].name = strdup(other_name);

            json_object_object_get_ex(token_item, "start_regex", &key_value);
            other_name = json_object_get_string(key_value);
            (*definition)->tokens[token_idx].start_regex = strdup(other_name);

            json_object_object_get_ex(token_item, "end_regex", &key_value);
            other_name = json_object_get_string(key_value);
            (*definition)->tokens[token_idx].end_regex = strdup(other_name);

            json_object_object_get_ex(token_item, "other_text_inside", &key_value);
            if (key_value)
                (*definition)->tokens[token_idx].other_text_inside = json_object_get_boolean(key_value);
            else
                (*definition)->tokens[token_idx].other_text_inside = false;

            json_object_object_get_ex(token_item, "delete_if_only_one_child", &key_value);
            if (key_value)
                (*definition)->tokens[token_idx].delete_if_only_one_child = json_object_get_boolean(key_value);
            else
                (*definition)->tokens[token_idx].delete_if_only_one_child = false;

            json_object_object_get_ex(token_item, "must_have_one_child", &key_value);
            if (key_value)
                (*definition)->tokens[token_idx].must_have_one_child = json_object_get_boolean(key_value);
            else
                (*definition)->tokens[token_idx].must_have_one_child = false;

            json_object_object_get_ex(token_item, "multi_line", &key_value);
            if (key_value)
                (*definition)->tokens[token_idx].multi_line = json_object_get_boolean(key_value);
            else
                (*definition)->tokens[token_idx].multi_line = false;

            json_object_object_get_ex(token_item, "search_parent_end_token_last", &key_value);
            if (key_value)
                (*definition)->tokens[token_idx].search_parent_end_token_last = json_object_get_boolean(key_value);
            else
                (*definition)->tokens[token_idx].search_parent_end_token_last = false;

            json_object_object_get_ex(token_item, "text_color", &key_value);
            if (key_value)
                (*definition)->tokens[token_idx].text_color = json_object_get_int(key_value);
            else
                (*definition)->tokens[token_idx].text_color = TEXTPARSER_NOCOLOR;

            json_object_object_get_ex(token_item, "text_background", &key_value);
            if (key_value)
                (*definition)->tokens[token_idx].text_background = json_object_get_int(key_value);
            else
                (*definition)->tokens[token_idx].text_background = TEXTPARSER_NOCOLOR;

            json_object_object_get_ex(token_item, "text_flags", &key_value);
            if (key_value)
                (*definition)->tokens[token_idx].text_flags = json_object_get_int(key_value);
            else
                (*definition)->tokens[token_idx].text_flags = 0;

            json_object_object_get_ex(token_item, "nested_tokens", &key_value);
            if (key_value) {
                if (!json_object_is_type(key_value, json_type_array)) {
                    (*definition)->error_string = "`nested_tokens` is not array!";
                    return TEXTPARSER_JSON_NESTED_TOKENS_NOT_ARRAY;
                }

                size_t nested_tokens_cnt = json_object_array_length(key_value);
                if (nested_tokens_cnt == 0) {
                    (*definition)->error_string = "`nested_tokens` array is empty!";
                    return TEXTPARSER_JSON_NESTED_TOKENS_IS_EMPTY;
                }

                (*definition)->tokens[token_idx].nested_tokens = malloc(sizeof(int) * (nested_tokens_cnt + 1));
                if ((*definition)->tokens[token_idx].nested_tokens == nullptr) {
                    return TEXTPARSER_JSON_OUT_OF_MEMORY;
                }

                memset((*definition)->tokens[token_idx].nested_tokens, 0, sizeof(int) * (nested_tokens_cnt + 1));

                for(size_t c2 = 0; token_idx < nested_tokens_cnt; c2++) {
                    json_object *array_item = json_object_array_get_idx(key_value, c2);

                    if (!json_object_is_type(array_item, json_type_string)) {
                        (*definition)->error_string = "`nested_tokens` element not string!";
                        return TEXTPARSER_JSON_NESTED_TOKENS_ELEMENT_NOT_STRING;
                    }

                    const char *name = json_object_get_string(array_item);
                    (*definition)->tokens[token_idx].nested_tokens[c2] = TextParser_END;

                    for(size_t c3 = 0; c3 < tokens_cnt; c3++) {
                        token_item = json_object_array_get_idx(tokens, c3);

                        struct json_object *key_value2 = nullptr;
                        json_object_object_get_ex(token_item, "name", &key_value2);
                        other_name = json_object_get_string(key_value2);

                        if (strcmp(name, other_name) == 0) {
                            (*definition)->tokens[token_idx].nested_tokens[c2] = c3;
                            break;
                        }
                    }
                }

                (*definition)->tokens[token_idx].nested_tokens[nested_tokens_cnt] = TextParser_END;
            } else {
                (*definition)->tokens[token_idx].nested_tokens = nullptr;
            }
        }
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
