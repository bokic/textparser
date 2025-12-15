#pragma once

#include <adv_regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


#ifdef libtextparser_EXPORTS
 #if defined(_MSC_VER)
  #define EXPORT_TEXTPARSER __declspec(dllexport)
 #else
  #define EXPORT_TEXTPARSER __attribute__((visibility("default")))
 #endif
#else
 #define EXPORT_TEXTPARSER
#endif

#define TextParser_END (-1)

#define textparser_defer(var) textparser_t var __attribute__((cleanup(textparser_cleanup))) = nullptr

typedef void* textparser_t;

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
#ifdef DEBUG_TEXTPARSER
    char *debug_token_name;
    char *debug_text;
#endif
    uint32_t text_color;
    uint32_t text_background;
    uint32_t text_flags;
    const char *error;
} textparser_token_item;

 typedef struct {
    const char *name;
    enum textparser_token_type type;
    const char *start_regex;
    const char *end_regex;
    bool other_text_inside;
    bool delete_if_only_one_child;
    bool must_have_one_child;
    bool multi_line;
    bool search_parent_end_token_last;
    uint32_t text_color;
    uint32_t text_background;
    uint32_t text_flags;
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

EXPORT_TEXTPARSER int textparser_openfile(const char *pathname, int text_format, textparser_t *handle);
EXPORT_TEXTPARSER int textparser_openmem(const char *text, int len, int text_format, textparser_t *handle);
EXPORT_TEXTPARSER void textparser_close(textparser_t handle);
EXPORT_TEXTPARSER void textparser_cleanup(textparser_t *handle);

EXPORT_TEXTPARSER void textparser_dump(textparser_t handle);

EXPORT_TEXTPARSER int textparser_parse(textparser_t handle, const language_definition *definition);

EXPORT_TEXTPARSER const char *textparser_get_text(textparser_t handle);
EXPORT_TEXTPARSER size_t textparser_get_text_size(textparser_t handle);
EXPORT_TEXTPARSER textparser_token_item *textparser_get_first_token(const textparser_t handle);
EXPORT_TEXTPARSER const char *textparser_get_token_id_name(const textparser_t handle, int token_id);
EXPORT_TEXTPARSER char *textparser_get_token_text(const textparser_t handle, const textparser_token_item *item);
EXPORT_TEXTPARSER const language_definition *textparser_get_language(const textparser_t handle);


EXPORT_TEXTPARSER textparser_parser_state *textparser_parse_state_new(textparser_t state, int size);
EXPORT_TEXTPARSER void textparser_parse_state_free(textparser_parser_state *state);
//EXPORT_TEXTPARSER textparser_line_parser_item *textparser_parse_line(const char *line, enum adv_regex_encoding text_format, textparser_parser_state *state, const language_definition *definition);

#ifdef __cplusplus
}
#endif
