#pragma once

#include <adv_regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


#ifdef libtextparser_EXPORTS
 #if defined(_MSC_VER)
  #define EXPORT_TEXT_PARSER __declspec(dllexport)
 #else
  #define EXPORT_TEXT_PARSER __attribute__((visibility("default")))
 #endif
#else
 #define EXPORT_TEXT_PARSER
#endif

#define TextParser_END (-1)

enum textparser_token_type {
    TEXTPARSER_TOKEN_TYPE_GROUP,
    TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER,
    TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY,
    TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
    TEXTPARSER_TOKEN_TYPE_START_STOP,
    TEXTPARSER_TOKEN_TYPE_START_OPT_STOP,
    TEXTPARSER_TOKEN_TYPE_DUAL_START_AND_STOP
};

typedef struct {
    int allocated;
    int len;
    int state[];
} textparser_parser_state;

typedef struct {
    const char *text;
    size_t len;
    int type;
} textparser_line_parser_item;

typedef struct textparser_token_item {
    struct textparser_token_item *next;
    struct textparser_token_item *child;
    int token_id;
    size_t position;
    size_t len;
    const char *error;
} textparser_token_item;

 typedef struct {
    const char *name;
    enum textparser_token_type type;
    const char *start_string;
    const char *end_string;
    bool immediate_start;
    bool delete_if_only_one_child;
    bool must_have_one_child;
    bool multi_line;
    int *nested_tokens;
} textparser_token;

typedef struct {
    const char *name;
    float version;
    const char *empty_segment_language;
    bool case_sensitivity;
    const char **default_file_extensions;
    int default_text_encoding;
    int *starts_with;
    bool other_text_inside;
    textparser_token *tokens;
    const char *error_string;
} language_definition;

#ifdef __cplusplus
extern "C"
{
#endif

EXPORT_TEXT_PARSER int textparser_load_language_definition_from_json_file(const char *pathname, language_definition **definition);
EXPORT_TEXT_PARSER int textparser_load_language_definition_from_string(const char *text, language_definition **definition);
EXPORT_TEXT_PARSER void textparser_free_language_definition(language_definition *definition);

EXPORT_TEXT_PARSER int textparser_openfile(const char *pathname, int text_format, void **handle);
EXPORT_TEXT_PARSER int textparser_openmem(const char *text, int len, int text_format, void **handle);
EXPORT_TEXT_PARSER void textparser_close(void *handle);

EXPORT_TEXT_PARSER void textparser_dump(void *handle);

EXPORT_TEXT_PARSER int textparser_parse(void *handle, const language_definition *definition);

EXPORT_TEXT_PARSER textparser_token_item *textparser_get_first_token(void *handle);
EXPORT_TEXT_PARSER const char *textparser_get_token_id_name(void *handle, int token_id);
EXPORT_TEXT_PARSER char *textparser_get_token_text(void *handle, textparser_token_item *item);

EXPORT_TEXT_PARSER textparser_parser_state *textparser_parse_state_new(void *state, int size);
EXPORT_TEXT_PARSER void textparser_parse_state_free(textparser_parser_state *state);
EXPORT_TEXT_PARSER textparser_line_parser_item *textparser_parse_line(const char *line, enum adv_regex_encoding text_format, textparser_parser_state *state, const language_definition *definition);

#ifdef __cplusplus
}
#endif
