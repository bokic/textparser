#include <textparser-json.h>

#include <json-c/json.h>
#include <sys/stat.h>
#include <sys/mman.h>
//#include <string.h>
//#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <assert.h>
//#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>


static int textparser_json_load_language_definition_internal(struct json_object *root_obj, language_definition **definition)
{
    int ret = 0;

    // TODO: Implement textparser_load_language_definition_internal()
    /*size_t array_length = 0;
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
    array_length = json_object_array_length(value);
    if (array_length > 0) {
        (*definition)->default_file_extensions = malloc((array_length + 1) * sizeof(char *));

        for(size_t i = 0; i < array_length; i++) {
            json_object *array_item = json_object_array_get_idx(value, i);
            (*definition)->default_file_extensions[i] = strdup(json_object_get_string(array_item));
        }

        (*definition)->default_file_extensions[array_length] = NULL;
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

            json_object_object_get_ex(token_item, "start_regex", &key_value);
            other_name = json_object_get_string(key_value);
            (*definition)->tokens[token_idx].start_regex = strdup(other_name);

            json_object_object_get_ex(token_item, "end_regex", &key_value);
            other_name = json_object_get_string(key_value);
            (*definition)->tokens[token_idx].end_regex = strdup(other_name);

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
    }*/

    return ret;
}

int textparser_json_load_language_definition_from_json_file(const char *pathname, language_definition **definition)
{
    return textparser_json_load_language_definition_internal(json_object_from_file(pathname), definition);
}

int textparser_json_load_language_definition_from_string(const char *text, language_definition **definition)
{
    return textparser_json_load_language_definition_internal(json_tokener_parse(text), definition);
}
