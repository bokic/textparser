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

/**
 * Get the version of the textparser library.
 *
 * @return NUL-terminated version string owned by the library.
 */
EXPORT_TEXTPARSER const char *textparser_version(void);

/**
 * Get the major version number of the textparser library.
 */
EXPORT_TEXTPARSER int textparser_version_major(void);

/**
 * Get the minor version number of the textparser library.
 */
EXPORT_TEXTPARSER int textparser_version_minor(void);

/**
 * Get the patch version number of the textparser library.
 */
EXPORT_TEXTPARSER int textparser_version_patch(void);

/**
 * Get the packed integer version number (major * 10000 + minor * 100 + patch).
 */
EXPORT_TEXTPARSER int textparser_version_int(void);

#define TEXTPARSER_TOKEN_ID_ERROR (-1)
#define TEXTPARSER_TOKEN_ID_UNPROCESSED (-2)
#define TEXTPARSER_TOKEN_ID_WHITESPACE (-3)
#define TEXTPARSER_TOKEN_ID_START_DELIMITER (-4)
#define TEXTPARSER_TOKEN_ID_END_DELIMITER (-5)
#define TEXTPARSER_NOCOLOR 0xffffffff

/* Node flags */
#define TEXTPARSER_NODE_NONE        (0)
#define TEXTPARSER_NODE_SYNTHETIC   (1 << 0)
#define TEXTPARSER_NODE_MISSING     (1 << 1)
#define TEXTPARSER_NODE_RECOVERED   (1 << 2)
#define TEXTPARSER_NODE_TRIVIA      (1 << 3)

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

/* Semantic event lifecycle */
typedef enum {
    TEXTPARSER_EVENT_VALIDATE,
    TEXTPARSER_EVENT_COMMIT,
    TEXTPARSER_EVENT_RECOVERY,
    TEXTPARSER_EVENT_SOURCE_COMPLETE,
} textparser_event_type;

typedef enum {
    TEXTPARSER_ACTION_ACCEPT,
    TEXTPARSER_ACTION_REJECT,
    TEXTPARSER_ACTION_ABORT,
} textparser_action;

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
typedef struct textparser_token_item textparser_node;

typedef struct {
    size_t len;
    const struct textparser_token_item *state[];
} textparser_parser_state;

typedef struct {
    size_t dirty_start;
    size_t dirty_end;
} textparser_dirty_range;

/** Read-only view of the shared transactional parser state. */
typedef struct {
    size_t source_offset;
    size_t token_index;
    size_t mode_depth;
    size_t context_depth;
    size_t diagnostic_count;
    size_t pending_event_count;
    unsigned speculation_depth;
    unsigned recovery_depth;
} textparser_parser_state_view;

typedef struct {
    size_t start_pos;
    size_t length;
    uint32_t start_line;
    uint32_t start_col;
    uint32_t end_line;
    uint32_t end_col;
    int token_id;
    uint32_t text_color;
    uint32_t text_background;
    uint32_t text_flags;
} textparser_token_range;

/* Immutable lexer stream flags. */
#define TEXTPARSER_LEX_FLAG_CONTAINS_LINE_TERMINATOR (1u << 0)

/**
 * A syntax token in the immutable lexer snapshot produced by a successful
 * parse. Offsets are expressed in the parser's encoding units and use a
 * half-open [start, end) range. Leading trivia indexes the companion trivia
 * stream. Its flags aggregate properties of that leading trivia. The
 * structure and decoded_value remain owned by the parser handle.
 */
typedef struct {
    int kind;
    size_t start;
    size_t end;
    size_t leading_trivia_start;
    size_t leading_trivia_count;
    int mode;
    int lexical_goal;
    uint32_t flags;
    const char *decoded_value;
} textparser_lex_token;

/**
 * A trivia item in the immutable lexer snapshot. Trivia currently comprises
 * whitespace leaves and nodes explicitly marked TEXTPARSER_NODE_TRIVIA.
 */
typedef struct {
    int kind;
    size_t start;
    size_t end;
    uint32_t flags;
} textparser_lex_trivia;

typedef enum {
    TEXTPARSER_PROD_TOKEN,
    TEXTPARSER_PROD_REF,
    TEXTPARSER_PROD_SEQUENCE,
    TEXTPARSER_PROD_CHOICE,
    TEXTPARSER_PROD_OPTIONAL,
    TEXTPARSER_PROD_REPEAT,
    TEXTPARSER_PROD_LOOKAHEAD,
    TEXTPARSER_PROD_NOT,
    TEXTPARSER_PROD_PREDICATE,
    TEXTPARSER_PROD_CONTEXT,
    TEXTPARSER_PROD_COMMIT,
    TEXTPARSER_PROD_PRATT,
} textparser_production_kind;

/**
 * A manually constructed grammar production. Child and reference values are
 * production IDs, not array indexes. REPEAT is zero-or-more. JSON loading for
 * these records is implemented by the next parser milestone.
 */
typedef struct {
    int id;
    const char *name;
    textparser_production_kind kind;
    const int *children;
    size_t child_count;
    int token_id;
    int referenced_production;
    const char *predicate_name;
    const char *context_name;
    int64_t context_value;
    int minimum_precedence;
    /* Recovery policy. A negative recovery_insert_token disables insertion. */
    int recovery_insert_token;
    bool recovery_insert_enabled;
    const int *recovery_sync_tokens;
    size_t recovery_sync_token_count;
    bool recovery_skip;
    bool allow_automatic_semicolon;
    const char *expected_description;
    const char *validate_handler;
    const char *validate_configuration;
    const char *commit_handler;
    const char *commit_configuration;
    const char *recovery_handler;
    const char *recovery_configuration;
} textparser_production;

typedef enum {
    TEXTPARSER_MATCH_OK,
    TEXTPARSER_MATCH_NO,
    TEXTPARSER_MATCH_ERROR,
    TEXTPARSER_MATCH_ABORT,
} textparser_match_status;

typedef struct {
    textparser_match_status status;
    textparser_node *node;
    size_t consumed_tokens;
    bool committed;
} textparser_match_result;

typedef struct {
    int start_production;
    size_t production_count;
    textparser_production *productions;
    const char *source_complete_handler;
    const char *source_complete_configuration;
} textparser_grammar_definition;

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

    /* Enhanced CST / AST properties */
    uint64_t id;
    uint32_t node_flags;
    const char *decoded_value;
    void *user_data;
    void (*free_user_data)(void *);
} textparser_token_item;

typedef struct {
    textparser_event_type type;
    textparser_node *node;
    textparser_node *parent;
    size_t start;
    size_t end;
    bool synthetic;
    bool recovered;
    const void *recovery_info;
    /* Compact JSON string for object-valued declarative event bindings. */
    const void *configuration;
    void *language_context;
} textparser_event;

typedef textparser_action (*textparser_semantic_handler)(
    textparser_t parser,
    const textparser_event *event,
    void *user_data
);

 typedef struct {
    int *when_parent_in;
    int *nested_tokens;
} textparser_context_nested_tokens;

typedef bool (*textparser_fast_regex_fn)(
    enum textparser_encoding encoding,
    const char *start,
    size_t max_len,
    size_t *offset,
    size_t *length,
    bool is_caseless,
    bool only_at_start
);

typedef struct {
    const char *name;
    enum textparser_token_type type;
    const char *start_regex;
    const char *end_regex;
    textparser_fast_regex_fn startRegexFunction;
    textparser_fast_regex_fn endRegexFunction;
    bool other_text_inside;
    bool delete_if_only_one_child;
    bool must_have_one_child;
    bool multi_line;
    bool search_parent_end_token_last;
    uint32_t text_color;
    uint32_t text_background;
    uint32_t text_flags;
    uint32_t delimiter_text_color;
    uint32_t delimiter_text_background;
    uint32_t delimiter_text_flags;
    int *nested_tokens;
    textparser_context_nested_tokens *context_nested_tokens;
} textparser_token;

typedef struct {
    int priority;
    bool is_trivia;
    const char *push_mode;
    bool pop_mode;
} textparser_contextual_lexer_rule;

typedef struct {
    const char *name;
    int *tokens;
    int *trivia;
} textparser_lexer_mode;

typedef struct {
    int source_token;
    int target_token;
} textparser_lexer_goal_mapping;

typedef struct {
    const char *name;
    size_t mapping_count;
    textparser_lexer_goal_mapping *mappings;
} textparser_lexer_goal;

struct textparser_operator_def;

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
    const int *regex_tokens;
    const int *division_tokens;
    const int *operand_tokens;
    const char **control_keywords;
} textparser_regex_disambiguation;

typedef struct {
    const int *template_open_tokens;
    const int *template_close_tokens;
    const int *valid_inner_tokens;
    const char **invalid_inner_operators;
    int template_group_token_id;
} textparser_template_disambiguation;

typedef struct {
    const int *type_tokens;
    const char **type_keywords;
    const char **type_suffixes;
    int cast_token_id;
} textparser_cast_disambiguation;

typedef struct {
    const int *return_type_tokens;
    const int *declarator_tokens;
    int identifier_token_id;
    int type_name_token_id;
    int function_token_id;
    int parameter_list_token_id;
} textparser_declaration_disambiguation;

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
    textparser_regex_disambiguation *regex_disambiguation;
    textparser_template_disambiguation *template_disambiguation;
    textparser_cast_disambiguation *cast_disambiguation;
    textparser_declaration_disambiguation *declaration_disambiguation;
    textparser_grammar_definition *grammar;
    const char *initial_lexer_mode;
    size_t lexer_mode_count;
    textparser_lexer_mode *lexer_modes;
    size_t lexer_goal_count;
    textparser_lexer_goal *lexer_goals;
    textparser_contextual_lexer_rule *lexer_rules;
    size_t operator_definition_count;
    struct textparser_operator_def *operator_definitions;
    size_t maximum_diagnostics;
    size_t maximum_skipped_tokens;
    size_t maximum_recovery_attempts;
    size_t recovery_sync_token_count;
    int *recovery_sync_tokens;
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

/**
 * Export flat token ranges for an entire parsed document into a caller-provided buffer.
 *
 * @param handle The parser handle.
 * @param buffer Array of textparser_token_range structures to populate.
 * @param max_tokens Capacity of the provided buffer in token entries.
 * @param out_count Pointer to store total number of tokens matching traversal.
 * @return 0 on success, non-zero if invalid arguments or buffer is insufficient.
 */
EXPORT_TEXTPARSER int textparser_export_tokens(const textparser_t handle, textparser_token_range *buffer, size_t max_tokens, size_t *out_count);

/**
 * Export flat token ranges intersecting a specific byte range [start_pos, end_pos).
 *
 * @param handle The parser handle.
 * @param start_pos Starting unit offset.
 * @param end_pos Ending unit offset.
 * @param buffer Array of textparser_token_range structures to populate.
 * @param max_tokens Capacity of the provided buffer.
 * @param out_count Pointer to store number of matching tokens found.
 * @return 0 on success, non-zero on failure.
 */
EXPORT_TEXTPARSER int textparser_export_tokens_range(const textparser_t handle, size_t start_pos, size_t end_pos, textparser_token_range *buffer, size_t max_tokens, size_t *out_count);

/**
 * Export flat token ranges intersecting a specific line range [start_line, end_line] (0-indexed).
 *
 * @param handle The parser handle.
 * @param start_line Starting line index (0-indexed).
 * @param end_line Ending line index (0-indexed, inclusive).
 * @param buffer Array of textparser_token_range structures to populate.
 * @param max_tokens Capacity of the provided buffer.
 * @param out_count Pointer to store number of matching tokens found.
 * @return 0 on success, non-zero on failure.
 */
EXPORT_TEXTPARSER int textparser_export_tokens_lines(const textparser_t handle, size_t start_line, size_t end_line, textparser_token_range *buffer, size_t max_tokens, size_t *out_count);

/**
 * Get the immutable syntax-token stream for the latest successful parse.
 * The returned array must not be modified or freed and is invalidated by the
 * next parse, text update, or parser close.
 *
 * @param handle The parser handle.
 * @param out_count Receives the number of entries in the returned array.
 * @return Read-only token array, or NULL when empty or arguments are invalid.
 */
EXPORT_TEXTPARSER const textparser_lex_token *textparser_get_lexer_tokens(
    const textparser_t handle,
    size_t *out_count
);

/**
 * Get the immutable trivia stream for the latest successful parse.
 * The returned array has the same lifetime as textparser_get_lexer_tokens().
 *
 * @param handle The parser handle.
 * @param out_count Receives the number of entries in the returned array.
 * @return Read-only trivia array, or NULL when empty or arguments are invalid.
 */
EXPORT_TEXTPARSER const textparser_lex_trivia *textparser_get_lexer_trivia(
    const textparser_t handle,
    size_t *out_count
);

/**
 * Register a named semantic handler with the parser.
 *
 * @param handle The parser handle.
 * @param name Unique identifier name for the semantic action.
 * @param handler Function pointer to semantic handler callback.
 * @param user_data Custom user context passed to the handler.
 * @return 0 on success, non-zero on failure.
 */
EXPORT_TEXTPARSER int textparser_register_handler(
    textparser_t handle,
    const char *name,
    textparser_semantic_handler handler,
    void *user_data
);

/**
 * Dispatch a semantic lifecycle event.
 *
 * @param handle The parser handle.
 * @param handler_name Name of registered handler (or NULL to invoke default handler).
 * @param event Event payload containing type, node, parent, spans, and recovery info.
 * @return TEXTPARSER_ACTION_ACCEPT, TEXTPARSER_ACTION_REJECT, or TEXTPARSER_ACTION_ABORT.
 */
EXPORT_TEXTPARSER textparser_action textparser_dispatch_event(
    textparser_t handle,
    const char *handler_name,
    const textparser_event *event
);

/**
 * Get the stable 64-bit unique ID of a syntax node.
 *
 * @param node The syntax node.
 * @return 64-bit node ID.
 */
EXPORT_TEXTPARSER uint64_t textparser_node_get_id(const textparser_node *node);

/**
 * Get the node flags (e.g. TEXTPARSER_NODE_SYNTHETIC, TEXTPARSER_NODE_MISSING, TEXTPARSER_NODE_RECOVERED).
 *
 * @param node The syntax node.
 * @return Bitfield flags.
 */
EXPORT_TEXTPARSER uint32_t textparser_node_get_flags(const textparser_node *node);

/**
 * Set node flags on a syntax node.
 *
 * @param node The syntax node.
 * @param flags Flags to set.
 */
EXPORT_TEXTPARSER void textparser_node_set_flags(textparser_node *node, uint32_t flags);

/**
 * Get the application-owned user data attachment from a node.
 *
 * @param node The syntax node.
 * @return Pointer to user_data.
 */
EXPORT_TEXTPARSER void *textparser_node_get_user_data(const textparser_node *node);

/**
 * Set the application-owned user data attachment on a node.
 *
 * @param node The syntax node.
 * @param user_data Pointer to user data.
 * @param free_fn Optional cleanup callback when node is destroyed.
 */
EXPORT_TEXTPARSER void textparser_node_set_user_data(
    textparser_node *node,
    void *user_data,
    void (*free_fn)(void *)
);

/**
 * Get decoded value string associated with a node (if decoded by valueDecoder).
 *
 * @param node The syntax node.
 * @return Pointer to decoded string value or NULL.
 */
EXPORT_TEXTPARSER const char *textparser_node_get_decoded_value(const textparser_node *node);

/**
 * Set decoded value string on a node.
 *
 * @param node The syntax node.
 * @param value Decoded string value.
 */
EXPORT_TEXTPARSER void textparser_node_set_decoded_value(textparser_node *node, const char *value);

/* -------------------------------------------------------------------------
 * Phase 3: Lexer Modes, Goals, Decoders & Validators
 * ------------------------------------------------------------------------- */

typedef char *(*textparser_decoder_fn)(
    textparser_t parser,
    const char *raw_text,
    size_t length,
    void *user_data
);

typedef bool (*textparser_validator_fn)(
    textparser_t parser,
    const char *raw_text,
    size_t length,
    const char **out_error,
    void *user_data
);

/**
 * Register a custom token value decoder (e.g. "ecmascript.identifier", "ecmascript.stringLiteral").
 *
 * @param handle The parser handle.
 * @param name Unique name of decoder.
 * @param decoder Function pointer returning allocated decoded string (or NULL).
 * @param user_data User data passed to decoder.
 * @return 0 on success, non-zero on failure.
 */
EXPORT_TEXTPARSER int textparser_register_decoder(
    textparser_t handle,
    const char *name,
    textparser_decoder_fn decoder,
    void *user_data
);

/**
 * Register a custom token validator (e.g. "ecmascript.numericLiteral").
 *
 * @param handle The parser handle.
 * @param name Unique name of validator.
 * @param validator Function pointer returning true if valid, false otherwise.
 * @param user_data User data passed to validator.
 * @return 0 on success, non-zero on failure.
 */
EXPORT_TEXTPARSER int textparser_register_validator(
    textparser_t handle,
    const char *name,
    textparser_validator_fn validator,
    void *user_data
);

/**
 * Decode a token text using a registered decoder name.
 *
 * @param handle The parser handle.
 * @param decoder_name Registered decoder name.
 * @param raw_text Raw token slice.
 * @param length Length of raw slice.
 * @return Decoded string or NULL.
 */
EXPORT_TEXTPARSER char *textparser_decode_token(
    textparser_t handle,
    const char *decoder_name,
    const char *raw_text,
    size_t length
);

/**
 * Validate a token text using a registered validator name.
 *
 * @param handle The parser handle.
 * @param validator_name Registered validator name.
 * @param raw_text Raw token slice.
 * @param length Length of raw slice.
 * @param out_error Output error message pointer.
 * @return True if valid, false if invalid.
 */
EXPORT_TEXTPARSER bool textparser_validate_token(
    textparser_t handle,
    const char *validator_name,
    const char *raw_text,
    size_t length,
    const char **out_error
);

/**
 * Push a transient lexical mode onto the mode stack.
 *
 * @param handle The parser handle.
 * @param mode_name Mode name to activate.
 * @return 0 on success, non-zero on failure.
 */
EXPORT_TEXTPARSER int textparser_push_mode(textparser_t handle, const char *mode_name);

/**
 * Pop the topmost transient lexical mode from the mode stack.
 *
 * @param handle The parser handle.
 * @return 0 on success, non-zero if stack was empty.
 */
EXPORT_TEXTPARSER int textparser_pop_mode(textparser_t handle);

/**
 * Get current active lexical mode name.
 *
 * @param handle The parser handle.
 * @return Current mode name (defaults to "default" if stack is empty).
 */
EXPORT_TEXTPARSER const char *textparser_get_current_mode(textparser_t handle);

/**
 * Set the contextual lexical goal for the next scanned token.
 *
 * @param handle The parser handle.
 * @param goal_name Goal name (e.g. "ExpressionStart", "ExpressionContinuation", or NULL to clear).
 */
EXPORT_TEXTPARSER void textparser_set_lexical_goal(textparser_t handle, const char *goal_name);

/**
 * Get the current contextual lexical goal.
 *
 * @param handle The parser handle.
 * @return Active goal name or NULL.
 */
EXPORT_TEXTPARSER const char *textparser_get_lexical_goal(textparser_t handle);

/** Scan without consuming using the current mode and the supplied goal. */
EXPORT_TEXTPARSER int textparser_lexer_peek(
    textparser_t handle,
    size_t lookahead,
    const char *goal_name,
    const textparser_lex_token **out_token
);

/** Scan and consume one token using the current mode and supplied goal. */
EXPORT_TEXTPARSER int textparser_lexer_consume(
    textparser_t handle,
    const char *goal_name,
    const textparser_lex_token **out_token
);

/**
 * Check if trivia (whitespace, line terminators, comments) between two offsets contains a line terminator.
 *
 * @param handle The parser handle.
 * @param start_pos Starting byte offset.
 * @param end_pos Ending byte offset.
 * @return True if a line terminator (\r, \n, \u2028, \u2029) is present within the span.
 */
EXPORT_TEXTPARSER bool textparser_has_line_terminator_between(textparser_t handle, size_t start_pos, size_t end_pos);

/* -------------------------------------------------------------------------
 * Phase 4: Declarative Grammar Engine & Speculative Parsing
 * ------------------------------------------------------------------------- */

typedef bool (*textparser_predicate_fn)(
    textparser_t parser,
    const char *predicate_name,
    void *user_data
);

typedef struct {
    int production_id;
    const textparser_lex_token *current;
    const textparser_lex_token *previous;
    bool has_preceding_line_terminator;
} textparser_predicate_context;

typedef bool (*textparser_parser_predicate_fn)(
    textparser_t parser,
    const textparser_predicate_context *context,
    void *user_data
);

/**
 * Register a native semantic predicate (e.g. "typescript.isStartOfTypeArguments").
 *
 * @param handle The parser handle.
 * @param name Unique name of predicate.
 * @param predicate Predicate function returning true if satisfied, false otherwise.
 * @param user_data Context pointer passed to predicate.
 * @return 0 on success, non-zero on failure.
 */
EXPORT_TEXTPARSER int textparser_register_predicate(
    textparser_t handle,
    const char *name,
    textparser_predicate_fn predicate,
    void *user_data
);

/**
 * Register a parser-aware predicate used by declarative grammar productions.
 * The callback may inspect tokens and contexts but must not permanently mutate
 * parser state.
 */
EXPORT_TEXTPARSER int textparser_register_parser_predicate(
    textparser_t handle,
    const char *name,
    textparser_parser_predicate_fn predicate,
    void *user_data
);

/**
 * Evaluate a registered predicate.
 *
 * @param handle The parser handle.
 * @param name Name of registered predicate.
 * @return True if predicate passes, false otherwise.
 */
EXPORT_TEXTPARSER bool textparser_eval_predicate(
    textparser_t handle,
    const char *name
);

/**
 * Set a scoped context boolean or integer value in the parser.
 *
 * @param handle The parser handle.
 * @param context_name Name of context (e.g. "AllowAwait", "AllowYield", "InType").
 * @param value Integer/boolean value.
 * @return 0 on success, non-zero on failure.
 */
EXPORT_TEXTPARSER int textparser_context_set(
    textparser_t handle,
    const char *context_name,
    int64_t value
);

/**
 * Get a scoped context value.
 *
 * @param handle The parser handle.
 * @param context_name Name of context.
 * @param out_value Pointer to receive value.
 * @return 0 if found, non-zero if not set.
 */
EXPORT_TEXTPARSER int textparser_context_get(
    textparser_t handle,
    const char *context_name,
    int64_t *out_value
);

/**
 * Check if a scoped context boolean is set to true.
 *
 * @param handle The parser handle.
 * @param context_name Name of context.
 * @return True if set and non-zero, false otherwise.
 */
EXPORT_TEXTPARSER bool textparser_context_is(
    textparser_t handle,
    const char *context_name
);

/**
 * Read the common transactional parser state used by grammar operations.
 *
 * @param handle The parser handle.
 * @param out_state Receives a value snapshot of parser cursors and depths.
 * @return 0 on success, non-zero for invalid arguments.
 */
EXPORT_TEXTPARSER int textparser_get_parser_state(
    textparser_t handle,
    textparser_parser_state_view *out_state
);

/**
 * Begin a speculative parse branch with prioritized commit points.
 *
 * @param handle The parser handle.
 * @param out_checkpoint Pointer to store saved checkpoint.
 */
EXPORT_TEXTPARSER void textparser_speculate_begin(
    textparser_t handle,
    void **out_checkpoint
);

/**
 * Commit a speculative parse branch (discards rollback data without rolling back).
 *
 * @param handle The parser handle.
 * @param checkpoint Checkpoint pointer from textparser_speculate_begin.
 */
EXPORT_TEXTPARSER void textparser_speculate_commit(
    textparser_t handle,
    void *checkpoint
);

/**
 * Roll back a failed speculative parse branch. This restores parser cursors,
 * all modes and contexts, the lexical goal, diagnostics, pending events,
 * nesting depths, node IDs, and arena allocations.
 *
 * @param handle The parser handle.
 * @param checkpoint Checkpoint pointer from textparser_speculate_begin.
 */
EXPORT_TEXTPARSER void textparser_speculate_rollback(
    textparser_t handle,
    void *checkpoint
);

/**
 * Execute one manually constructed grammar production from the beginning of
 * the immutable lexer-token stream. Successful execution leaves the shared
 * parser cursor after the matched tokens; failed matches restore its start.
 *
 * @param handle A successfully parsed handle with an immutable token stream.
 * @param productions Production table with unique IDs.
 * @param production_count Number of table entries.
 * @param start_production ID of the production to execute.
 * @param out_result Receives match status, syntax node, and consumed count.
 * @return 0 when execution ran, non-zero for invalid arguments.
 */
EXPORT_TEXTPARSER int textparser_execute_production(
    textparser_t handle,
    const textparser_production *productions,
    size_t production_count,
    int start_production,
    textparser_match_result *out_result
);

/** Execute the grammar stored in a loaded language definition. */
EXPORT_TEXTPARSER int textparser_execute_language_grammar(
    textparser_t handle,
    const textparser_language_definition *language,
    textparser_match_result *out_result
);

/* -------------------------------------------------------------------------
 * Phase 5: Operator Precedence & Pratt / Precedence Engine
 * ------------------------------------------------------------------------- */

typedef enum textparser_operator_role {
    TEXTPARSER_OP_INFIX = 0,
    TEXTPARSER_OP_PREFIX = 1,
    TEXTPARSER_OP_POSTFIX = 2,
    TEXTPARSER_OP_TERNARY = 3,
} textparser_operator_role;

typedef struct textparser_operator_def {
    int token_id;
    textparser_operator_role role;
    int precedence;
    enum textparser_associativity associativity;
    int secondary_token_id; // For ternary (e.g. ':' following '?')
} textparser_operator_def;

/**
 * Register an operator definition with explicit role, precedence, and associativity.
 *
 * @param handle The parser handle.
 * @param op Operator definition structure.
 * @return 0 on success, non-zero on failure.
 */
EXPORT_TEXTPARSER int textparser_register_operator(
    textparser_t handle,
    const textparser_operator_def *op
);

/**
 * Look up operator information for a token ID and expected role.
 *
 * @param handle The parser handle.
 * @param token_id Token ID.
 * @param role Expected role (or -1 for any role).
 * @param out_op Output operator definition.
 * @return 0 if found, non-zero if not defined.
 */
EXPORT_TEXTPARSER int textparser_get_operator(
    textparser_t handle,
    int token_id,
    int role,
    textparser_operator_def *out_op
);

/**
 * Parse an expression using Pratt / Precedence Climbing with the registered operator table.
 *
 * @param handle The parser handle.
 * @param min_precedence Minimum binding power / precedence level.
 * @param out_node Pointer to root AST node created for the parsed expression.
 * @return 0 on success, non-zero on error.
 */
EXPORT_TEXTPARSER int textparser_parse_pratt_expression(
    textparser_t handle,
    int min_precedence,
    textparser_node **out_node
);

/* -------------------------------------------------------------------------
 * Phase 6: Error Recovery & Diagnostic Engine
 * ------------------------------------------------------------------------- */

typedef enum textparser_diagnostic_severity {
    TEXTPARSER_SEVERITY_ERROR = 0,
    TEXTPARSER_SEVERITY_WARNING = 1,
    TEXTPARSER_SEVERITY_INFO = 2,
    TEXTPARSER_SEVERITY_HINT = 3,
} textparser_diagnostic_severity;

typedef struct textparser_diagnostic {
    textparser_diagnostic_severity severity;
    const char *code;
    const char *message;
    size_t start_pos;
    size_t length;
    uint32_t line;
    uint32_t column;
} textparser_diagnostic;

/**
 * Report a new diagnostic (error, warning, info) with exact source location.
 *
 * @param handle The parser handle.
 * @param severity Severity level.
 * @param code Diagnostic error code (e.g. "TS1005", "SYNTAX_ERROR").
 * @param message Diagnostic descriptive message.
 * @param start_pos Starting byte offset.
 * @param length Length of error span in bytes.
 * @return 0 on success, non-zero on failure.
 */
EXPORT_TEXTPARSER int textparser_report_diagnostic(
    textparser_t handle,
    textparser_diagnostic_severity severity,
    const char *code,
    const char *message,
    size_t start_pos,
    size_t length
);

/**
 * Get the total number of diagnostics recorded by the parser.
 *
 * @param handle The parser handle.
 * @return Diagnostic count.
 */
EXPORT_TEXTPARSER size_t textparser_get_diagnostic_count(textparser_t handle);

/**
 * Get a specific diagnostic by index.
 *
 * @param handle The parser handle.
 * @param index 0-based index of diagnostic.
 * @param out_diagnostic Pointer to store diagnostic copy.
 * @return 0 on success, non-zero if index out of bounds.
 */
EXPORT_TEXTPARSER int textparser_get_diagnostic(
    textparser_t handle,
    size_t index,
    textparser_diagnostic *out_diagnostic
);

/**
 * Clear all diagnostics in the parser handle.
 *
 * @param handle The parser handle.
 */
EXPORT_TEXTPARSER void textparser_clear_diagnostics(textparser_t handle);

/**
 * Recover parser to the next synchronization token from a given token list.
 *
 * @param handle The parser handle.
 * @param sync_tokens Array of token IDs to synchronize on (terminated by TextParser_END or -1).
 * @param current_offset Current parsing offset in units/bytes.
 * @param out_new_offset Pointer to receive updated offset after synchronization.
 * @return 0 if synchronized to a matching token, non-zero if EOF reached.
 */
EXPORT_TEXTPARSER int textparser_recover_until_token(
    textparser_t handle,
    const int *sync_tokens,
    size_t current_offset,
    size_t *out_new_offset
);
