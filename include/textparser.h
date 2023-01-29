#pragma once

#include <stdbool.h>
#include <stdint.h>


#ifdef libtextparser_EXPORTS
 #if defined(_MSC_VER)
  #define EXPORT_CFRDS __declspec(dllexport)
 #else
  #define EXPORT_CFRDS __attribute__((visibility("default")))
 #endif
#else
 #define EXPORT_CFRDS
#endif

#define TextParser_END -1

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
    bool can_have_other_text_inside;
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
    bool can_have_other_text_inside;
    textparse_token_t *tokens;
} language_definition_t;

#ifdef __cplusplus
extern "C"
{
#endif

EXPORT_CFRDS int textparse_openfile(const char *pathname, int text_format, void **handle);
EXPORT_CFRDS int textparse_openmem(const char *text, int len, int text_format, void **handle);
EXPORT_CFRDS void textparse_close(void *handle);

EXPORT_CFRDS int textparse_parse(void *handle, const language_definition_t *definition);

EXPORT_CFRDS textparse_token_item_t *textparse_get_first_token(void *handle);
EXPORT_CFRDS const char *textparse_get_token_id_name(void *handle, int token_id);
EXPORT_CFRDS char *textparse_get_token_text(void *handle, textparse_token_item_t *item);

#ifdef __cplusplus
}
#endif
