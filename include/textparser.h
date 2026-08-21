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
#define TEXTPARSER_TOKEN_ID_UNPROCESSED (-2)
#define TEXTPARSER_NOCOLOR 0xffffffff

#if defined(_MSC_VER) && !defined(__clang__)
#error "MSVC compiler is not supported by textparser (requires GCC/Clang extensions like __attribute__((cleanup)))"
#endif

#if defined(__cplusplus) && !defined(TEXTPARSER_ALLOW_C_HEADER_IN_CPP)
#error "textparser.h is for C compilation only. Please include textparser.hpp when compiling C++ code."
#endif

#define textparser_defer(var) textparser_t var __attribute__((cleanup(textparser_cleanup))) = nullptr
#define textparser_parser_state_defer(var) textparser_parser_state *var __attribute__((cleanup(textparser_state_cleanup))) = nullptr

enum textparser_error {
    TEXTPARSER_OK = 0,
    TEXTPARSER_ERROR_FILE_OPEN = 1,
    TEXTPARSER_ERROR_INVALID_ARGUMENT = 2,
    TEXTPARSER_ERROR_OUT_OF_MEMORY = 3,
    TEXTPARSER_ERROR_PARSE_FAILED = 4,
    TEXTPARSER_ERROR_UNSUPPORTED_BOM = 5,
    TEXTPARSER_ERROR_BYTE_ORDER_CONVERSION = 6,
    TEXTPARSER_ERROR_INVALID_ENCODING = 7,
    TEXTPARSER_ERROR_FILE_TOO_LARGE = 8,
    TEXTPARSER_ERROR_INVALID_UTF16_SIZE = 9,
    TEXTPARSER_ERROR_INVALID_UTF32_SIZE = 10,
};

enum textparser_encoding { TEXTPARSER_ENCODING_NONE, TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_ENCODING_UTF_8, TEXTPARSER_ENCODING_UNICODE, TEXTPARSER_ENCODING_UTF_16, TEXTPARSER_ENCODING_UTF_32 };

enum textparser_bom {
    TEXTPARSER_BOM_NONE         =       0,
    TEXTPARSER_BOM_UTF_8        = (1 << 0),
    TEXTPARSER_BOM_UTF_16_BE    = (1 << 1),
    TEXTPARSER_BOM_UTF_16_LE    = (1 << 2),
    TEXTPARSER_BOM_UTF_32_BE    = (1 << 3),
    TEXTPARSER_BOM_UTF_32_LE    = (1 << 4),
    // TEXTPARSER_BOM_UTF_7_1      = (1 << 5),
    // TEXTPARSER_BOM_UTF_7_2      = (1 << 6),
    // TEXTPARSER_BOM_UTF_7_3      = (1 << 7),
    // TEXTPARSER_BOM_UTF_7_4      = (1 << 8),
    // TEXTPARSER_BOM_UTF_7_5      = (1 << 9),
    // TEXTPARSER_BOM_UTF_1        = (1 << 10),
    // TEXTPARSER_BOM_UTF_EBCDIC   = (1 << 11),
    // TEXTPARSER_BOM_UTF_SCSU     = (1 << 12),
    // TEXTPARSER_BOM_UTF_BOCU1    = (1 << 13),
    // TEXTPARSER_BOM_UTF_GB_18030 = (1 << 14),
};

#define TEXTPARSER_BOM_ALL (TEXTPARSER_BOM_UTF_8 | TEXTPARSER_BOM_UTF_16_BE | TEXTPARSER_BOM_UTF_16_LE | TEXTPARSER_BOM_UTF_32_BE | TEXTPARSER_BOM_UTF_32_LE)

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
    TEXTPARSER_TOKEN_TYPE_START_OPT_STOP,
    TEXTPARSER_TOKEN_TYPE_SEQUENCE
};

typedef struct textparser_handle *textparser_t;
typedef struct textparser_token_item_s *textparser_token_item_t;

typedef struct {
    size_t len;
    const struct textparser_token_item *state[];
} textparser_parser_state;

typedef struct {
    size_t dirty_start;
    size_t dirty_end;
} textparser_dirty_range;

typedef struct textparser_token_item {
    struct textparser_token_item *prev;
    struct textparser_token_item *next;
    struct textparser_token_item *child;
    struct textparser_token_item *parent;
    int token_id;
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
    const int *sign_tokens;
    const int *number_tokens;
    const int *operand_tokens;
} textparser_sign_merge;

enum textparser_associativity {
    TEXTPARSER_ASSOC_LEFT = 0,
    TEXTPARSER_ASSOC_RIGHT = 1,
};

typedef struct {
    const int *operators;
    enum textparser_associativity associativity;
} textparser_precedence_rule;

typedef struct {
    size_t count;
    const textparser_precedence_rule *rules;
} textparser_operator_precedence;

typedef struct {
    const char *name;
    double version;
    const char *empty_segment_language;
    bool case_sensitivity;
    const char **default_file_extensions;
    enum textparser_encoding default_text_encoding;
    int supported_bom;
    int *starts_with;
    textparser_override_start_token_rule *override_start_tokens;
    bool other_text_inside;
    textparser_sign_merge *sign_merge;
    textparser_operator_precedence *operator_precedence;
    textparser_token *tokens;
    const char *error_string;
    void *string_pool;
} textparser_language_definition;


typedef struct {
    enum textparser_validation_item_type type;
    size_t position;
    size_t length;
    char *text;
} textparser_validation_item;

typedef struct {
    int len;
    int capacity;
    textparser_validation_item *items[];
} textparser_validation;



/**
 * Open and load a text file for parsing.
 *
 * @param pathname Path to the file to open.
 * @param default_text_format Fallback encoding if auto-detection fails.
 * @param bom_mask Bitmask of supported BOM types (e.g. TEXTPARSER_BOM_ALL).
 * @param handle Pointer where the created parser handle will be stored.
 * @return 0 on success, non-zero error code on failure.
 */
EXPORT_TEXTPARSER int textparser_openfile(const char *pathname, int default_text_format, int bom_mask, textparser_t *handle);

/**
 * Create a parser handle from an in-memory string buffer.
 *
 * @param text Pointer to the buffer containing text to parse.
 * @param len Length of text in bytes (-1 for null-terminated strings).
 * @param text_format Encoding format of the text.
 * @param handle Pointer where the created parser handle will be stored.
 * @return 0 on success, non-zero error code on failure.
 */
EXPORT_TEXTPARSER int textparser_openmem(const char *text, int len, int text_format, textparser_t *handle);

/**
 * Update the text buffer associated with an existing parser handle for incremental parsing.
 *
 * @param handle The parser handle.
 * @param text Pointer to the new buffer containing updated text.
 * @param len Length of text in bytes (-1 for null-terminated strings).
 * @return 0 on success, non-zero error code on failure.
 */
EXPORT_TEXTPARSER int textparser_set_text(textparser_t handle, const char *text, int len);

/**
 * Set or update the filename associated with the parser handle.
 *
 * @param handle The parser handle.
 * @param filename Filename string to assign.
 */
EXPORT_TEXTPARSER void textparser_set_filename(textparser_t handle, const char *filename);

/**
 * Get the filename associated with the parser handle.
 *
 * @param handle The parser handle.
 * @return Const pointer to the filename string, or NULL if unset.
 */
EXPORT_TEXTPARSER const char *textparser_get_filename(const textparser_t handle);

/**
 * Close and release resources associated with a parser handle.
 *
 * @param handle The parser handle to close.
 */
EXPORT_TEXTPARSER void textparser_close(textparser_t handle);

/**
 * Cleanup function for auto-cleanup attributes (`textparser_defer`).
 *
 * @param handle Pointer to the parser handle pointer to close/free.
 */
EXPORT_TEXTPARSER void textparser_cleanup(textparser_t *handle);

/**
 * Perform a full document parse using the provided language definition.
 *
 * @param handle The parser handle containing the source text.
 * @param definition Pointer to the language definition rules.
 * @return 0 on success, non-zero error code on syntax/parsing error.
 */
EXPORT_TEXTPARSER int textparser_parse(textparser_t handle, const textparser_language_definition *definition);

/**
 * Perform an incremental document parse for a delta edit.
 * Slices the modified text chunk into the parser's internal buffer, updates
 * the CST for the modified region, and optionally reports the dirty repaint range.
 *
 * @param handle The parser handle.
 * @param definition Pointer to the language definition rules.
 * @param edit_offset Starting unit offset where the edit occurred.
 * @param old_len Length of replaced/deleted text in units.
 * @param new_text Buffer containing the inserted text (or NULL for deletion).
 * @param new_len Length of inserted text in units.
 * @param out_range (Optional) Pointer to receive the dirty repaint range coordinates.
 * @return 0 on success, non-zero error code on failure.
 */
EXPORT_TEXTPARSER int textparser_parse_incremental(textparser_t handle, const textparser_language_definition *definition, size_t edit_offset, size_t old_len, const void *new_text, size_t new_len, textparser_dirty_range *out_range);

/**
 * Perform a 2nd AST post-processing pass to collapse/unwrap container nodes marked
 * with `delete_if_only_one_child` that contain exactly 1 child token.
 *
 * NOTE: This function MUST ONLY be called for full one-time document parses.
 * DO NOT use this function during interactive incremental parsing (`textparser_parse_incremental`),
 * as modifying node pointers invalidates parser state snapshots for subsequent edits.
 *
 * @param root Pointer to the root token item pointer of the AST.
 * @param language Pointer to the language definition rules.
 */
EXPORT_TEXTPARSER void textparser_post_process(textparser_token_item **root, const textparser_language_definition *language);

/**
 * Retrieve the latest parse error message for a handle.
 *
 * @param handle The parser handle.
 * @return Const pointer to error message string, or NULL if no error.
 */
EXPORT_TEXTPARSER const char *textparser_parse_error(textparser_t handle);

/**
 * Retrieve the byte position of the latest parse error.
 *
 * @param handle The parser handle.
 * @return Byte position index of the parse error.
 */
EXPORT_TEXTPARSER size_t textparser_parse_error_position(textparser_t handle);

/**
 * Return a human-readable description for a textparser_error code.
 *
 * @param error_code The error code.
 * @return Const string description of the error.
 */
EXPORT_TEXTPARSER const char *textparser_strerror(int error_code);

/**
 * Set a user callback function to be executed when tokens are matched.
 *
 * @param handle The parser handle.
 * @param callback Callback function pointer.
 * @param user_data Arbitrary pointer passed to the callback.
 */
EXPORT_TEXTPARSER void textparser_set_callback(textparser_t handle, void (*callback)(textparser_t, textparser_token_item *, enum textparser_callback_type callback_type, void *user_data), void *user_data);

/**
 * Get the underlying raw input text buffer from the parser.
 *
 * @param handle The parser handle.
 * @return Const pointer to the text buffer.
 */
EXPORT_TEXTPARSER const char *textparser_get_text(textparser_t handle);

/**
 * Get the total byte size of the input text buffer.
 *
 * @param handle The parser handle.
 * @return Length of text in bytes.
 */
EXPORT_TEXTPARSER size_t textparser_get_text_size(textparser_t handle);

/**
 * Get the root (first) token item of the parsed AST.
 *
 * @param handle The parser handle.
 * @return Pointer to the first textparser_token_item node.
 */
EXPORT_TEXTPARSER textparser_token_item *textparser_get_first_token(const textparser_t handle);

/**
 * Get a dynamically allocated UTF-8 string containing the token's text.
 * Caller must release with `textparser_free_token_text`.
 *
 * @param handle The parser handle.
 * @param item The token item node.
 * @return Allocated string copy of token text, or NULL.
 */
EXPORT_TEXTPARSER char *textparser_get_token_text(const textparser_t handle, const textparser_token_item *item);

/**
 * Get a dynamically allocated UTF-16 string containing the token's text.
 * Caller must release with `textparser_free_token_text`.
 *
 * @param handle The parser handle.
 * @param item The token item node.
 * @return Allocated uint16_t buffer copy of token text, or NULL.
 */
EXPORT_TEXTPARSER uint16_t *textparser_get_token_text16(const textparser_t handle, const textparser_token_item *item);

/**
 * Get a dynamically allocated UTF-32 string containing the token's text.
 * Caller must release with `textparser_free_token_text`.
 *
 * @param handle The parser handle.
 * @param item The token item node.
 * @return Allocated uint32_t buffer copy of token text, or NULL.
 */
EXPORT_TEXTPARSER uint32_t *textparser_get_token_text32(const textparser_t handle, const textparser_token_item *item);

/**
 * Free memory allocated by token text getter functions (`textparser_get_token_text*`).
 *
 * @param text Pointer to text buffer to free.
 */
EXPORT_TEXTPARSER void textparser_free_token_text(void *text);

/**
 * Get the active language definition assigned to the parser.
 *
 * @param handle The parser handle.
 * @return Const pointer to language definition.
 */
EXPORT_TEXTPARSER const textparser_language_definition *textparser_get_language(const textparser_t handle);

/**
 * Free resources allocated for a language definition structure.
 *
 * @param definition Pointer to language definition to free.
 */
EXPORT_TEXTPARSER void textparser_free_language_definition(textparser_language_definition *definition);

/**
 * Count the number of direct child tokens under the given token item.
 *
 * @param token The parent token item node.
 * @return Count of child tokens.
 */
EXPORT_TEXTPARSER size_t textparser_get_token_children_count(const textparser_token_item *token);

/**
 * Get the first child token item of a parent token node.
 *
 * @param token The parent token item node.
 * @return Const pointer to first child token node, or NULL if none.
 */
EXPORT_TEXTPARSER const textparser_token_item *textparser_get_token_child(const textparser_token_item *token);

/**
 * Get the next sibling token item.
 *
 * @param token The current token item node.
 * @return Const pointer to next sibling token node, or NULL if none.
 */
EXPORT_TEXTPARSER const textparser_token_item *textparser_get_token_next(const textparser_token_item *token);

/**
 * Get the previous sibling token item.
 *
 * @param token The current token item node.
 * @return Const pointer to previous sibling token node, or NULL if none.
 */
EXPORT_TEXTPARSER const textparser_token_item *textparser_get_token_prev(const textparser_token_item *token);

/**
 * Get the textual token name/type string defined in the language definition.
 *
 * @param language Language definition structure.
 * @param token The token item node.
 * @return Const pointer to name string, or NULL if unknown.
 */
EXPORT_TEXTPARSER const char *textparser_get_token_type_str(const textparser_language_definition *language, const textparser_token_item *token);

/**
 * Get the integer token ID of the token item.
 *
 * @param token The token item node.
 * @return Token ID integer.
 */
EXPORT_TEXTPARSER int textparser_get_token_type(const textparser_token_item *token);

/**
 * Get the byte offset position of the token item in the input text.
 *
 * @param token The token item node.
 * @return Byte offset position.
 */
EXPORT_TEXTPARSER size_t textparser_get_token_position(const textparser_token_item *token);

/**
 * Get the byte length of the token item.
 *
 * @param token The token item node.
 * @return Byte length.
 */
EXPORT_TEXTPARSER size_t textparser_get_token_length(const textparser_token_item *token);

/**
 * Get foreground text color code assigned to the token.
 *
 * @param token The token item node.
 * @return ARGB/RGB color value or TEXTPARSER_NOCOLOR.
 */
EXPORT_TEXTPARSER uint32_t textparser_get_token_text_color(const textparser_token_item *token);

/**
 * Get background text color code assigned to the token.
 *
 * @param token The token item node.
 * @return ARGB/RGB color value or TEXTPARSER_NOCOLOR.
 */
EXPORT_TEXTPARSER uint32_t textparser_get_token_text_background(const textparser_token_item *token);

/**
 * Get text styling/formatting flags associated with the token.
 *
 * @param token The token item node.
 * @return Bitfield flags.
 */
EXPORT_TEXTPARSER uint32_t textparser_get_token_text_flags(const textparser_token_item *token);

/**
 * Get syntax error message associated with a specific token item.
 *
 * @param token The token item node.
 * @return Const pointer to error message string, or NULL if no error.
 */
EXPORT_TEXTPARSER const char *textparser_get_token_error(const textparser_token_item *token);

/**
 * Build line offset lookup table for fast line/column mapping.
 *
 * @param handle The parser handle.
 * @return 0 on success, non-zero on failure.
 */
EXPORT_TEXTPARSER int textparser_build_line_map(textparser_t handle);

/**
 * Get total number of lines in the input text document.
 *
 * @param handle The parser handle.
 * @return Line count.
 */
EXPORT_TEXTPARSER size_t textparser_get_line_count(const textparser_t handle);

/**
 * Get byte starting position of a given 0-indexed line.
 *
 * @param handle The parser handle.
 * @param line_index 0-based index of the line.
 * @return Byte offset position.
 */
EXPORT_TEXTPARSER size_t textparser_get_line_start_position(const textparser_t handle, size_t line_index);

/**
 * Get 0-indexed line number containing the given byte position.
 *
 * @param handle The parser handle.
 * @param position Byte position offset.
 * @return 0-based line index.
 */
EXPORT_TEXTPARSER size_t textparser_get_line_number_at_position(const textparser_t handle, size_t position);

/**
 * Create an empty parser state snapshot structure.
 *
 * @param handle The parser handle.
 * @return Allocated parser state object.
 */
EXPORT_TEXTPARSER textparser_parser_state *textparser_state_new(const textparser_t handle);

/**
 * Generate a parser state snapshot at the specified byte position.
 *
 * @param handle The parser handle.
 * @param position Byte offset position.
 * @return Allocated parser state snapshot.
 */
EXPORT_TEXTPARSER textparser_parser_state *textparser_state_generate(const textparser_t handle, size_t position);

/**
 * Free a parser state snapshot.
 *
 * @param state State snapshot object to free.
 */
EXPORT_TEXTPARSER void textparser_state_free(textparser_parser_state *state);

/**
 * Cleanup helper for auto-cleanup parser state pointers (`textparser_parser_state_defer`).
 *
 * @param state Pointer to parser state pointer.
 */
EXPORT_TEXTPARSER void textparser_state_cleanup(textparser_parser_state **state);

/**
 * Query AST tokens matching a selector string starting from root node.
 *
 * @param handle The parser handle.
 * @param root Root token node to query from.
 * @param selector Query selector string.
 * @param out_count Pointer to store result array length.
 * @return Array of matching token node pointers (must be freed with `textparser_free_query_result`).
 */
EXPORT_TEXTPARSER const textparser_token_item **textparser_query(const textparser_t handle, const textparser_token_item *root, const char *selector, size_t *out_count);

/**
 * Free results array returned by `textparser_query`.
 *
 * @param results Matching token array to free.
 */
EXPORT_TEXTPARSER void textparser_free_query_result(const textparser_token_item **results);


