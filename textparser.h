#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct textparse_token {
    char *name;
    char *start_string;
    char *end_string;
    bool only_start_tag;
    bool multi_line;

} textparse_token_t;

typedef struct language_definition {
    char *name;
    char *empty_segment_language;
    bool case_sensitivity;
    char **default_file_extensions;
    char **starts_with;
    textparse_token_t **tokens;
} language_definition_t;

int textparse_parsefile(const char *pathname, void **handle);
void textparse_close(void *handle);
