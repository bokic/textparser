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
#define MAX_RECURSION_DEPTH 1000

#define TEXTPARSER_TOKEN_ID_ERROR (-1)
#define TEXTPARSER_NOCOLOR 0xffffffff

#if defined(_MSC_VER) && !defined(__clang__)
#error "MSVC compiler is not supported by textparser (requires GCC/Clang extensions like __attribute__((cleanup)))"
#endif

#define textparser_defer(var) textparser_t var __attribute__((cleanup(textparser_cleanup))) = nullptr
#define textparser_parser_state_defer(var) textparser_parser_state *var __attribute__((cleanup(textparser_state_cleanup))) = nullptr

enum textparser_encoding { TEXTPARSER_ENCODING_NONE, TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_ENCODING_UTF_8, TEXTPARSER_ENCODING_UNICODE, TEXTPARSER_ENCODING_UTF_16, TEXTPARSER_ENCODING_UTF_32 };

enum textparser_bom {
    TEXTPARSER_BOM_NONE         =       0,
    TEXTPARSER_BOM_UTF_8        = (1 << 0),
    TEXTPARSER_BOM_UTF_16_BE    = (1 << 1),
    TEXTPARSER_BOM_UTF_16_LE    = (1 << 2),
    TEXTPARSER_BOM_UTF_32_BE    = (1 << 3),
    TEXTPARSER_BOM_UTF_32_LE    = (1 << 4),
    TEXTPARSER_BOM_UTF_7_1      = (1 << 5),
    TEXTPARSER_BOM_UTF_7_2      = (1 << 6),
    TEXTPARSER_BOM_UTF_7_3      = (1 << 7),
    TEXTPARSER_BOM_UTF_7_4      = (1 << 8),
    TEXTPARSER_BOM_UTF_7_5      = (1 << 9),
    TEXTPARSER_BOM_UTF_1        = (1 << 10),
    TEXTPARSER_BOM_UTF_EBCDIC   = (1 << 11),
    TEXTPARSER_BOM_UTF_SCSU     = (1 << 12),
    TEXTPARSER_BOM_UTF_BOCU1    = (1 << 13),
    TEXTPARSER_BOM_UTF_GB_18030 = (1 << 14),
};

#define TEXTPARSER_BOM_ALL (TEXTPARSER_BOM_UTF_8 | TEXTPARSER_BOM_UTF_16_BE | TEXTPARSER_BOM_UTF_16_LE | TEXTPARSER_BOM_UTF_32_BE | TEXTPARSER_BOM_UTF_32_LE | TEXTPARSER_BOM_UTF_7_1 | TEXTPARSER_BOM_UTF_7_2 | TEXTPARSER_BOM_UTF_7_3 | TEXTPARSER_BOM_UTF_7_4 | TEXTPARSER_BOM_UTF_7_5 | TEXTPARSER_BOM_UTF_1 | TEXTPARSER_BOM_UTF_EBCDIC | TEXTPARSER_BOM_UTF_SCSU | TEXTPARSER_BOM_UTF_BOCU1 | TEXTPARSER_BOM_UTF_GB_18030)

enum textparser_validation_item_type {
    TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR,
    TEXTPARSER_VALIDATION_ITEM_TYPE_WARNING,
    TEXTPARSER_VALIDATION_ITEM_TYPE_INFO,
};

enum textparser_callback_type {
    TEXTPARSER_CALLBACK_TYPE_START,
    TEXTPARSER_CALLBACK_TYPE_END,
};

enum textparser_token_type {
    TEXTPARSER_TOKEN_TYPE_GROUP,
    TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER,
    TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY,
    TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
    TEXTPARSER_TOKEN_TYPE_START_STOP,
    TEXTPARSER_TOKEN_TYPE_START_OPT_STOP
};

typedef struct textparser_handle *textparser_t;
typedef struct textparser_token_item_s *textparser_token_item_t;

typedef struct {
    size_t len;
    const struct textparser_token_item *state[];
} textparser_parser_state;

typedef struct textparser_token_item {
    struct textparser_token_item *prev;
    struct textparser_token_item *next;
    struct textparser_token_item *child;
    struct textparser_token_item *parent;
    int token_id;
    size_t position;
    size_t len;
    uint32_t text_color;
    uint32_t text_background;
    uint32_t text_flags;
    const char *error;
} textparser_token_item;

 typedef struct {
    int *when_parent_in;
    int *nested_tokens;
} textparser_context_nested_tokens;

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
    textparser_context_nested_tokens *context_nested_tokens;
} textparser_token;

typedef struct {
    const char **file_extensions;
    const char *regex;
    int *start_tokens;
} textparser_override_start_token_rule;

typedef struct {
    const char *name;
    double version;
    const char *empty_segment_language;
    bool case_sensitivity;
    const char **default_file_extensions;
    int default_text_encoding;
    int supported_bom;
    int *starts_with;
    textparser_override_start_token_rule *override_start_tokens;
    bool other_text_inside;
    bool sign_ambiguity_fix;
    int token_number_id;
    int token_operator_id;
    textparser_token *tokens;
    const char *error_string;
} textparser_language_definition;


typedef struct {
    enum textparser_validation_item_type type;
    size_t position;
    size_t length;
    char *text;
} textparser_validation_item;

typedef struct {
    int len;
    textparser_validation_item *items[];
} textparser_validation;

#ifdef __cplusplus
extern "C"
{
#endif

EXPORT_TEXTPARSER int textparser_openfile(const char *pathname, int default_text_format, int bom_mask, textparser_t *handle);
EXPORT_TEXTPARSER int textparser_openmem(const char *text, int len, int text_format, textparser_t *handle);
EXPORT_TEXTPARSER void textparser_set_filename(textparser_t handle, const char *filename);
EXPORT_TEXTPARSER const char *textparser_get_filename(const textparser_t handle);
EXPORT_TEXTPARSER void textparser_close(textparser_t handle);
EXPORT_TEXTPARSER void textparser_cleanup(textparser_t *handle);

EXPORT_TEXTPARSER int textparser_parse(textparser_t handle, const textparser_language_definition *definition);
EXPORT_TEXTPARSER int textparser_parse_incremental(textparser_t handle, const textparser_language_definition *definition, textparser_parser_state *state, size_t start_pos, size_t end_pos);
EXPORT_TEXTPARSER const char *textparser_parse_error(textparser_t handle);
EXPORT_TEXTPARSER size_t textparser_parse_error_position(textparser_t handle);

EXPORT_TEXTPARSER void textparser_set_callback(textparser_t handle, void (*callback)(textparser_t, textparser_token_item *, enum textparser_callback_type callback_type, void *user_data), void *user_data);
EXPORT_TEXTPARSER const char *textparser_get_text(textparser_t handle);
EXPORT_TEXTPARSER size_t textparser_get_text_size(textparser_t handle);
EXPORT_TEXTPARSER textparser_token_item *textparser_get_first_token(const textparser_t handle);
EXPORT_TEXTPARSER char *textparser_get_token_text(const textparser_t handle, const textparser_token_item *item);
EXPORT_TEXTPARSER uint16_t *textparser_get_token_text16(const textparser_t handle, const textparser_token_item *item);
EXPORT_TEXTPARSER uint32_t *textparser_get_token_text32(const textparser_t handle, const textparser_token_item *item);
EXPORT_TEXTPARSER void textparser_free_token_text(void *text);
EXPORT_TEXTPARSER const textparser_language_definition *textparser_get_language(const textparser_t handle);
EXPORT_TEXTPARSER void textparser_free_language_definition(textparser_language_definition *definition);

EXPORT_TEXTPARSER size_t textparser_get_token_children_count(const textparser_token_item *token);
EXPORT_TEXTPARSER const textparser_token_item *textparser_get_token_child(const textparser_token_item *token);
EXPORT_TEXTPARSER const textparser_token_item *textparser_get_token_next(const textparser_token_item *token);
EXPORT_TEXTPARSER const textparser_token_item *textparser_get_token_prev(const textparser_token_item *token);
EXPORT_TEXTPARSER const char *textparser_get_token_type_str(const textparser_language_definition *language, const textparser_token_item *token);
EXPORT_TEXTPARSER int textparser_get_token_type(const textparser_token_item *token);
EXPORT_TEXTPARSER size_t textparser_get_token_position(const textparser_token_item *token);
EXPORT_TEXTPARSER size_t textparser_get_token_length(const textparser_token_item *token);
EXPORT_TEXTPARSER uint32_t textparser_get_token_text_color(const textparser_token_item *token);
EXPORT_TEXTPARSER uint32_t textparser_get_token_text_background(const textparser_token_item *token);
EXPORT_TEXTPARSER uint32_t textparser_get_token_text_flags(const textparser_token_item *token);
EXPORT_TEXTPARSER const char *textparser_get_token_error(const textparser_token_item *token);

EXPORT_TEXTPARSER int textparser_build_line_map(textparser_t handle);
EXPORT_TEXTPARSER size_t textparser_get_line_count(const textparser_t handle);
EXPORT_TEXTPARSER size_t textparser_get_line_start_position(const textparser_t handle, size_t line_index);
EXPORT_TEXTPARSER size_t textparser_get_line_number_at_position(const textparser_t handle, size_t position);

EXPORT_TEXTPARSER textparser_parser_state *textparser_state_new(const textparser_t handle);
EXPORT_TEXTPARSER textparser_parser_state *textparser_state_generate(const textparser_t handle, size_t position);
EXPORT_TEXTPARSER void textparser_state_free(textparser_parser_state *state);
EXPORT_TEXTPARSER void textparser_state_cleanup(textparser_parser_state **state);

EXPORT_TEXTPARSER const textparser_token_item **textparser_query(const textparser_t handle, const textparser_token_item *root, const char *selector, size_t *out_count);
EXPORT_TEXTPARSER void textparser_free_query_result(const textparser_token_item **results);

#ifdef __cplusplus
}
#endif
