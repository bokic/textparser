#pragma once

#include <stdbool.h>
#include <stdint.h>


#ifdef libtextparser_EXPORTS
 #if defined(_MSC_VER)
  #define EXPORT_TEXT_PARSER __declspec(dllexport)
 #else
  #define EXPORT_TEXT_PARSER __attribute__((visibility("default")))
 #endif
#else
 #define EXPORT_TEXT_PARSER
#endif

#define TextParser_END -1

enum textparse_text_format {
    TEXTPARSE_LATIN_1,
    TEXTPARSE_UTF_8,
    TEXTPARSE_UNICODE
};

typedef struct {
    int allocated;
    int len;
    int state[];
} textparse_parser_state;

typedef struct {
    char *text;
    int len;
    int type;
} textparse_line_parser_item;

typedef struct textparse_token_item {
    struct textparse_token_item *next;
    struct textparse_token_item *child;
    int token_id;
    int position;
    int len;
    const char *error;
} textparse_token_item;

 typedef struct {
    const char *name;
    const char *start_string;
    const char *end_string;
    bool only_start_tag;
    bool multi_line;
    bool can_have_other_text_inside;
    bool end_tag_is_optional;
    int *nested_tokens;
} textparse_token;

typedef struct {
    const char *name;
    float version;
    const char *empty_segment_language;
    bool case_sensitivity;
    const char **default_file_extensions;
    int default_text_encoding;
    int *starts_with;
    bool other_text_inside;
    textparse_token *tokens;
    const char *error_string;
} language_definition;

#ifdef __cplusplus
extern "C"
{
#endif

EXPORT_TEXT_PARSER int textparse_load_language_definition_from_json_file(const char *pathname, language_definition **definition);
EXPORT_TEXT_PARSER int textparse_load_language_definition_from_string(const char *text, language_definition **definition);
EXPORT_TEXT_PARSER void textparse_free_language_definition(language_definition *definition);

EXPORT_TEXT_PARSER int textparse_openfile(const char *pathname, int text_format, void **handle);
EXPORT_TEXT_PARSER int textparse_openmem(const char *text, int len, int text_format, void **handle);
EXPORT_TEXT_PARSER void textparse_close(void *handle);

EXPORT_TEXT_PARSER void textparse_dump(void *handle);

EXPORT_TEXT_PARSER int textparse_parse(void *handle, const language_definition *definition);

EXPORT_TEXT_PARSER textparse_token_item *textparse_get_first_token(void *handle);
EXPORT_TEXT_PARSER const char *textparse_get_token_id_name(void *handle, int token_id);
EXPORT_TEXT_PARSER char *textparse_get_token_text(void *handle, textparse_token_item *item);

EXPORT_TEXT_PARSER textparse_parser_state *textparse_parse_state_new(void *state, int size);
EXPORT_TEXT_PARSER void textparse_parse_state_free(textparse_parser_state *state);
EXPORT_TEXT_PARSER textparse_line_parser_item *textparse_parse_line(const char *line, enum textparse_text_format text_format, textparse_parser_state *state, const language_definition *definition);

#ifdef __cplusplus
}
#endif
