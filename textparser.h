#pragma once

#include <stdbool.h>
#include <stdint.h>

enum textparse_text_format {
    TEXTPARSE_LATIN_1,
    TEXTPARSE_UTF_8,
    TEXTPARSE_UNICODE
};

typedef struct textparse_token_item {
    struct textparse_token_item *next;
    struct textparse_token_item *child;
    int token_id;
    int position;
    int len;
    const char *error;
} textparse_token_item_t;

typedef struct textparse_token {
    char *name;
    char *start_string;
    char *end_string;
    bool only_start_tag;
    bool multi_line;
    bool has_stuff_inside;
    bool end_tag_is_optional;
    int *nested_tokens;
} textparse_token_t;

typedef struct language_definition {
    char *name;
    float version;
    char *empty_segment_language;
    bool case_sensitivity;
    char **default_file_extensions;
    int default_text_encoding;
    int *starts_with;
    bool has_stuff_inside;
    textparse_token_t *tokens;
} language_definition_t;

int textparse_openfile(const char *pathname, int text_format, void **handle);
void textparse_close(void *handle);

int textparse_parse(void *handle, const language_definition_t *definition);
