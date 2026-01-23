#pragma once

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

#define TextParser_START (0)
#define TextParser_END (-1)

#define TEXTPARSER_NOCOLOR 0xffffffff

#define textparser_defer(var) textparser_t var __attribute__((cleanup(textparser_cleanup))) = nullptr
#define textparser_parser_state_defer(var) textparser_parser_state *var __attribute__((cleanup(textparser_state_cleanup))) = nullptr

enum textparser_encoding { TEXTPARSER_ENCODING_ERROR, TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_ENCODING_UTF_8, TEXTPARSER_ENCODING_UNICODE, TEXTPARSER_ENCODING_UTF_16, TEXTPARSER_ENCODING_UTF_32 };

enum textparser_token_type {
    TEXTPARSER_TOKEN_TYPE_GROUP,
    TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER,
    TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY,
    TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
    TEXTPARSER_TOKEN_TYPE_START_STOP,
    TEXTPARSER_TOKEN_TYPE_START_OPT_STOP,
    TEXTPARSER_TOKEN_TYPE_DUAL_START_AND_STOP
};

typedef void* textparser_t;
typedef void* textparser_token_item_t;

typedef struct {
    int len;
    int state[];
} textparser_parser_state;

typedef struct textparser_token_item {
    struct textparser_token_item *next;
    struct textparser_token_item *child;
    int token_id;
    size_t position;
    size_t len;
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
} textparser_language_definition;

#ifdef __cplusplus
extern "C"
{
#endif

EXPORT_TEXTPARSER int textparser_openfile(const char *pathname, int default_text_format, textparser_t *handle);
EXPORT_TEXTPARSER int textparser_openmem(const char *text, int len, int text_format, textparser_t *handle);
EXPORT_TEXTPARSER void textparser_close(textparser_t handle);
EXPORT_TEXTPARSER void textparser_cleanup(textparser_t *handle);

EXPORT_TEXTPARSER int textparser_parse(textparser_t handle, const textparser_language_definition *definition);
EXPORT_TEXTPARSER const char *textparser_parse_error(textparser_t handle);

EXPORT_TEXTPARSER const char *textparser_get_text(textparser_t handle);
EXPORT_TEXTPARSER size_t textparser_get_text_size(textparser_t handle);
EXPORT_TEXTPARSER textparser_token_item *textparser_get_first_token(const textparser_t handle);
EXPORT_TEXTPARSER char *textparser_get_token_text(const textparser_t handle, const textparser_token_item *item);
EXPORT_TEXTPARSER const textparser_language_definition *textparser_get_language(const textparser_t handle);

EXPORT_TEXTPARSER size_t textparser_get_token_children_count(const textparser_token_item *token);
EXPORT_TEXTPARSER const textparser_token_item *textparser_get_token_child(const textparser_token_item *token);
EXPORT_TEXTPARSER const textparser_token_item *textparser_get_token_next(const textparser_token_item *token);
EXPORT_TEXTPARSER const char *textparser_get_token_type_str(const textparser_language_definition *language, const textparser_token_item *token);
EXPORT_TEXTPARSER int textparser_get_token_type(const textparser_token_item *token);
EXPORT_TEXTPARSER int textparser_get_token_position(const textparser_token_item *token);
EXPORT_TEXTPARSER int textparser_get_token_length(const textparser_token_item *token);
EXPORT_TEXTPARSER uint32_t textparser_get_token_text_color(const textparser_token_item *token);
EXPORT_TEXTPARSER uint32_t textparser_get_token_text_background(const textparser_token_item *token);
EXPORT_TEXTPARSER uint32_t textparser_get_token_text_flags(const textparser_token_item *token);
EXPORT_TEXTPARSER const char *textparser_get_token_error(const textparser_token_item *token);

EXPORT_TEXTPARSER textparser_parser_state *textparser_state_new(const textparser_t handle);
EXPORT_TEXTPARSER void textparser_state_free(textparser_parser_state *state);
EXPORT_TEXTPARSER void textparser_state_cleanup(textparser_parser_state **state);

#ifdef __cplusplus
}
#endif
