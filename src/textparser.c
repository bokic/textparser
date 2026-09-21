#include <textparser.h>
#include "adv_regex.h"
#include "logger.h"
#include "string_pool.h"
#include <os.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <ctype.h>
#include <time.h>

#ifndef SSIZE_MAX
#define SSIZE_MAX ((ssize_t)((((size_t)-1) << 1) >> 1))
#endif

#define MAX_PARSE_SIZE (16 * 1024 * 1024)

#define TOKEN_NOT_FOUND -1

/*
 * Internal node_flags marker recording that a node's subtree has been through
 * textparser_post_process. Cast/declaration disambiguation mutate token IDs in
 * place without synthesizing a tagged node, so this marker is the only reliable
 * way for the incremental engine to know that a tree is in AST mode and must be
 * re-derived after an edit. Bit 5 is unused by the public TEXTPARSER_NODE_* set.
 */
#define TEXTPARSER_NODE_POST_PROCESSED (1u << 5)

#define exit_with_error(handle, error_text, offset, error_len)   \
    LOGE("Error: %s at %zu", error_text, offset);                \
    if(handle) (handle)->error = error_text;                     \
    if(handle) (handle)->error_offset = offset;                  \
    if(handle) (handle)->error_length = error_len;               \
    goto exit;                                        \

#define check_and_exit_on_fatal_parsing_error(handle, child, offset)                          \
    if ((handle)->error) {                                                                    \
        LOGW("Fatal error detected(%s) at offset %zu. exiting..", (handle)->error, offset);   \
        goto exit;                                                                            \
    }                                                                                         \
    if ((child)->len == 0) {                                                                  \
        LOGW("child->len == 0 detected(%s) at offset %zu. exiting..", (handle)->error ? (handle)->error : "none", offset); \
        exit_with_error(handle, "infinite loop due to 0-length token", offset, 0);               \
    }

#define check_and_exit_on_fatal_parsing_error_start_stop(handle, child, offset)                \
    if ((handle)->error) {                                                                    \
        LOGW("Fatal error detected(%s) at offset %zu. exiting..", (handle)->error, offset);   \
        goto exit;                                                                            \
    }

static size_t calculate_chunk_size(size_t text_size);
static size_t textparser_skip_whitespace(const struct textparser_handle *handle, size_t pos);

/**
 * Check whether a given token ID corresponds to trivia (unprocessed text, whitespace, or delimiters).
 *
 * @param token_id Integer ID of the token to evaluate.
 * @return true if token_id is UNPROCESSED, WHITESPACE, START_DELIMITER, or END_DELIMITER; false otherwise.
 */
static inline bool is_trivia_token_id(int token_id)
{
    return token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED ||
           token_id == TEXTPARSER_TOKEN_ID_WHITESPACE ||
           token_id == TEXTPARSER_TOKEN_ID_START_DELIMITER ||
           token_id == TEXTPARSER_TOKEN_ID_END_DELIMITER;
}

enum parent_start_stop{
    TEXTPARSER_SEARCH_END_TOKEN,
    TEXTPARSER_SEARCH_START_TOKEN,
};

typedef struct textparser_handler_entry {
    char *name;
    textparser_semantic_handler handler;
    void *user_data;
    struct textparser_handler_entry *next;
} textparser_handler_entry;

typedef struct textparser_capture_entry {
    const char *name;
    size_t start;
    size_t end;
    struct textparser_capture_entry *next;
} textparser_capture_entry;

typedef struct {
    const char *handler_name;
    textparser_event event;
} textparser_pending_event;

typedef struct textparser_decoder_entry {
    char *name;
    textparser_decoder_fn decoder;
    void *user_data;
    struct textparser_decoder_entry *next;
} textparser_decoder_entry;

typedef struct textparser_validator_entry {
    char *name;
    textparser_validator_fn validator;
    void *user_data;
    struct textparser_validator_entry *next;
} textparser_validator_entry;

typedef struct textparser_operand_validator_entry {
    char *name;
    textparser_operand_validator_fn validator;
    void *user_data;
    struct textparser_operand_validator_entry *next;
} textparser_operand_validator_entry;

typedef struct textparser_predicate_entry {
    char *name;
    textparser_predicate_fn predicate;
    textparser_parser_predicate_fn parser_predicate;
    void *user_data;
    struct textparser_predicate_entry *next;
} textparser_predicate_entry;

typedef struct textparser_context_entry {
    char *name;
    int64_t value;
    struct textparser_context_entry *next;
} textparser_context_entry;

#define TEXTPARSER_MAX_MODE_STACK 64

typedef struct {
    textparser_t owner;
    const textparser_language_definition *language;
    size_t source_offset;
    size_t token_index;
    size_t pending_event_count;
    unsigned speculation_depth;
    unsigned recovery_depth;
    bool has_previous_token;
    textparser_lex_token previous_token;
} textparser_parser_runtime;

typedef struct {
    int kind;
    size_t start;
    size_t end;
} textparser_cached_trivia;

typedef struct textparser_lexer_cache_entry {
    size_t source_offset;
    int source_rule;
    char *mode;
    char *goal;
    textparser_lex_token token;
    struct textparser_lexer_cache_entry *next;
} textparser_lexer_cache_entry;

/* A dynamic lexer capture (e.g. a here-doc delimiter). Stored as raw subject
 * bytes so the dynamic match can compare without re-encoding. */
typedef struct {
    char *bytes;
    size_t byte_length;
    size_t unit_length;
    bool strip_tabs;
} textparser_lexer_capture;

/* FIFO of captures for one slot (bash reads queued here-docs in order). */
typedef struct {
    textparser_lexer_capture *items;
    size_t count;
    size_t capacity;
} textparser_lexer_capture_queue;

/* Reusable chunked bump allocator. `scratch` is reset at the start of every
 * incremental parse and only holds per-call temporaries. */
typedef struct {
    void **chunks;
    size_t chunk_count;
    size_t chunk_capacity;
    size_t chunk_size;
    void *current_chunk;
    size_t current_chunk_index;
    size_t current_chunk_used;
} textparser_arena;

struct textparser_handle {
    const textparser_language_definition *language;
    adv_regex_context *regex_ctx;
    void *start_regex;
    void *end_regex;
    void *mmap_addr;
    size_t mmap_size;
    void *owned_buffer;
    size_t owned_buffer_capacity;
    enum textparser_bom bom;
    enum textparser_encoding text_format;
    textparser_token_item *first_item;
    size_t error_offset;
    size_t error_length;
    const char *error;
    size_t token_count;
    const char *text_addr;
    size_t text_size;
    size_t no_lines;
    size_t *lines;
    /* Cached end-of-line offset for the single-line search bound. Valid for any
     * position in [line_cache_anchor, line_cache_end); reset per parse. */
    size_t line_cache_anchor;
    size_t line_cache_end;
    // Arena allocator fields
    textparser_arena arena;   // live token tree
    textparser_arena scratch; // per-call temporaries
    void (*callback)(textparser_t, textparser_token_item *, enum textparser_callback_type callback_type, void *user_data);
    void *user_data;
    int recursion_depth;
    size_t parse_steps;
    size_t parse_step_limit;
    bool parse_budget_exceeded;
    char *filename;

    /* Semantic action handlers & node ID generation */
    uint64_t next_node_id;
    textparser_handler_entry *handlers;
    textparser_pending_event *pending_events;
    size_t pending_event_capacity;

    /* Phase 3: Decoders & Validators */
    textparser_decoder_entry *decoders;
    textparser_validator_entry *validators;

    /* Phase 3: Lexer Mode Stack & Goals */
    char *mode_stack[TEXTPARSER_MAX_MODE_STACK];
    size_t mode_stack_depth;
    char *lexical_goal;

    /* Phase 4: Predicates & Scoped Contexts */
    textparser_predicate_entry *predicates;
    textparser_context_entry *contexts;

    /* Phase 5: Operator Precedence & Pratt Engine */
    textparser_operator_def *operators;
    size_t operator_count;
    size_t operator_capacity;
    textparser_operand_validator_entry *operand_validators;

    /* Phase 6: Multi-Diagnostic Vector */
    textparser_diagnostic *diagnostics;
    size_t diagnostic_count;
    size_t diagnostic_capacity;

    /* Immutable lexer snapshot for the latest successful parse. The buffers are
     * retained across incremental edits so a rebuild reuses capacity instead of
     * freeing and reallocating on every edit. */
    textparser_lex_token *lexer_tokens;
    size_t lexer_token_count;
    size_t lexer_token_capacity;
    textparser_lex_trivia *lexer_trivia;
    size_t lexer_trivia_count;
    size_t lexer_trivia_capacity;
    textparser_lexer_cache_entry *lexer_cache;
    /* Dynamic lexer captures (here-doc delimiters). Slot 0 is unused; slots are
     * 1-based to match the JSON `capture`/`dynamic` fields. */
    textparser_lexer_capture_queue lexer_captures[TEXTPARSER_MAX_LEXER_CAPTURES];

    /* Phase 3 — position bias for the flat snapshot arrays.
     *
     * In-leaf edits change the length of one leaf without restructuring the
     * tree. Instead of walking the entire suffix of `lexer_tokens` and
     * `lexer_trivia` to shift every entry's `start`/`end` by `delta_units`,
     * the delta is recorded here and applied lazily:
     *
     *   For i >= lexer_snapshot_token_bias_start:
     *     effective start = lexer_tokens[i].start + lexer_snapshot_bias
     *     effective end   = lexer_tokens[i].end   + lexer_snapshot_bias
     *
     *   For j >= lexer_snapshot_trivia_bias_start:
     *     effective start = lexer_trivia[j].start + lexer_snapshot_bias
     *     effective end   = lexer_trivia[j].end   + lexer_snapshot_bias
     *
     * Callers that need absolute positions (binary-search find functions,
     * memo-shift reads) must use the LEXER_TOKEN_START / LEXER_TOKEN_END /
     * LEXER_TRIVIA_START / LEXER_TRIVIA_END accessor macros.  The bias is
     * materialized (applied to every suffix entry) before any full rebuild and
     * on stream clear, so the grammar executor always sees correct values.
     *
     * Multiple consecutive in-leaf edits with the same bias threshold
     * accumulate into a single delta field without extra allocation. */
    ssize_t  lexer_snapshot_bias;
    size_t   lexer_snapshot_token_bias_start;
    size_t   lexer_snapshot_trivia_bias_start;


    /* Packrat memoization table for grammar productions. */
    struct textparser_memo_entry *grammar_memo;
    uint64_t next_memo_seq;

    /* Packrat memoization of legacy parser failures, keyed by
     * (token_id, offset, parent_token_id, prev_token_id). Bounds the
     * exponential candidate retry loop on nested ambiguous input. */
    struct textparser_parse_memo **parse_memo;
    size_t parse_memo_buckets;
    size_t parse_memo_count;

    /* Shared transactional state used by all grammar operations. */
    textparser_parser_runtime parser;
};

static void textparser_memo_clear(struct textparser_handle *handle);
static void textparser_memo_shift_and_invalidate(
    struct textparser_handle *handle,
    size_t dirty_start_token,
    size_t dirty_end_token,
    ssize_t delta_tokens);

/**
 * Release cached contextual lexer lookup entries.
 *
 * The cache is keyed by absolute source offset, so any text edit or stream
 * rebuild invalidates every entry. The token/trivia snapshot buffers are
 * deliberately left intact so they can be reused by the next rebuild.
 *
 * @param handle Pointer to the textparser handle whose lexer cache will be cleared.
 */
static void textparser_clear_lexer_cache(struct textparser_handle *handle)
{
    if (handle == nullptr) return;
    textparser_lexer_cache_entry *entry = handle->lexer_cache;
    while (entry != nullptr) {
        textparser_lexer_cache_entry *next = entry->next;
        free(entry->mode);
        free(entry->goal);
        free(entry);
        entry = next;
    }
    handle->lexer_cache = nullptr;
    for (int i = 0; i < TEXTPARSER_MAX_LEXER_CAPTURES; i++) {
        textparser_lexer_capture_queue *queue = &handle->lexer_captures[i];
        for (size_t j = 0; j < queue->count; j++) free(queue->items[j].bytes);
        free(queue->items);
        queue->items = nullptr;
        queue->count = 0;
        queue->capacity = 0;
    }
}

/* ---- Phase 3: snapshot position-bias accessor macros --------------------
 *
 * These macros must be used by any code that reads a lexer_tokens[i].start/end
 * or lexer_trivia[j].start/end after a potential in-leaf patch. The bias is
 * zero (no-op) when lexer_snapshot_bias == 0 or the index is below the
 * threshold. The grammar executor and full-rebuild paths always see a
 * materialized (bias-free) snapshot, so they can read the fields directly.
 */
#define LEXER_TOKEN_START(h, i) \
    ((size_t)((ssize_t)(h)->lexer_tokens[(i)].start + \
              ((i) >= (h)->lexer_snapshot_token_bias_start ? (h)->lexer_snapshot_bias : 0)))
#define LEXER_TOKEN_END(h, i) \
    ((size_t)((ssize_t)(h)->lexer_tokens[(i)].end + \
              ((i) >= (h)->lexer_snapshot_token_bias_start ? (h)->lexer_snapshot_bias : 0)))
#define LEXER_TRIVIA_START(h, j) \
    ((size_t)((ssize_t)(h)->lexer_trivia[(j)].start + \
              ((j) >= (h)->lexer_snapshot_trivia_bias_start ? (h)->lexer_snapshot_bias : 0)))
#define LEXER_TRIVIA_END(h, j) \
    ((size_t)((ssize_t)(h)->lexer_trivia[(j)].end + \
              ((j) >= (h)->lexer_snapshot_trivia_bias_start ? (h)->lexer_snapshot_bias : 0)))

/**
 * Materialize a pending snapshot position bias into the stored arrays.
 *
 * Each in-leaf patch records a `lexer_snapshot_bias` delta instead of walking
 * the O(n) suffix. Before any full rebuild or stream release the bias must be
 * applied so the stored values are correct absolute positions again.
 *
 * The function is O(n_suffix_tokens + n_suffix_trivia) — the same cost as the
 * old per-edit loops — but is called at most once per structural edit (rebuild)
 * rather than once per in-leaf edit, so the total work over a sequence of k
 * in-leaf edits followed by one rebuild drops from O(k*n) to O(n).
 *
 * @param handle Pointer to the textparser handle.
 */
static void textparser_snapshot_materialize_bias(struct textparser_handle *handle)
{
    if (handle == nullptr || handle->lexer_snapshot_bias == 0) return;
    ssize_t bias = handle->lexer_snapshot_bias;
    for (size_t i = handle->lexer_snapshot_token_bias_start;
         i < handle->lexer_token_count; i++) {
        handle->lexer_tokens[i].start =
            (size_t)((ssize_t)handle->lexer_tokens[i].start + bias);
        handle->lexer_tokens[i].end   =
            (size_t)((ssize_t)handle->lexer_tokens[i].end   + bias);
    }
    for (size_t j = handle->lexer_snapshot_trivia_bias_start;
         j < handle->lexer_trivia_count; j++) {
        handle->lexer_trivia[j].start =
            (size_t)((ssize_t)handle->lexer_trivia[j].start + bias);
        handle->lexer_trivia[j].end   =
            (size_t)((ssize_t)handle->lexer_trivia[j].end   + bias);
    }
    handle->lexer_snapshot_bias = 0;
    handle->lexer_snapshot_token_bias_start  = 0;
    handle->lexer_snapshot_trivia_bias_start = 0;
}

/**
 * Release allocated lexer token streams, trivia streams, and cached lexer lookup entries stored in the handle.
 *
 * @param handle Pointer to the textparser handle whose lexer streams will be cleared.
 */
static void textparser_clear_lexer_streams(struct textparser_handle *handle)
{
    if (handle == nullptr) return;
    free(handle->lexer_tokens);
    free(handle->lexer_trivia);
    handle->lexer_tokens = nullptr;
    handle->lexer_token_count = 0;
    handle->lexer_token_capacity = 0;
    handle->lexer_trivia = nullptr;
    handle->lexer_trivia_count = 0;
    handle->lexer_trivia_capacity = 0;
    handle->lexer_snapshot_bias = 0;
    handle->lexer_snapshot_token_bias_start  = 0;
    handle->lexer_snapshot_trivia_bias_start = 0;
    textparser_clear_lexer_cache(handle);
}

/**
 * Convert a character code unit offset into an absolute byte offset based on document text encoding.
 *
 * @param handle Pointer to the textparser handle holding encoding configuration.
 * @param pos Character unit offset within the input text.
 * @return Absolute byte offset corresponding to pos.
 */
static size_t textparser_get_byte_offset(const struct textparser_handle *handle, size_t pos)
{
    switch (handle->text_format)
    {
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
        return pos * sizeof(uint16_t);
    case TEXTPARSER_ENCODING_UTF_32:
        return pos * sizeof(uint32_t);
    default:
        return pos;
    }
}

/**
 * Convert a character code unit length into a byte length based on document text encoding.
 *
 * @param handle Pointer to the textparser handle holding encoding configuration.
 * @param len Length in character code units.
 * @return Length expressed in bytes.
 */
static size_t textparser_get_byte_len(const struct textparser_handle *handle, size_t len)
{
    return textparser_get_byte_offset(handle, len);
}

/**
 * Calculate the total number of character code units in the active document text buffer.
 *
 * @param handle Pointer to the textparser handle.
 * @return Total character code units in text_addr.
 */
static size_t textparser_get_total_units(const struct textparser_handle *handle)
{
    switch (handle->text_format)
    {
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
        return handle->text_size / sizeof(uint16_t);
    case TEXTPARSER_ENCODING_UTF_32:
        return handle->text_size / sizeof(uint32_t);
    default:
        return handle->text_size;
    }
}

/**
 * Retrieve the character code unit at a specific unit offset in the text buffer.
 *
 * @param handle Pointer to the textparser handle.
 * @param pos 0-based character unit offset to read.
 * @return Character code unit value cast to uint32_t.
 */
static uint32_t textparser_get_unit_at(const struct textparser_handle *handle, size_t pos)
{
    switch (handle->text_format)
    {
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
        return ((const uint16_t *)handle->text_addr)[pos];
    case TEXTPARSER_ENCODING_UTF_32:
        return ((const uint32_t *)handle->text_addr)[pos];
    default:
        return (unsigned char)handle->text_addr[pos];
    }
}

/**
 * Determine the code unit length of a single character starting at pos (e.g. multi-byte UTF-8 sequence).
 *
 * @param handle Pointer to the textparser handle.
 * @param pos Character unit offset where the character begins.
 * @return Length of the character in code units (1 to 4 units).
 */
static size_t textparser_char_len(const struct textparser_handle *handle, size_t pos)
{
    if (handle->text_format == TEXTPARSER_ENCODING_UTF_8)
    {
        unsigned char c = (unsigned char)handle->text_addr[pos];
        if ((c & 0x80) == 0) return 1;
        if ((c & 0xE0) == 0xC0) return 2;
        if ((c & 0xF0) == 0xE0) return 3;
        if ((c & 0xF8) == 0xF0) return 4;
        return 1;
    }
    return 1;
}

/**
 * Scan forward from pos to locate the character unit offset of the line ending.
 *
 * @param handle Pointer to the textparser handle.
 * @param pos Starting character unit offset.
 * @return Unit offset immediately preceding the newline sequence, or end-of-text if on the last line.
 */
static size_t textparser_get_end_of_line_units(const struct textparser_handle *handle, size_t pos)
{
    // The parser queries the line end for every single-line candidate at every
    // position. Scanning to the newline each time is O(line length) per call,
    // which is O(n^2) for long lines. Cache the line end: any position in
    // [anchor, end) shares the same newline.
    struct textparser_handle *mutable_handle = (struct textparser_handle *)handle;
    if (mutable_handle->line_cache_end > pos && pos >= mutable_handle->line_cache_anchor) {
        return mutable_handle->line_cache_end - pos;
    }

    size_t total = textparser_get_total_units(handle);
    size_t cur = pos;
    while (cur < total) {
        uint32_t ch = textparser_get_unit_at(handle, cur);
        if (ch == '\n' || ch == '\r')
            break;
        cur++;
    }
    mutable_handle->line_cache_anchor = pos;
    mutable_handle->line_cache_end = cur;
    return cur - pos;
}

/**
 * Determine the maximum searchable unit length from pos depending on the token multi_line configuration.
 *
 * @param handle Pointer to the textparser handle.
 * @param pos Starting unit offset for search.
 * @param token_def Token definition determining whether searching is bounded by the current line.
 * @return Maximum number of character code units available for matching.
 */
static size_t textparser_get_search_len(const struct textparser_handle *handle, size_t pos, const textparser_token *token_def)
{
    size_t len = textparser_get_total_units(handle) - pos;
    if (token_def != nullptr && token_def->multi_line == false) {
        size_t line_len = textparser_get_end_of_line_units(handle, pos);
        if (line_len < len)
            len = line_len;
    }
    return len;
}

/**
 * Validate whether a raw byte buffer conforms to well-formed UTF-8 encoding rules.
 *
 * @param text Pointer to raw byte buffer.
 * @param len Buffer length in bytes.
 * @return true if all byte sequences form valid UTF-8 code points; false otherwise.
 */
static bool textparser_validate_utf8(const char *text, size_t len)
{
    size_t i = 0;
    while (i < len) {
        unsigned char c = (unsigned char)text[i];
        if (c < 0x80) { i += 1; continue; }
        size_t extra;
        uint32_t cp;
        if ((c & 0xE0) == 0xC0)       { extra = 1; cp = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0)  { extra = 2; cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0)  { extra = 3; cp = c & 0x07; }
        else return false;
        if (i + extra >= len) return false;
        for (size_t k = 1; k <= extra; k++) {
            unsigned char cc = (unsigned char)text[i + k];
            if ((cc & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (cc & 0x3F);
        }
        if ((extra == 1 && cp < 0x80) || (extra == 2 && cp < 0x800) ||
            (extra == 3 && cp < 0x10000) || cp > 0x10FFFF ||
            (cp >= 0xD800 && cp <= 0xDFFF))
            return false;
        i += extra + 1;
    }
    return true;
}

/**
 * Validate whether a raw byte buffer conforms to well-formed UTF-16 encoding with valid surrogate pairs.
 *
 * @param handle Pointer to the active textparser handle.
 * @return true if buffer length is 16-bit aligned and surrogates are paired properly; false otherwise.
 */
static bool textparser_validate_utf16(const struct textparser_handle *handle)
{
    size_t units = textparser_get_total_units(handle);
    const uint16_t *u = (const uint16_t *)handle->text_addr;
    for (size_t i = 0; i < units; i++) {
        uint16_t w = u[i];
        if (w >= 0xD800 && w <= 0xDBFF) {
            if (i + 1 >= units) return false;
            uint16_t low = u[i + 1];
            if (!(low >= 0xDC00 && low <= 0xDFFF)) return false;
            i++;
        } else if (w >= 0xDC00 && w <= 0xDFFF) {
            return false;
        }
    }
    return true;
}

/**
 * Validate whether a raw byte buffer conforms to well-formed UTF-32 code points in valid Unicode ranges.
 *
 * @param handle Pointer to the active textparser handle.
 * @return true if buffer length is 32-bit aligned and all values represent valid Unicode scalars; false otherwise.
 */
static bool textparser_validate_utf32(const struct textparser_handle *handle)
{
    size_t units = textparser_get_total_units(handle);
    const uint32_t *u = (const uint32_t *)handle->text_addr;
    for (size_t i = 0; i < units; i++) {
        uint32_t c = u[i];
        if (c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) return false;
    }
    return true;
}

/**
 * Validate the entire active text buffer according to the handle configured text_format encoding.
 *
 * @param handle Pointer to the textparser handle.
 * @return true if the buffer content is valid for handle->text_format; false otherwise.
 */
static bool textparser_validate_text_encoding(const struct textparser_handle *handle)
{
    switch (handle->text_format) {
    case TEXTPARSER_ENCODING_UTF_8:
        return textparser_validate_utf8(handle->text_addr, handle->text_size);
    case TEXTPARSER_ENCODING_UTF_16:
    case TEXTPARSER_ENCODING_UNICODE:
        return textparser_validate_utf16(handle);
    case TEXTPARSER_ENCODING_UTF_32:
        return textparser_validate_utf32(handle);
    default:
        return true; // LATIN1: pcre2 runs without the UTF flag
    }
}




/**
 * Compute an optimal memory chunk size for the arena allocator based on document text size.
 *
 * @param filesize Size of input file in bytes.
 * @return Calculated arena allocation chunk size in bytes.
 */
static size_t calculate_chunk_size(size_t filesize)
{
    size_t min_chunk = 4096;      // 4KB minimum
    size_t max_chunk = 4194304;   // 4MB initial cap

    // Estimate initial arena size as 75% of file size
    size_t estimated = (filesize * 3) / 4;
    if (estimated < min_chunk) {
        return min_chunk;
    }

    size_t chunk_size = min_chunk;
    while (chunk_size < estimated && chunk_size < max_chunk) {
        chunk_size *= 2;
    }
    return chunk_size;
}

/**
 * Convert a UTF-16 Big-Endian text buffer into native host endianness in place.
 *
 * @param src Pointer to UTF-16 Big-Endian byte buffer.
 * @param size Size of the buffer in bytes.
 * @return Pointer to converted native buffer, or NULL on error.
 */
static void *textparser_convert_utf16be_to_native(const char *src, size_t size)
{
    size_t unit_count = size / sizeof(uint16_t);
    if (unit_count == 0)
        return nullptr;

    uint16_t *buf = (uint16_t *)malloc(size);
    if (buf == nullptr)
        return nullptr;

    const uint16_t *units = (const uint16_t *)src;
    for (size_t i = 0; i < unit_count; i++) {
        buf[i] = (uint16_t)((units[i] >> 8) | (units[i] << 8));
    }
    return buf;
}

/**
 * Release all memory chunks held by an arena and reset it.
 *
 * @param arena Pointer to the arena to free.
 */
static void arena_free(textparser_arena *arena)
{
    if (arena->chunks) {
        for (size_t i = 0; i < arena->chunk_count; i++) {
            free(arena->chunks[i]);
        }
        free(arena->chunks);
    }
    memset(arena, 0, sizeof(*arena));
}

/**
 * Reset an arena so its existing chunks are reused from the start,
 * releasing any high-water chunks beyond the base chunk.
 *
 * @param arena Pointer to the arena to reset.
 */
static void arena_reset(textparser_arena *arena)
{
    if (arena->chunk_count > 1) {
        for (size_t i = 1; i < arena->chunk_count; i++) {
            free(arena->chunks[i]);
            arena->chunks[i] = nullptr;
        }
        arena->chunk_count = 1;
    }
    arena->current_chunk_index = 0;
    arena->current_chunk_used = 0;
    arena->current_chunk = (arena->chunk_count > 0) ? arena->chunks[0] : nullptr;
}

/**
 * Allocate zero-initialized, pointer-aligned memory from an arena.
 *
 * @param arena Pointer to the arena.
 * @param size Number of bytes to allocate.
 * @return Pointer to the allocation, or NULL on allocation failure.
 */
static void *arena_alloc(textparser_arena *arena, size_t size)
{
    const size_t align = sizeof(void *);
    size = (size + align - 1) & ~(align - 1);

    if (arena->current_chunk == nullptr ||
        arena->current_chunk_used + size > arena->chunk_size)
    {
        if (arena->current_chunk != nullptr &&
            arena->current_chunk_index + 1 < arena->chunk_count)
        {
            arena->current_chunk_index++;
            arena->current_chunk = arena->chunks[arena->current_chunk_index];
            arena->current_chunk_used = 0;
        }
        else
        {
            size_t chunk_size = (size > arena->chunk_size) ? size : arena->chunk_size;
            void *new_chunk = malloc(chunk_size);
            if (new_chunk == nullptr) {
                return nullptr;
            }

            if (arena->chunk_count >= arena->chunk_capacity) {
                size_t new_capacity = arena->chunk_capacity == 0 ? 4 : arena->chunk_capacity * 2;
                void **new_chunks = realloc(arena->chunks, new_capacity * sizeof(void *));
                if (new_chunks == nullptr) {
                    free(new_chunk);
                    return nullptr;
                }
                arena->chunks = new_chunks;
                arena->chunk_capacity = new_capacity;
            }

            arena->chunks[arena->chunk_count] = new_chunk;
            arena->current_chunk = new_chunk;
            arena->current_chunk_index = arena->chunk_count;
            arena->chunk_count++;
            arena->current_chunk_used = 0;
        }
    }

    void *ret = (char *)arena->current_chunk + arena->current_chunk_used;
    arena->current_chunk_used += size;
    return ret;
}

/**
 * Release all memory chunks managed by the handle's live arena and reset allocation cursors.
 *
 * @param handle Pointer to the textparser handle.
 */
static void free_arena(struct textparser_handle *handle)
{
    arena_free(&handle->arena);
}

typedef struct {
    size_t chunk_index;
    size_t chunk_used;
    size_t chunk_count;
    size_t token_count;
    uint64_t next_node_id;
} textparser_arena_checkpoint;

/**
 * Capture the current arena chunk index and used bytes watermark for speculative rollback.
 *
 * @param handle Pointer to the textparser handle.
 * @return Watermark checkpoint snapshot of current arena state.
 */
static inline textparser_arena_checkpoint textparser_arena_checkpoint_save(const struct textparser_handle *handle)
{
    textparser_arena_checkpoint cp;
    cp.chunk_index = handle->arena.current_chunk_index;
    cp.chunk_used = handle->arena.current_chunk_used;
    cp.chunk_count = handle->arena.chunk_count;
    cp.token_count = handle->token_count;
    cp.next_node_id = handle->next_node_id;
    return cp;
}

/**
 * Roll back arena memory allocations to a previously saved checkpoint watermark.
 *
 * @param handle Pointer to the textparser handle.
 * @param cp Pointer to the checkpoint watermark to restore.
 */
static inline void textparser_arena_checkpoint_restore(struct textparser_handle *handle, const textparser_arena_checkpoint *cp)
{
    handle->arena.current_chunk_index = cp->chunk_index;
    if (handle->arena.chunks && cp->chunk_index < handle->arena.chunk_count) {
        handle->arena.current_chunk = handle->arena.chunks[cp->chunk_index];
    } else {
        handle->arena.current_chunk = nullptr;
    }
    handle->arena.current_chunk_used = cp->chunk_used;
    handle->token_count = cp->token_count;
    handle->next_node_id = cp->next_node_id;
}

/**
 * Attempt to match the start delimiter or initial pattern of a token using fast matchers or regex.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Integer token ID to match.
 * @param text Pointer to current position in text buffer.
 * @param len Length in character code units.
 * @param offset Output pointer receiving matched offset relative to pos.
 * @param match_len Output pointer receiving matched span length.
 * @param only_at_start true if match must be anchored at the current position.
 * @return true if start token matched successfully; false otherwise.
 */
static inline bool textparser_match_start_token(
    const struct textparser_handle *handle,
    int token_id,
    const char *text,
    size_t len,
    size_t *offset,
    size_t *match_len,
    bool only_at_start,
    bool whole_match)
{
    const textparser_token *token_def = &handle->language->tokens[token_id];
    bool ret;
    if (token_def->startRegexFunction != NULL) {
        ret = token_def->startRegexFunction(
            handle->text_format,
            text,
            len,
            offset,
            match_len,
            !handle->language->case_sensitivity,
            only_at_start
        );
    } else {
        ret = adv_regex_find_pattern_capture_ctx(
            handle->regex_ctx,
            token_def->start_regex,
            (void **)handle->start_regex + token_id,
            handle->text_format,
            text,
            len,
            offset,
            match_len,
            !handle->language->case_sensitivity,
            only_at_start,
            whole_match,
            0, NULL, NULL
        );
    }
    return ret;
}

/**
 * Attempt to match the end delimiter or closing pattern of a container token.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Integer token ID of the container.
 * @param text Pointer to current position in text buffer.
 * @param len Length in character code units.
 * @param offset Output pointer receiving matched offset relative to pos.
 * @param match_len Output pointer receiving matched span length.
 * @param only_at_start true if match must be anchored at the current position.
 * @return true if end token matched successfully; false otherwise.
 */
static inline bool textparser_match_end_token(
    const struct textparser_handle *handle,
    int token_id,
    const char *text,
    size_t len,
    size_t *offset,
    size_t *match_len,
    bool only_at_start)
{
    const textparser_token *token_def = &handle->language->tokens[token_id];
    bool ret;
    if (token_def->endRegexFunction != NULL) {
        ret = token_def->endRegexFunction(
            handle->text_format,
            text,
            len,
            offset,
            match_len,
            !handle->language->case_sensitivity,
            only_at_start
        );
    } else {
        ret = adv_regex_find_pattern_ctx(
            handle->regex_ctx,
            token_def->end_regex,
            (void **)handle->end_regex + token_id,
            handle->text_format,
            text,
            len,
            offset,
            match_len,
            !handle->language->case_sensitivity,
            only_at_start
        );
    }
    return ret;
}

/**
 * Allocate a new zero-initialized textparser_token_item node from the handle arena allocator.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Token ID assigned to the node.
 * @param len Length in character code units spanned by the token.
 * @return Pointer to the allocated token item node, or NULL on allocation failure.
 */
static textparser_token_item *textparser_alloc_token(struct textparser_handle *handle, int token_id, size_t len)
{
    textparser_token_item *ret = arena_alloc(&handle->arena, sizeof(textparser_token_item));
    if (ret == nullptr) {
        handle->error = "Can't allocate memory!";
        return nullptr;
    }
    memset(ret, 0, sizeof(textparser_token_item));

    ret->id = ++handle->next_node_id;
    ret->token_id = token_id;
    ret->len = len;
    ret->span_len = len;
    ret->text_color = TEXTPARSER_NOCOLOR;
    ret->text_background = TEXTPARSER_NOCOLOR;
    return ret;
}

/**
 * Check whether any newline character sequence exists in the slice [pos, pos + len).
 *
 * @param handle Pointer to the textparser handle.
 * @param pos Starting unit offset in text buffer.
 * @param len Span length in units to inspect.
 * @return true if a CR, LF, or Unicode line break character is detected; false otherwise.
 */
static bool textparser_has_newline(const struct textparser_handle *handle, size_t pos, size_t len)
{
    if (handle == nullptr || len == 0) return false;

    size_t max_units = textparser_get_total_units(handle);
    if (pos >= max_units) return false;
    size_t end = (pos + len > max_units) ? max_units : pos + len;
    for (size_t c = pos; c < end; c++)
    {
        uint32_t ch = textparser_get_unit_at(handle, c);
        if (ch == '\n' || ch == '\r') return true;
    }

    return false;
}

/**
 * Advance past any contiguous whitespace characters starting at the given position.
 *
 * @param handle Pointer to the textparser handle.
 * @param pos Starting character unit offset.
 * @return Character unit offset of the first non-whitespace character, or end of text.
 */
static size_t textparser_skip_whitespace(const struct textparser_handle *handle, size_t pos)
{
    if (handle == nullptr) return pos;

    size_t max_units = textparser_get_total_units(handle);
    while (pos < max_units)
    {
        uint32_t ch = textparser_get_unit_at(handle, pos);
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
        {
            pos++;
        }
        else
        {
            break;
        }
    }

    return pos;
}
/**
 * Resolve the allowed nested token array for a container, applying parent-context rules if defined.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Token ID of the active container.
 * @param parent_item Parent token node in the CST hierarchy.
 * @return Pointer to a TextParser_END-terminated array of allowed nested token IDs.
 */
static const int *get_effective_nested_tokens(const struct textparser_handle *handle, int token_id, const textparser_token_item *parent_item)
{
    const textparser_language_definition *definition = handle->language;
    const textparser_token *token = &definition->tokens[token_id];

    if (token->context_nested_tokens != nullptr) {
        for (int i = 0; token->context_nested_tokens[i].when_parent_in != nullptr; i++) {
            const int *when_parents = token->context_nested_tokens[i].when_parent_in;
            const textparser_token_item *curr = parent_item;
            while (curr != nullptr) {
                int curr_token_id = curr->token_id;
                for (int p = 0; when_parents[p] != TextParser_END; p++) {
                    if (when_parents[p] == curr_token_id) {
                        return token->context_nested_tokens[i].nested_tokens;
                    }
                }
                curr = curr->parent;
            }
        }
    }

    return token->nested_tokens;
}


/**
 * Check whether a specific token ID is present in a TextParser_END-terminated array.
 *
 * @param list TextParser_END-terminated array of token IDs.
 * @param token_id Token ID to search for.
 * @return true if token_id is found in list; false otherwise.
 */
static bool textparser_token_in_id_list(const int *list, int token_id)
{
    if (list == nullptr) return false;
    for (int i = 0; list[i] != TextParser_END; i++) {
        if (list[i] == token_id) return true;
    }
    return false;
}

/**
 * Locate the last sibling in a linked list of child token items.
 *
 * @param item Current token item node.
 * @return Pointer to the last child token item, or NULL if token has no children.
 */
static textparser_token_item *textparser_get_last_child_item(textparser_token_item *item)
{
    if (item == nullptr) return nullptr;
    while (item->child != nullptr) {
        textparser_token_item *last = item->child;
        while (last->next != nullptr) {
            last = last->next;
        }
        item = last;
    }
    return item;
}

/**
 * Count the number of non-whitespace, non-trivia children under a given token node.
 *
 * @param token Parent token item node.
 * @return Number of semantic child nodes.
 */
static size_t textparser_get_semantic_children_count(const textparser_token_item *token)
{
    if (token == nullptr) return 0;
    size_t ret = 0;
    const textparser_token_item *child = token->child;
    while (child) {
        if (!is_trivia_token_id(child->token_id)) {
            ret++;
        }
        child = child->next;
    }
    return ret;
}

/**
 * Absorb adjacent unary plus/minus signs into numeric literals according to language sign merge rules.
 *
 * @param handle Pointer to the textparser handle.
 * @param n Token item node whose children will be inspected and merged.
 */
static void maybe_merge_sign(struct textparser_handle *handle, textparser_token_item *n)
{
    if (handle == nullptr || handle->language == nullptr || n == nullptr) return;
    const textparser_sign_merge *sign_merge = handle->language->sign_merge;
    if (sign_merge == nullptr || sign_merge->sign_tokens == nullptr || sign_merge->number_tokens == nullptr) return;

    if (!textparser_token_in_id_list(sign_merge->number_tokens, n->token_id)) return;

    textparser_token_item *prev = n->prev;
    if (prev == nullptr) return;

    textparser_token_item *sign = nullptr;
    textparser_token_item *context = nullptr;

    if (textparser_token_in_id_list(sign_merge->sign_tokens, prev->token_id)) {
        sign = prev;
        context = prev->prev;
    } else {
        textparser_token_item *last = textparser_get_last_child_item(prev);
        if (last != nullptr && textparser_token_in_id_list(sign_merge->sign_tokens, last->token_id)) {
            sign = last;
            context = (sign->prev != nullptr) ? sign->prev : prev->prev;
        } else {
            return;
        }
    }

    while (context && is_trivia_token_id(context->token_id)) {
        context = context->prev;
    }
    if (context == nullptr && prev != sign) {
        context = prev->prev;
        while (context && is_trivia_token_id(context->token_id)) {
            context = context->prev;
        }
    }

    // Only literal "+" and "-" are signs; never absorb other operators (e.g. "!3").
    if (sign->len != 1) return;

    // Unary context: the token before the sign must not be an operand.
    if (context != nullptr && textparser_token_in_id_list(sign_merge->operand_tokens, context->token_id)) return;

    size_t sign_pos = textparser_get_token_position(sign);
    uint32_t sign_ch = textparser_get_unit_at(handle, sign_pos);
    if (sign_ch != '+' && sign_ch != '-') return;

    // Absorb the sign into the number.
    n->len += sign->len;

    // Unlink the sign token. In the standalone case sign->next == n, so this
    // also repoints n->prev to the context token (or to null when first child).
    if (sign->prev != nullptr) {
        sign->prev->next = sign->next;
    }
    if (sign->next != nullptr) {
        sign->next->prev = sign->prev;
    }
    if (n->parent != nullptr && n->parent->child == sign) {
        n->parent->child = n;
    }

    // When the sign was the last child of a container (operator group), shrink
    // the container and unwrap it if a single semantic child remains.
    if (prev != sign) {
        if (prev->len >= sign->len) prev->len -= sign->len;
        if (textparser_get_semantic_children_count(prev) == 1) {
            textparser_token_item *first_child = prev->child;
            textparser_token_item *last_child = prev->child;
            for (textparser_token_item *it = first_child; it != nullptr; it = it->next) {
                it->parent = prev->parent;
                last_child = it;
            }
            first_child->prev = prev->prev;
            last_child->next = n;

            if (prev->prev != nullptr) {
                prev->prev->next = first_child;
            } else if (prev->parent != nullptr) {
                prev->parent->child = first_child;
            }
            n->prev = last_child;
        }
    }
}

/**
 * Evaluate contextual disambiguation rules to decide if a slash token begins a regex or is division.
 *
 * @param handle Pointer to the textparser handle.
 * @param parent_item Enclosing parent container node.
 * @param prev_sibling Preceding sibling node in CST.
 * @param pos Position where token appears.
 * @return true if the token is valid in this context; false otherwise.
 */
static bool is_regex_valid_in_context(
    const struct textparser_handle *handle,
    const textparser_token_item *parent_item,
    const textparser_token_item *prev_sibling,
    size_t pos)
{
    (void)pos;
    if (handle == nullptr || handle->language == nullptr) return true;
    const textparser_regex_disambiguation *reg_div = handle->language->regex_disambiguation;
    if (reg_div == nullptr) return true;

    // Find the last non-unprocessed, non-comment token before pos
    const textparser_token_item *curr = prev_sibling;
    const textparser_token_item *prev = nullptr;
    while (curr) {
        if (!is_trivia_token_id(curr->token_id) && curr->token_id >= 0) {
            const char *tok_name = handle->language->tokens[curr->token_id].name;
            if (tok_name && (strstr(tok_name, "Comment") != nullptr || strstr(tok_name, "comment") != nullptr)) {
                curr = curr->prev;
                continue;
            }
            prev = curr;
            break;
        }
        curr = curr->prev;
    }

    if (prev == nullptr && parent_item != nullptr) {
        // At start of container (e.g. inside `(...)` or `{...}` or `[...]`)
        return true;
    }

    if (prev == nullptr) {
        // At start of document
        return true;
    }

    // Check if prev token is in operand tokens
    if (textparser_token_in_id_list(reg_div->operand_tokens, prev->token_id)) {
        // Check special case: Parenthesis condition for control statements (if, while, for, switch, catch, with)
        const char *prev_name = handle->language->tokens[prev->token_id].name;
        if (prev_name && (strcasecmp(prev_name, "Parenthesis") == 0)) {
            // Find token preceding this parenthesis
            const textparser_token_item *before_paren = prev->prev;
            while (before_paren && is_trivia_token_id(before_paren->token_id)) {
                before_paren = before_paren->prev;
            }
            if (before_paren && before_paren->token_id >= 0 && reg_div->control_keywords != nullptr) {
                char *kw_text = textparser_get_token_text((textparser_t)handle, before_paren);
                if (kw_text) {
                    bool is_ctrl = false;
                    for (int k = 0; reg_div->control_keywords[k] != nullptr; k++) {
                        if (strcmp(kw_text, reg_div->control_keywords[k]) == 0) {
                            is_ctrl = true;
                            break;
                        }
                    }
                    textparser_free_token_text(kw_text);
                    if (is_ctrl) {
                        return true; // e.g. `if (x) /abc/` -> regex allowed
                    }
                }
            }
        }

        // If prev is an operand, `/` is division, not regex!
        return false;
    }

    // If prev is ++ or --, check if it was postfix (preceded by operand)
    if (prev->token_id >= 0) {
        char *prev_txt = textparser_get_token_text((textparser_t)handle, prev);
        if (prev_txt) {
            bool is_inc_dec = (strcmp(prev_txt, "++") == 0 || strcmp(prev_txt, "--") == 0);
            textparser_free_token_text(prev_txt);
            if (is_inc_dec) {
                const textparser_token_item *before_op = prev->prev;
                while (before_op && is_trivia_token_id(before_op->token_id)) {
                    before_op = before_op->prev;
                }
                if (before_op && before_op->token_id >= 0 && textparser_token_in_id_list(reg_div->operand_tokens, before_op->token_id)) {
                    return false; // postfix operand -> division
                }
            }
        }
    }

    return true;
}

/**
 * Search for a token match starting at pos, taking context and disambiguation rules into account.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Token ID to search for.
 * @param pos Starting unit offset in text buffer.
 * @param other_text_inside Whether non-token text is permitted before match.
 * @param parent_item Enclosing parent container node.
 * @param prev_sibling Preceding sibling node in CST.
 * @return Relative offset from pos where token begins, or TOKEN_NOT_FOUND (-1).
 */
static ssize_t textparser_find_token(const struct textparser_handle *handle, int token_id, size_t pos, bool other_text_inside, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    if (handle == nullptr || handle->recursion_depth >= MAX_RECURSION_DEPTH) {
        return TOKEN_NOT_FOUND;
    }

    if (pos >= textparser_get_total_units(handle)) {
        return TOKEN_NOT_FOUND;
    }

    const textparser_language_definition *definition = nullptr;
    const textparser_token *token = nullptr;
    const char *text = nullptr;
    size_t found_at = 0;
    size_t len = 0;

    definition = handle->language;
    if (definition == nullptr || definition->tokens == nullptr || token_id < 0 || (size_t)token_id >= handle->token_count) {
        return TOKEN_NOT_FOUND;
    }

    token = &definition->tokens[token_id];
    text = handle->text_addr + textparser_get_byte_offset(handle, pos);
    len = textparser_get_search_len(handle, pos, token);

    LOGV("textparser_find_token token->type [%s] pos %zu", token->name, pos);

    ((struct textparser_handle *)handle)->recursion_depth++;
    ssize_t result = TOKEN_NOT_FOUND;

    switch(token->type)
    {
        case TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY");
            /* fallthrough */
        case TEXTPARSER_TOKEN_TYPE_GROUP:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_GROUP");
            {
                const int *effective_nested = get_effective_nested_tokens(handle, token_id, parent_item);
                if (effective_nested)
                {
                    ssize_t closest_child_pos = SSIZE_MAX;
                    for (int c = 0; effective_nested[c] != TextParser_END; c++)
                    {
                        ssize_t child_token_pos = textparser_find_token(handle, effective_nested[c], pos, token->other_text_inside, parent_item, prev_sibling);
                        if (child_token_pos == TOKEN_NOT_FOUND) continue;
                        if (child_token_pos == 0) {
                            closest_child_pos = 0;
                            break;
                        }

                        if (child_token_pos < closest_child_pos) {
                            closest_child_pos = child_token_pos;
                        }
                    }

                    if (closest_child_pos < SSIZE_MAX) {
                        result = closest_child_pos;
                    }
                }
            }
            break;
        case TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER");
            {
                const int *effective_nested = get_effective_nested_tokens(handle, token_id, parent_item);
                if (effective_nested) {
                    result = textparser_find_token(handle, effective_nested[0], pos, other_text_inside, parent_item, prev_sibling);
                } else {
                    LOGE("nested_tokens = nullptr for TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER");
                }
            }
            break;
        case TEXTPARSER_TOKEN_TYPE_SEQUENCE:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_SEQUENCE");
            {
                const int *effective_nested = get_effective_nested_tokens(handle, token_id, parent_item);
                if (effective_nested && effective_nested[0] != TextParser_END) {
                    result = textparser_find_token(handle, effective_nested[0], pos, other_text_inside, parent_item, prev_sibling);
                } else {
                    LOGE("nested_tokens = nullptr for TEXTPARSER_TOKEN_TYPE_SEQUENCE");
                }
            }
            break;
        case TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN");
            /* fallthrough */
        case TEXTPARSER_TOKEN_TYPE_START_STOP:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_START_STOP");
            /* fallthrough */
        case TEXTPARSER_TOKEN_TYPE_START_OPT_STOP:
            LOGV("textparser_find_token() - TEXTPARSER_TOKEN_TYPE_START_OPT_STOP");
            if (textparser_match_start_token(handle, token_id, text, len, &found_at, nullptr, true, false)) {
                LOGI("found_at token type: [%s] at %zu",  handle->language->tokens[token_id].name, pos + found_at);
                result = (ssize_t)found_at;
                if (result == 0 && handle->language && handle->language->regex_disambiguation) {
                    if (textparser_token_in_id_list(handle->language->regex_disambiguation->regex_tokens, token_id)) {
                        if (!is_regex_valid_in_context(handle, parent_item, prev_sibling, pos)) {
                            result = TOKEN_NOT_FOUND;
                        }
                    }
                }
            }
            break;
        default:
            LOGF("textparser_find_token() - unknown!!!!!");
            break;
    }

    ((struct textparser_handle *)handle)->recursion_depth--;
    return result;
}

static textparser_token_item *textparser_parse_token(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling);

/**
 * Check whether parsing at pos has reached the boundary or closing delimiter of an ancestor token.
 *
 * @param handle Pointer to the textparser handle.
 * @param parent_token_id Token ID of the enclosing parent container.
 * @param parent_start_stop Flag indicating delimiter search direction relative to parent container.
 * @param offset Output pointer receiving matched offset relative to pos.
 * @return true if pos reaches parent boundary or closing delimiter; false otherwise.
 */
static bool check_parent_token_boundary(
    struct textparser_handle *handle,
    int parent_token_id,
    int parent_start_stop,
    size_t offset)
{
    if (parent_token_id < 0) return false;
    const textparser_language_definition *definition = handle->language;
    const textparser_token *parent_def = &definition->tokens[parent_token_id];

    if (parent_start_stop == TEXTPARSER_SEARCH_END_TOKEN)
    {
        if (parent_def->end_regex != nullptr || parent_def->endRegexFunction != NULL)
        {
            size_t token_end = 0;
            size_t end_len = 0;
            bool found_end = textparser_match_end_token(handle, parent_token_id, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, &token_end, &end_len, true);
            if (found_end && token_end == 0)
            {
                return true;
            }
        }
    }
    else if (parent_start_stop == TEXTPARSER_SEARCH_START_TOKEN)
    {
        if (parent_def->start_regex != nullptr || parent_def->startRegexFunction != NULL)
        {
            size_t token_start = 0;
            size_t start_len = 0;
            bool found_start = textparser_match_start_token(handle, parent_token_id, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, &token_start, &start_len, true, false);
            if (found_start && token_start == 0)
            {
                return true;
            }
        }
    }

    return false;
}

/**
 * Append a newly parsed child node to the end of a parent container children linked list.
 *
 * @param parent Parent container token node.
 * @param head In/out pointer to head of node or token list.
 * @param tail In/out pointer to tail of node or token list.
 * @param new_child New child node to link.
 */
static void append_child_to_ast(
    textparser_token_item *parent,
    textparser_token_item **head,
    textparser_token_item **tail,
    textparser_token_item *new_child)
{
    new_child->parent = parent;
    if (*tail != nullptr)
    {
        (*tail)->next = new_child;
        new_child->prev = *tail;
        *tail = new_child;
    }
    else
    {
        *head = new_child;
        *tail = new_child;
        new_child->prev = nullptr;
    }
}

/**
 * Synthesize and append a start delimiter leaf node to the parent container AST.
 *
 * @param handle Pointer to the textparser handle.
 * @param parent Parent container token node.
 * @param head In/out pointer to head of node or token list.
 * @param tail In/out pointer to tail of node or token list.
 * @param len Length of the start delimiter in units.
 * @param text_color ARGB foreground text color code.
 * @param text_background ARGB background text color code.
 * @param text_flags Text styling bitflags.
 */
static void append_start_delimiter(
    struct textparser_handle *handle,
    textparser_token_item *parent,
    textparser_token_item **head,
    textparser_token_item **tail,
    size_t len,
    uint32_t text_color,
    uint32_t text_background,
    uint32_t text_flags)
{
    if (len == 0) return;
    textparser_token_item *item = textparser_alloc_token(handle, TEXTPARSER_TOKEN_ID_START_DELIMITER, len);
    if (item == nullptr) return;
    item->text_color = text_color;
    item->text_background = text_background;
    item->text_flags = text_flags;
    append_child_to_ast(parent, head, tail, item);
}

/**
 * Synthesize and append an end delimiter leaf node to the parent container AST.
 *
 * @param handle Pointer to the textparser handle.
 * @param parent Parent container token node.
 * @param head In/out pointer to head of node or token list.
 * @param tail In/out pointer to tail of node or token list.
 * @param len Length of the end delimiter in units.
 * @param text_color ARGB foreground text color code.
 * @param text_background ARGB background text color code.
 * @param text_flags Text styling bitflags.
 */
static void append_end_delimiter(
    struct textparser_handle *handle,
    textparser_token_item *parent,
    textparser_token_item **head,
    textparser_token_item **tail,
    size_t len,
    uint32_t text_color,
    uint32_t text_background,
    uint32_t text_flags)
{
    if (len == 0) return;
    textparser_token_item *item = textparser_alloc_token(handle, TEXTPARSER_TOKEN_ID_END_DELIMITER, len);
    if (item == nullptr) return;
    item->text_color = text_color;
    item->text_background = text_background;
    item->text_flags = text_flags;
    append_child_to_ast(parent, head, tail, item);
}

/**
 * Create and append a whitespace trivia node if ws_skipped length is greater than zero.
 *
 * @param handle Pointer to the textparser handle.
 * @param parent Parent container token node (or NULL if top level).
 * @param head In/out pointer to the head of the token list.
 * @param tail In/out pointer to tail of node or token list.
 * @param len Length in character code units.
 */
static textparser_token_item *append_whitespace_if_needed(
    struct textparser_handle *handle,
    textparser_token_item *parent,
    textparser_token_item **head,
    textparser_token_item **tail,
    size_t len)
{
    if (len == 0) return *tail;
    if (*tail != nullptr && (*tail)->token_id == TEXTPARSER_TOKEN_ID_WHITESPACE)
    {
        (*tail)->len += len;
        return *tail;
    }
    textparser_token_item *item = textparser_alloc_token(handle, TEXTPARSER_TOKEN_ID_WHITESPACE, len);
    if (item == nullptr) return *tail;
    append_child_to_ast(parent, head, tail, item);
    return item;
}

/**
 * Create and append an unprocessed leaf node for source spans not matched by grammar tokens.
 *
 * @param handle Pointer to the textparser handle.
 * @param parent Parent container token node (or NULL if top level).
 * @param head In/out pointer to the head of the token list.
 * @param tail In/out pointer to tail of node or token list.
 * @param len Length of unprocessed span in units.
 */
static textparser_token_item *append_unprocessed_if_needed(
    struct textparser_handle *handle,
    textparser_token_item *parent,
    textparser_token_item **head,
    textparser_token_item **tail,
    size_t len)
{
    if (len == 0) return *tail;
    if (*tail != nullptr && (*tail)->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED)
    {
        (*tail)->len += len;
        return *tail;
    }
    textparser_token_item *item = textparser_alloc_token(handle, TEXTPARSER_TOKEN_ID_UNPROCESSED, len);
    if (item == nullptr) return *tail;
    append_child_to_ast(parent, head, tail, item);
    return item;
}

/**
 * Parse a group container token whose grammar requires exactly one matching child.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Token ID of the group container.
 * @param parent_token_id Token ID of the enclosing parent container.
 * @param parent_start_stop Flag indicating delimiter search direction relative to parent container.
 * @param offset Output pointer receiving matched offset relative to pos.
 * @param parent_item Enclosing parent container node.
 * @param prev_sibling Preceding sibling node.
 * @return Allocated container token item node on success, or NULL on mismatch.
 */
static textparser_token_item *parse_token_group_one_child_only(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    (void)parent_item;
    textparser_token_item *ret = nullptr;

    if (handle == nullptr)
    {
        LOGF("handle == nullptr");
        return nullptr;
    }

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];
    textparser_token_item *child = nullptr;

    LOGV("enter TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY");
    const int *effective_nested = get_effective_nested_tokens(handle, token_id, parent_item);
    if (!effective_nested) {
        exit_with_error(handle, "group_one_child token type nested_tokens list is empty!", offset, 0);
    }

    size_t start_offset = offset;

    ret = textparser_alloc_token(handle, token_id, 0);
    if (ret == nullptr) {
        return nullptr;
    }

    ret->parent = (textparser_token_item *)parent_item;
    ret->prev = (textparser_token_item *)prev_sibling;

    LOGV("id: %d - [%s]  at offset: %zu", token_id, token_def->name, offset);

    textparser_token_item *last_child = nullptr;
    for (int c = 0; effective_nested[c] != TextParser_END; c++)
    {
        int cand_id = effective_nested[c];
        ssize_t cand_pos = textparser_find_token(handle, cand_id, offset, token_def->other_text_inside, parent_item, prev_sibling);
        if (cand_pos == 0)
        {
            textparser_arena_checkpoint cp = textparser_arena_checkpoint_save(handle);
            const char *saved_err = handle->error;
            size_t saved_err_off = handle->error_offset;

            child = textparser_parse_token(handle, cand_id, parent_token_id, parent_start_stop, offset, ret, last_child);
            if (child != nullptr && handle->error == nullptr && child->len > 0) {
                break;
            }

            textparser_arena_checkpoint_restore(handle, &cp);
            handle->error = saved_err;
            handle->error_offset = saved_err_off;
            child = nullptr;
        }
    }

    if (child == nullptr)
    {
        size_t closest = SIZE_MAX;
        int current_token_id = TextParser_END;
        for (int c = 0; effective_nested[c] != TextParser_END; c++)
        {
            ssize_t current_closest = textparser_find_token(handle, effective_nested[c], offset, token_def->other_text_inside, parent_item, prev_sibling);
            if ((current_closest > 0) && ((size_t)current_closest < closest))
            {
                closest = (size_t)current_closest;
                current_token_id = effective_nested[c];
            }
        }

        if (current_token_id == TextParser_END)
        {
            exit_with_error(handle, "Search for group_one_child token type failed. Can't find one child.", offset, 0);
        }

        if (closest > 0) {
            append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, closest);
            offset += closest;
        }

        child = textparser_parse_token(handle, current_token_id, parent_token_id, parent_start_stop, offset, ret, last_child);
        if (child == nullptr) {
            exit_with_error(handle, "Search for group_one_child token type failed. Child token parsing failed.", offset, 0);
        }
    }

    append_child_to_ast(ret, &ret->child, &last_child, child);
    LOGV("TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY - Found [%s]", handle->language->tokens[child->token_id].name);
    offset += child->len;
    ret->len = offset - start_offset;
    check_and_exit_on_fatal_parsing_error(handle, child, offset);

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

/**
 * Parse a general group container token matching multiple nested tokens.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Token ID of the group container.
 * @param parent_token_id Token ID of the enclosing parent container.
 * @param parent_start_stop Flag indicating delimiter search direction relative to parent container.
 * @param offset Output pointer receiving matched offset relative to pos.
 * @param parent_item Enclosing parent container node.
 * @param prev_sibling Preceding sibling node.
 * @return Allocated container token item node on success, or NULL on mismatch.
 */
static textparser_token_item *parse_token_group(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    textparser_token_item *ret = nullptr;

    if (handle == nullptr)
    {
        LOGF("handle == nullptr");
        return nullptr;
    }

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];
    textparser_token_item *child = nullptr;

    LOGV("enter TEXTPARSER_TOKEN_TYPE_GROUP");
    const int *effective_nested = get_effective_nested_tokens(handle, token_id, parent_item);
    if (!effective_nested) {
        exit_with_error(handle, "nested_tokens list is empty!", offset, 0);
    }

    size_t start_offset = offset;

    ret = textparser_alloc_token(handle, token_id, 0);
    if (ret == nullptr) {
        return nullptr;
    }

    ret->parent = (textparser_token_item *)parent_item;
    ret->prev = (textparser_token_item *)prev_sibling;

    LOGV("id: %d - [%s]  at offset: %zu", token_id, token_def->name, offset);
    while(1) {
        size_t ws_skipped = textparser_skip_whitespace(handle, offset) - offset;
        if (ws_skipped > 0) {
            append_whitespace_if_needed(handle, ret, &ret->child, &child, ws_skipped);
            offset += ws_skipped;
        }

        if (offset >= textparser_get_total_units(handle))
        {
            if (child)
            {
                ret->len = offset - start_offset;
                goto exit;
            }
            exit_with_error(handle, "Search for group token type failed. Can't find any child.", offset, 0);
        }

        const textparser_token_item *current_prev = child;

        if (check_parent_token_boundary(handle, parent_token_id, parent_start_stop, offset)) {
            ret->len = offset - start_offset;
            break;
        }

        const int *loop_effective_nested = get_effective_nested_tokens(handle, token_id, ret);
        textparser_token_item *new_child = nullptr;
        textparser_token_item *error_child = nullptr;
        const char *error_child_msg = nullptr;
        size_t error_child_off = 0;
        if (loop_effective_nested) {
            for (int c = 0; loop_effective_nested[c] != TextParser_END; c++) {
                int cand_id = loop_effective_nested[c];
                ssize_t found = textparser_find_token(handle, cand_id, offset, token_def->other_text_inside, ret, current_prev);
                if (found == 0) {
                    textparser_arena_checkpoint cp = textparser_arena_checkpoint_save(handle);
                    const char *saved_err = handle->error;
                    size_t saved_err_off = handle->error_offset;

                    textparser_token_item *attempt = textparser_parse_token(handle, cand_id, parent_token_id, parent_start_stop, offset, ret, current_prev);
                    if (attempt != nullptr && handle->error == nullptr && attempt->len > 0) {
                        new_child = attempt;
                        break;
                    }

                    if (attempt != nullptr && handle->error != nullptr && error_child == nullptr) {
                        // Keep the first failing child and its diagnostic, but
                        // clear the error so later candidates can still be tried.
                        error_child = attempt;
                        error_child_msg = handle->error;
                        error_child_off = handle->error_offset;
                        handle->error = saved_err;
                        handle->error_offset = saved_err_off;
                    } else {
                        textparser_arena_checkpoint_restore(handle, &cp);
                        handle->error = saved_err;
                        handle->error_offset = saved_err_off;
                    }
                }
            }
        }

        if (new_child == nullptr && error_child != nullptr) {
            new_child = error_child;
            handle->error = error_child_msg;
            handle->error_offset = error_child_off;
        }

        if (new_child != nullptr)
        {
            if (handle->error) {
                if (token_def->other_text_inside && !handle->parse_budget_exceeded && offset < textparser_get_total_units(handle)) {
                    handle->error = nullptr;
                    handle->error_offset = 0;
                    handle->error_length = 0;
                    size_t char_l = textparser_char_len(handle, offset);
                    append_unprocessed_if_needed(handle, ret, &ret->child, &child, char_l);
                    offset += char_l;
                    continue;
                }
                goto exit;
            }

            size_t child_advance = new_child->len;
            append_child_to_ast(ret, &ret->child, &child, new_child);
            maybe_merge_sign(handle, child);

            if (child->len == 0) {
                exit_with_error(handle, "0-length child token match caused infinite loop", offset, 0);
            }

            offset += child_advance;
            ret->len = offset - start_offset;
        }
        else
        {
            if (token_def->other_text_inside && !handle->parse_budget_exceeded && offset < textparser_get_total_units(handle))
            {
                size_t char_l = textparser_char_len(handle, offset);
                append_unprocessed_if_needed(handle, ret, &ret->child, &child, char_l);
                offset += char_l;
            }
            else
            {
                if (child)
                {
                    ret->len = offset - start_offset;
                    goto exit;
                }
                exit_with_error(handle, "Unrecognized token inside group", offset, 1);
            }
        }
    }

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

/**
 * Parse a group container token requiring all child tokens to appear in fixed sequential order.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Token ID of the group container.
 * @param parent_token_id Token ID of the enclosing parent container.
 * @param parent_start_stop Flag indicating delimiter search direction relative to parent container.
 * @param offset Output pointer receiving matched offset relative to pos.
 * @param parent_item Enclosing parent container node.
 * @param prev_sibling Preceding sibling node.
 * @return Allocated container token item node on success, or NULL on mismatch.
 */
static textparser_token_item *parse_token_group_all_children_in_same_order(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    textparser_token_item *ret = nullptr;

    if (handle == nullptr)
    {
        LOGF("handle == nullptr");
        return nullptr;
    }

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];
    textparser_token_item *child = nullptr;
    textparser_token_item *last_child = nullptr;

    LOGV("enter TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER");
    if (!token_def->nested_tokens) {
        exit_with_error(handle, "nested_tokens list is empty!", offset, 0);
    }

    int nested_count = 0;
    while(token_def->nested_tokens[nested_count] != TextParser_END) nested_count++;

    if (nested_count != 3) {
         exit_with_error(handle, "GroupAllChildrenInSameOrder should have exactly 3 nested tokens", offset, 0);
    }

    int start_token_id = token_def->nested_tokens[0];
    int inner_token_id = token_def->nested_tokens[1];
    int end_token_id   = token_def->nested_tokens[2];

    size_t start_offset = offset;

    ret = textparser_alloc_token(handle, token_id, 0);
    if (ret == nullptr) {
        return nullptr;
    }

    ret->parent = (textparser_token_item *)parent_item;
    ret->prev = (textparser_token_item *)prev_sibling;

    ssize_t start_pos = textparser_find_token(handle, start_token_id, offset, definition->other_text_inside, ret, prev_sibling);
    if (start_pos != 0) {
        exit_with_error(handle, "Expected start token!", offset, 1);
    }

    child = textparser_parse_token(handle, start_token_id, parent_token_id, parent_start_stop, offset, ret, prev_sibling);
    if (child == nullptr) {
        exit_with_error(handle, "Parsing start token failed", offset, 0);
    }
    append_child_to_ast(ret, &ret->child, &last_child, child);
    maybe_merge_sign(handle, child);
    check_and_exit_on_fatal_parsing_error(handle, child, offset);

    offset += child->len;

    while(1)
    {
        size_t ws_skipped = textparser_skip_whitespace(handle, offset) - offset;
        if (ws_skipped > 0) {
            append_whitespace_if_needed(handle, ret, &ret->child, &last_child, ws_skipped);
            offset += ws_skipped;
        }

        if (offset >= textparser_get_total_units(handle)) {
            exit_with_error(handle, "Expected end token, reached end of text!", offset, 0);
        }

        ssize_t end_pos   = textparser_find_token(handle, end_token_id,   offset, definition->other_text_inside, ret, last_child);
        if (end_pos == 0) {
            break;
        }

        ssize_t inner_pos = textparser_find_token(handle, inner_token_id, offset, definition->other_text_inside, ret, last_child);
        if (inner_pos == 0) {
            child = textparser_parse_token(handle, inner_token_id, end_token_id, TEXTPARSER_SEARCH_START_TOKEN, offset, ret, last_child);
            if (child == nullptr) {
                exit_with_error(handle, "Parsing inner token failed", offset, 0);
            }
            size_t child_advance = child->len;
            append_child_to_ast(ret, &ret->child, &last_child, child);
            maybe_merge_sign(handle, child);
            check_and_exit_on_fatal_parsing_error(handle, child, offset);

            if (child->len == 0) {
                exit_with_error(handle, "0-length child token match caused infinite loop", offset, 0);
            }

            offset += child_advance;
            continue;
        }

        if (definition->other_text_inside) {
            size_t char_l = textparser_char_len(handle, offset);
            append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, char_l);
            offset += char_l;
        } else {
            exit_with_error(handle, "Expected inner or end token!", offset, 1);
        }
    }

    size_t ws_skipped_end = textparser_skip_whitespace(handle, offset) - offset;
    if (ws_skipped_end > 0) {
        append_whitespace_if_needed(handle, ret, &ret->child, &last_child, ws_skipped_end);
        offset += ws_skipped_end;
    }

    child = textparser_parse_token(handle, end_token_id, parent_token_id, parent_start_stop, offset, ret, last_child);
    if (child == nullptr) {
        exit_with_error(handle, "Parsing end token failed", offset, 0);
    }
    append_child_to_ast(ret, &ret->child, &last_child, child);
    maybe_merge_sign(handle, child);
    check_and_exit_on_fatal_parsing_error(handle, child, offset);

    offset += child->len;
    ret->len = offset - start_offset;

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

/**
 * Parse a sequence container token matching children sequentially with backtracking on failure.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Token ID of the sequence container.
 * @param parent_token_id Token ID of the enclosing parent container.
 * @param parent_start_stop Flag indicating delimiter search direction relative to parent container.
 * @param offset Output pointer receiving matched offset relative to pos.
 * @param parent_item Enclosing parent container node.
 * @param prev_sibling Preceding sibling node.
 * @return Allocated sequence token item node on success, or NULL on mismatch.
 */
static textparser_token_item *parse_token_sequence(
    struct textparser_handle *handle,
    int token_id,
    int parent_token_id,
    int parent_start_stop,
    size_t offset,
    const textparser_token_item *parent_item,
    const textparser_token_item *prev_sibling)
{
    (void)parent_token_id;
    (void)parent_start_stop;
    if (handle == nullptr) return nullptr;

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];
    const int *nested = get_effective_nested_tokens(handle, token_id, parent_item);
    if (!nested || nested[0] == TextParser_END) {
        return nullptr;
    }

    textparser_arena_checkpoint cp = textparser_arena_checkpoint_save(handle);
    const char *saved_error = handle->error;
    size_t saved_error_offset = handle->error_offset;

    size_t start_offset = offset;
    textparser_token_item *ret = textparser_alloc_token(handle, token_id, 0);
    if (!ret) {
        return nullptr;
    }
    ret->parent = (textparser_token_item *)parent_item;
    ret->prev = (textparser_token_item *)prev_sibling;
    textparser_token_item *last_child = nullptr;

    for (int i = 0; nested[i] != TextParser_END; i++) {
        int elem_id = nested[i];

        size_t ws_skipped = textparser_skip_whitespace(handle, offset) - offset;
        if (ws_skipped > 0) {
            append_whitespace_if_needed(handle, ret, &ret->child, &last_child, ws_skipped);
            offset += ws_skipped;
        }

        if (offset >= textparser_get_total_units(handle)) {
            textparser_arena_checkpoint_restore(handle, &cp);
            handle->error = saved_error;
            handle->error_offset = saved_error_offset;
            return nullptr;
        }

        ssize_t found = textparser_find_token(handle, elem_id, offset, token_def->other_text_inside, ret, last_child);
        if (found != 0) {
            textparser_arena_checkpoint_restore(handle, &cp);
            handle->error = saved_error;
            handle->error_offset = saved_error_offset;
            return nullptr;
        }

        textparser_token_item *elem_item = textparser_parse_token(handle, elem_id, token_id, TEXTPARSER_SEARCH_START_TOKEN, offset, ret, last_child);
        if (elem_item == nullptr || handle->error != nullptr || elem_item->len == 0) {
            textparser_arena_checkpoint_restore(handle, &cp);
            handle->error = saved_error;
            handle->error_offset = saved_error_offset;
            return nullptr;
        }

        append_child_to_ast(ret, &ret->child, &last_child, elem_item);
        maybe_merge_sign(handle, elem_item);
        offset += elem_item->len;
    }

    ret->len = offset - start_offset;
    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }
    return ret;
}

/**
 * Parse a simple leaf token matching a single regex or literal pattern.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Token ID of the simple token.
 * @param parent_token_id Token ID of the enclosing parent container.
 * @param parent_start_stop Flag indicating delimiter search direction relative to parent container.
 * @param offset Output pointer receiving matched offset relative to pos.
 * @param parent_item Parent container CST node in the hierarchy.
 * @param prev_sibling Preceding sibling CST node in the linked list.
 * @return Allocated leaf token item node on success, or NULL on mismatch.
 */
static textparser_token_item *parse_token_simple_token(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    (void)parent_item;
    (void)prev_sibling;
    (void)parent_start_stop;
    (void)parent_token_id;
    textparser_token_item *ret = nullptr;

    if (handle == nullptr)
    {
        LOGF("handle == nullptr");
        return nullptr;
    }

    LOGV("enter TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN");

    if (offset >= textparser_get_total_units(handle)) {
        exit_with_error(handle, "offset >= total units count!", offset, 0);
    }

    ret = textparser_alloc_token(handle, token_id, 0);
    if (ret == nullptr) {
        exit_with_error(handle, "Can't allocate memory!", offset, 0);
    }

    size_t len = 0;
    if (!textparser_match_start_token(handle, token_id, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &len, true, false)) {
        exit_with_error(handle, "Can't find start of the token!", offset, 1);
    }

    LOGV("TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN - Found [%s]", handle->language->tokens[ret->token_id].name);
    ret->len = len;
    ret->parent = (textparser_token_item *)parent_item;
    ret->prev = (textparser_token_item *)prev_sibling;

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

/**
 * Parse a paired start/stop container token matching nested content until its closing delimiter.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Token ID of the start/stop container.
 * @param parent_token_id Token ID of the enclosing parent container.
 * @param parent_start_stop Flag indicating search direction relative to parent delimiters.
 * @param offset Output pointer receiving matched offset relative to pos.
 * @param stop_required Whether closing delimiter must be matched.
 * @param parent_item Enclosing parent container node.
 * @param prev_sibling Preceding sibling node.
 * @return Allocated container token item node on success, or NULL on mismatch.
 */
static textparser_token_item *parse_token_start_stop(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, bool stop_required, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    (void)prev_sibling;
    (void)parent_start_stop;
    (void)parent_token_id;
    textparser_token_item *ret = nullptr;
    textparser_token_item *child = nullptr;

    size_t len = 0;
    size_t token_end = 0;

    if (handle == nullptr) {
        exit_with_error(handle, "handle == nullptr!", offset, 0);
    }

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];

    if (stop_required) {
        LOGV("enter TEXTPARSER_TOKEN_TYPE_START_STOP");
    } else {
        LOGV("enter TEXTPARSER_TOKEN_TYPE_START_OPT_STOP");
    }

    if (offset >= textparser_get_total_units(handle)) {
        exit_with_error(handle, "offset >= total units count!", offset, 0);
    }

    size_t start_offset = offset;

    ret = textparser_alloc_token(handle, token_id, 0);
    if (ret == nullptr) {
        exit_with_error(handle, "Can't allocate memory!", offset, 0);
    }

    ret->parent = (textparser_token_item *)parent_item;
    ret->prev = (textparser_token_item *)prev_sibling;

    // Search for start token
    if (!textparser_match_start_token(handle, token_id, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &len, true, false)) {
        exit_with_error(handle, "Can't find start of the token!", offset, 1);
    }

    uint32_t delim_color = (token_def->delimiter_text_color != TEXTPARSER_NOCOLOR) ? token_def->delimiter_text_color : token_def->text_color;
    uint32_t delim_bg = (token_def->delimiter_text_background != TEXTPARSER_NOCOLOR) ? token_def->delimiter_text_background : token_def->text_background;
    uint32_t delim_flags = (token_def->delimiter_text_flags != 0) ? token_def->delimiter_text_flags : token_def->text_flags;

    textparser_token_item *last_child = nullptr;
    append_start_delimiter(handle, ret, &ret->child, &last_child, len, delim_color, delim_bg, delim_flags);
    offset += len;

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_START, handle->user_data);
    }

    if (offset > textparser_get_total_units(handle)) {
        exit_with_error(handle, "offset >= total units count!", offset, 0);
    }

    if (offset == textparser_get_total_units(handle)) {
        if (token_def->type == TEXTPARSER_TOKEN_TYPE_START_STOP) {
            exit_with_error(handle, "reached end of text!", offset, 0);
        } else {
            ret->len = offset - start_offset;
            goto exit;
        }
    }

    const int *effective_nested = get_effective_nested_tokens(handle, token_id, ret);
    if (effective_nested)
    {
        const int *nested_tokens = effective_nested;

        while (1) {
            size_t ws_skipped = textparser_skip_whitespace(handle, offset) - offset;
            if (ws_skipped > 0) {
                append_whitespace_if_needed(handle, ret, &ret->child, &last_child, ws_skipped);
                offset += ws_skipped;
            }

            if (token_def->search_parent_end_token_last == false)
            {
                size_t end_match_len = 0;
                bool found_end = textparser_match_end_token(handle, token_id, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &end_match_len, true);
                if (found_end)
                {
                    break;
                }
            }

            if (offset >= textparser_get_total_units(handle))
            {
                if (token_def->search_parent_end_token_last == true)
                {
                    size_t end_match_len = 0;
                    bool found_end = textparser_match_end_token(handle, token_id, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &end_match_len, true);
                    if (found_end)
                    {
                        break;
                    }
                }
                if (token_def->type == TEXTPARSER_TOKEN_TYPE_START_STOP) {
                    exit_with_error(handle, "Reached end of text before finding end token!", start_offset, offset - start_offset);
                } else {
                    break;
                }
            }

            const textparser_token_item *current_prev = last_child;
            textparser_token_item *new_child = nullptr;
            textparser_token_item *error_child = nullptr;
            const char *error_child_msg = nullptr;
            size_t error_child_off = 0;
            if (nested_tokens) {
                for (int c = 0; nested_tokens[c] != TextParser_END; c++) {
                    int cand_id = nested_tokens[c];
                    ssize_t found = textparser_find_token(handle, cand_id, offset, token_def->other_text_inside, ret, current_prev);
                    if (found == 0) {
                        textparser_arena_checkpoint cp = textparser_arena_checkpoint_save(handle);
                        const char *saved_err = handle->error;
                        size_t saved_err_off = handle->error_offset;

                        textparser_token_item *attempt = textparser_parse_token(handle, cand_id, token_id, TEXTPARSER_SEARCH_END_TOKEN, offset, ret, current_prev);
                        if (attempt != nullptr && handle->error == nullptr && attempt->len > 0) {
                            new_child = attempt;
                            break;
                        }

                        if (attempt != nullptr && handle->error != nullptr && error_child == nullptr) {
                            // Keep the first failing child and its diagnostic,
                            // but clear the error so later candidates can still
                            // be tried.
                            error_child = attempt;
                            error_child_msg = handle->error;
                            error_child_off = handle->error_offset;
                            handle->error = saved_err;
                            handle->error_offset = saved_err_off;
                        } else {
                            textparser_arena_checkpoint_restore(handle, &cp);
                            handle->error = saved_err;
                            handle->error_offset = saved_err_off;
                        }
                    }
                }
            }

            if (new_child == nullptr && error_child != nullptr) {
                new_child = error_child;
                handle->error = error_child_msg;
                handle->error_offset = error_child_off;
            }

            if (new_child != nullptr)
            {
                if (handle->error) {
                    if (token_def->other_text_inside && !handle->parse_budget_exceeded && offset < textparser_get_total_units(handle)) {
                        handle->error = nullptr;
                        handle->error_offset = 0;
                        handle->error_length = 0;
                        size_t char_l = textparser_char_len(handle, offset);
                        append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, char_l);
                        offset += char_l;
                        continue;
                    }
                    goto exit;
                }

                child = new_child;
                size_t child_advance = new_child->len;
                append_child_to_ast(ret, &ret->child, &last_child, child);
                maybe_merge_sign(handle, child);

                if (child->len == 0) {
                    exit_with_error(handle, "0-length child token match caused infinite loop", offset, 0);
                }

                offset += child_advance;
                check_and_exit_on_fatal_parsing_error(handle, child, offset);
                continue;
            }

            if (token_def->search_parent_end_token_last == true)
            {
                size_t end_match_len = 0;
                bool found_end = textparser_match_end_token(handle, token_id, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &end_match_len, true);
                if (found_end)
                {
                    break;
                }
            }

            if (token_def->other_text_inside) {
                size_t char_l = textparser_char_len(handle, offset);
                append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, char_l);
                offset += char_l;
            } else {
                exit_with_error(handle, "Unexpected token inside start-stop block!", offset, 1);
            }
        }
    }

    size_t ws_skipped_final = textparser_skip_whitespace(handle, offset) - offset;
    if (ws_skipped_final > 0) {
        append_whitespace_if_needed(handle, ret, &ret->child, &last_child, ws_skipped_final);
        offset += ws_skipped_final;
    }

    size_t end_len = 0;
    bool end_only_at_start = false;
    size_t end_search_len = textparser_get_total_units(handle) - offset;
    bool found_end = false;

    if (token_def->other_text_inside == false && effective_nested != nullptr) {
        // Structured content (nested tokens only): the parser loop stops exactly
        // at the end token, so match it at the current position only.
        end_only_at_start = true;
        end_search_len = textparser_get_search_len(handle, offset, token_def);
    } else if (token_def->other_text_inside == true && token_def->multi_line == false) {
        // Single-line token with arbitrary text inside: bound the search to the
        // current line; if the end token is not there, fall back to the full
        // remainder so the multi-line-span validation can still raise its
        // specific error for malformed input.
        end_search_len = textparser_get_search_len(handle, offset, token_def);
    }

    found_end = textparser_match_end_token(handle, token_id, handle->text_addr + textparser_get_byte_offset(handle, offset), end_search_len, &token_end, &end_len, end_only_at_start);

    if (!found_end && token_def->other_text_inside == true && token_def->multi_line == false) {
        found_end = textparser_match_end_token(handle, token_id, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, &token_end, &end_len, false);
    }

    if (!found_end) {
        if (token_def->type == TEXTPARSER_TOKEN_TYPE_START_STOP) {
            LOGE("Can't find [%s] at %zu. Text: [%s]", token_def->end_regex, offset, handle->text_addr + textparser_get_byte_offset(handle, offset));
            exit_with_error(handle, "Can't find end of the token!", offset, textparser_get_total_units(handle) - offset);
        } else {
            ret->len = offset - start_offset;
            goto exit;
        }
    }

    LOGV("TEXTPARSER_TOKEN_TYPE_START_(OPT)_STOP - Found [%s]", handle->language->tokens[ret->token_id].name);
    if (token_end > 0) {
        append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, token_end);
    }
    if (end_len > 0) {
        append_end_delimiter(handle, ret, &ret->child, &last_child, end_len, delim_color, delim_bg, delim_flags);
    }
    offset += token_end + end_len;
    ret->len = offset - start_offset;

    bool has_custom_delimiter_styling = (token_def->delimiter_text_color != TEXTPARSER_NOCOLOR) ||
                                        (token_def->delimiter_text_background != TEXTPARSER_NOCOLOR) ||
                                        (token_def->delimiter_text_flags != 0);

    if (!has_custom_delimiter_styling) {
        if (ret->child && ret->child->token_id == TEXTPARSER_TOKEN_ID_START_DELIMITER &&
            ret->child->next && ret->child->next->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED &&
            ret->child->next->next && ret->child->next->next->token_id == TEXTPARSER_TOKEN_ID_END_DELIMITER &&
            ret->child->next->next->next == nullptr) {
            ret->child = nullptr;
        } else if (ret->child && ret->child->token_id == TEXTPARSER_TOKEN_ID_START_DELIMITER &&
                   ret->child->next && ret->child->next->token_id == TEXTPARSER_TOKEN_ID_END_DELIMITER &&
                   ret->child->next->next == nullptr) {
            ret->child = nullptr;
        } else if (ret->child && ret->child->next == nullptr && ret->child->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED && ret->child->len == ret->len) {
            ret->child = nullptr;
        }
    }

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

/**
 * Record a fatal parsing syntax error and set handle error state.
 *
 * @param handle Pointer to the textparser handle.
 * @param msg Error message string.
 * @param offset Character unit offset where error occurred.
 * @return Always returns NULL.
 */
static textparser_token_item *parse_token_error_error(struct textparser_handle *handle, const char *msg, size_t offset)
{
    exit_with_error(handle, msg, offset, 0);
exit:
    return nullptr;
}

/*
 * Packrat memoization of legacy parser failures.
 *
 * The parser retries candidate tokens speculatively. On deeply nested ambiguous
 * input a failed sub-parse can be re-run an exponential number of times. A
 * failure of `token_id` at `offset` under `parent_token_id` is a property of the
 * text and the parent's nested rules, so it can be cached and skipped on repeat.
 *
 * Only failures are cached: a success carries arena node pointers that a
 * checkpoint rollback would invalidate. The cached `error` string is a static
 * literal (all `exit_with_error` messages are).
 */
typedef struct textparser_parse_memo {
    int token_id;
    size_t offset;
    int parent_token_id;
    const char *error;
    size_t error_offset;
    size_t error_length;
    struct textparser_parse_memo *next;
} textparser_parse_memo;

#define TEXTPARSER_PARSE_MEMO_BUCKETS 8192u
#define TEXTPARSER_PARSE_MEMO_MAX_ENTRIES 4000000u

static size_t textparser_parse_memo_hash(int token_id, size_t offset, int parent_token_id)
{
    uint64_t h = (uint64_t)(uint32_t)token_id * 0x9E3779B185EBCA87ull;
    h ^= (uint64_t)offset * 0xC2B2AE3D27D4EB4Full;
    h ^= (uint64_t)(uint32_t)parent_token_id * 0x165667B19E3779F9ull;
    h ^= h >> 29;
    return (size_t)(h & (TEXTPARSER_PARSE_MEMO_BUCKETS - 1));
}

static const textparser_parse_memo *textparser_parse_memo_lookup(
    const struct textparser_handle *handle, int token_id, size_t offset, int parent_token_id)
{
    if (handle == nullptr || handle->parse_memo == nullptr) return nullptr;
    size_t b = textparser_parse_memo_hash(token_id, offset, parent_token_id);
    for (const textparser_parse_memo *e = handle->parse_memo[b]; e != nullptr; e = e->next) {
        if (e->token_id == token_id && e->offset == offset && e->parent_token_id == parent_token_id) {
            return e;
        }
    }
    return nullptr;
}

static void textparser_parse_memo_store(
    struct textparser_handle *handle, int token_id, size_t offset, int parent_token_id,
    const char *error, size_t error_offset, size_t error_length)
{
    if (handle == nullptr || error == nullptr) return;
    if (handle->parse_memo == nullptr) {
        handle->parse_memo = calloc(TEXTPARSER_PARSE_MEMO_BUCKETS, sizeof(*handle->parse_memo));
        if (handle->parse_memo == nullptr) return;
        handle->parse_memo_buckets = TEXTPARSER_PARSE_MEMO_BUCKETS;
    }
    if (handle->parse_memo_count >= TEXTPARSER_PARSE_MEMO_MAX_ENTRIES) return;
    size_t b = textparser_parse_memo_hash(token_id, offset, parent_token_id);
    textparser_parse_memo *e = malloc(sizeof(*e));
    if (e == nullptr) return;
    e->token_id = token_id;
    e->offset = offset;
    e->parent_token_id = parent_token_id;
    e->error = error;
    e->error_offset = error_offset;
    e->error_length = error_length;
    e->next = handle->parse_memo[b];
    handle->parse_memo[b] = e;
    handle->parse_memo_count++;
}

/* A token whose match is decided by the previous sibling (regex-vs-division)
 * depends on more context than the memo key captures, so it is never cached. */
static bool textparser_token_is_memoizable(const textparser_language_definition *definition, int token_id)
{
    if (definition == nullptr || definition->regex_disambiguation == nullptr) return true;
    const textparser_regex_disambiguation *rd = definition->regex_disambiguation;
    return !textparser_token_in_id_list(rd->regex_tokens, token_id);
}

static void textparser_parse_memo_reset(struct textparser_handle *handle)
{
    if (handle == nullptr || handle->parse_memo == nullptr) return;
    for (size_t b = 0; b < handle->parse_memo_buckets; b++) {
        textparser_parse_memo *e = handle->parse_memo[b];
        while (e != nullptr) {
            textparser_parse_memo *next = e->next;
            free(e);
            e = next;
        }
        handle->parse_memo[b] = nullptr;
    }
    handle->parse_memo_count = 0;
}

static void textparser_parse_memo_clear(struct textparser_handle *handle)
{
    if (handle == nullptr || handle->parse_memo == nullptr) return;
    textparser_parse_memo_reset(handle);
    free(handle->parse_memo);
    handle->parse_memo = nullptr;
    handle->parse_memo_buckets = 0;
}

/**
 * Main token parsing dispatcher routing to specific handlers based on token definition type.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Token ID to parse.
 * @param parent_token_id Token ID of enclosing parent container.
 * @param parent_start_stop Delimiter search mode flag.
 * @param offset Output pointer receiving matched offset relative to pos.
 * @param parent_item Enclosing parent container node.
 * @param prev_sibling Preceding sibling node.
 * @return Constructed token item node on success, or NULL on mismatch/error.
 */
static textparser_token_item *textparser_parse_token(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    if (handle == nullptr) {
        LOGF("handle == nullptr");
        return nullptr;
    }

    textparser_token_item *ret = nullptr;
    bool memoizable = false;

    if (handle->recursion_depth >= MAX_RECURSION_DEPTH) {
        exit_with_error(handle, "Maximum recursion depth exceeded!", offset, 0);
    }
    handle->recursion_depth++;

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];

    // Packrat: a sub-parse known to fail in this (token, offset, parent) context
    // is skipped. This bounds the exponential candidate retry loop.
    memoizable = textparser_token_is_memoizable(definition, token_id);
    if (memoizable) {
        const textparser_parse_memo *memo = textparser_parse_memo_lookup(handle, token_id, offset, parent_token_id);
        if (memo != nullptr) {
            handle->error = memo->error;
            handle->error_offset = memo->error_offset;
            handle->error_length = memo->error_length;
            handle->recursion_depth--;
            return nullptr;
        }
    }

    // Bound speculative backtracking: nested ambiguous containers can make the
    // candidate retry loop exponential. A valid parse is O(text), so a generous
    // multiple of the input size terminates pathological inputs with a clear
    // error instead of hanging.
    if (handle->parse_step_limit != 0 && ++handle->parse_steps > handle->parse_step_limit) {
        handle->parse_budget_exceeded = true;
        exit_with_error(handle, "Parse complexity limit exceeded!", offset, 0);
    }

    LOGV("id: %d - [%s]  at offset: %zu", token_id, token_def->name, offset);
    switch(token_def->type)
    {
        case TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY:             ret = parse_token_group_one_child_only(handle, token_id, parent_token_id, parent_start_stop, offset, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_GROUP:                            ret = parse_token_group(handle, token_id, parent_token_id, parent_start_stop, offset, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER: ret = parse_token_group_all_children_in_same_order(handle, token_id, parent_token_id, parent_start_stop, offset, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_SEQUENCE:                         ret = parse_token_sequence(handle, token_id, parent_token_id, parent_start_stop, offset, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN:                     ret = parse_token_simple_token(handle, token_id, parent_token_id, parent_start_stop, offset, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_START_STOP:                       ret = parse_token_start_stop(handle, token_id, parent_token_id, parent_start_stop, offset, true, parent_item, prev_sibling); break;
        case TEXTPARSER_TOKEN_TYPE_START_OPT_STOP:                   ret = parse_token_start_stop(handle, token_id, parent_token_id, parent_start_stop, offset, false, parent_item, prev_sibling); break;
        default:
            parse_token_error_error(handle, "Unknown token type!", offset);
            break;
    }

    if (ret) {
        ret->text_color = token_def->text_color;
        ret->text_background = token_def->text_background;
        ret->text_flags = token_def->text_flags;

        if (!token_def->multi_line && textparser_has_newline(handle, offset, ret->len)) {
            exit_with_error(handle, "Token spans multiple lines but multi_line flag is not set!", offset, ret->len);
        }

        if (token_def->must_have_one_child && textparser_get_semantic_children_count(ret) != 1) {
            exit_with_error(handle, "Token must have exactly one child token!", offset, ret->len);
        }

        if ((token_def->type == TEXTPARSER_TOKEN_TYPE_GROUP || token_def->type == TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY) &&
            token_def->delete_if_only_one_child && textparser_get_semantic_children_count(ret) == 1) {
            textparser_token_item *first_child = ret->child;
            textparser_token_item *last_child = ret->child;
            while (last_child->next) {
                last_child->parent = ret->parent;
                last_child = last_child->next;
            }
            last_child->parent = ret->parent;
            ret = first_child;
        }
    }

exit:
    handle->recursion_depth--;
    if (handle->error != nullptr && memoizable) {
        textparser_parse_memo_store(handle, token_id, offset, parent_token_id,
                                    handle->error, handle->error_offset, handle->error_length);
    }
    return ret;
}

/**
 * Compile and initialize all regular expressions defined in the active language definition.
 *
 * @param handle Pointer to the textparser handle.
 * @return 0 on success, or non-zero error code if regex compilation failed.
 */
static int textparser_init_regex(struct textparser_handle *handle)
{
    if (handle == nullptr)
        return -1;

    if (handle->regex_ctx == nullptr) {
        handle->regex_ctx = adv_regex_context_create();
    }
    int token_cnt = 0;

    while(handle->language->tokens[token_cnt].name != nullptr)
        token_cnt++;

    handle->token_count = (size_t)token_cnt;

    if (token_cnt > 0)
    {
        size_t malloc_size = (size_t)token_cnt * sizeof(void *);

        handle->start_regex = malloc(malloc_size);
        if (handle->start_regex == nullptr) {
            LOGE("malloc() failed for start_regex");
            handle->error = "Can't allocate memory!";
            return -1;
        }
        memset(handle->start_regex, 0, malloc_size);

        handle->end_regex = malloc(malloc_size);
        if (handle->end_regex == nullptr) {
            LOGE("malloc() failed for end_regex");
            free(handle->start_regex);
            handle->start_regex = nullptr;
            handle->error = "Can't allocate memory!";
            return -1;
        }
        memset(handle->end_regex, 0, malloc_size);
    }
    return 0;
}

/**
 * Release all compiled regular expression handles stored in the language definition.
 *
 * @param handle Pointer to the textparser handle.
 */
static void textparser_free_regex(struct textparser_handle *handle)
{
    if (handle == nullptr)
        return;

    enum textparser_encoding text_format = handle->text_format;
    size_t token_cnt = handle->token_count;

    if (handle->start_regex)
    {
        if (token_cnt > 0)
        {
            void **regex = (void **)handle->start_regex;

            for(size_t c = 0; c < token_cnt; c++)
            {
                adv_regex_free(handle->regex_ctx, &regex[c], text_format);
            }
        }
        free(handle->start_regex);
        handle->start_regex = nullptr;
    }

    if (handle->end_regex)
    {
        if (token_cnt > 0)
        {
            void **regex = (void **)handle->end_regex;

            for(size_t c = 0; c < token_cnt; c++)
            {
                adv_regex_free(handle->regex_ctx, &regex[c], text_format);
            }
        }
        free(handle->end_regex);
        handle->end_regex = nullptr;
    }

    if (handle->regex_ctx) {
        adv_regex_context_free(handle->regex_ctx);
        handle->regex_ctx = nullptr;
    }
}

void textparser_free_language_definition(textparser_language_definition *definition)
{
    if (definition == nullptr)
        return;

    bool uses_pool = (definition->string_pool != nullptr);

    if (definition->lexer_modes) {
        for (size_t i = 0; i < definition->lexer_mode_count; i++) {
            free(definition->lexer_modes[i].tokens);
            free(definition->lexer_modes[i].trivia);
        }
        free(definition->lexer_modes);
    }
    if (definition->lexer_goals) {
        for (size_t i = 0; i < definition->lexer_goal_count; i++) free(definition->lexer_goals[i].mappings);
        free(definition->lexer_goals);
    }
    free(definition->lexer_rules);
    free(definition->operator_definitions);
    free(definition->recovery_sync_tokens);

    if (definition->default_file_extensions) {
        if (!uses_pool) {
            int c = 0;
            while(definition->default_file_extensions[c]) {
                free((void *)definition->default_file_extensions[c]);
                c++;
            }
        }
        free((void *)definition->default_file_extensions);
    }

    if (!uses_pool) {
        if (definition->name) {
            free((void *)definition->name);
        }

        if (definition->empty_segment_language) {
            free((void *)definition->empty_segment_language);
        }
    }

    if (definition->starts_with) {
        free((void *)definition->starts_with);
    }

    if (definition->grammar) {
        if (definition->grammar->productions) {
            for (size_t i = 0; i < definition->grammar->production_count; i++) {
                free((void *)definition->grammar->productions[i].children);
                free((void *)definition->grammar->productions[i].recovery_sync_tokens);
                const textparser_guard *guard = definition->grammar->productions[i].guard;
                if (guard != nullptr) {
                    free((void *)guard->next_tokens);
                    free(guard->file_suffixes);
                    free((void *)guard);
                }
            }
            free(definition->grammar->productions);
        }
        free(definition->grammar);
    }

    if (definition->sign_merge) {
        if (definition->sign_merge->sign_tokens) {
            free((void *)definition->sign_merge->sign_tokens);
        }
        if (definition->sign_merge->number_tokens) {
            free((void *)definition->sign_merge->number_tokens);
        }
        if (definition->sign_merge->operand_tokens) {
            free((void *)definition->sign_merge->operand_tokens);
        }
        free((void *)definition->sign_merge);
    }

    if (definition->operator_precedence) {
        if (definition->operator_precedence->rules) {
            for (size_t r = 0; r < definition->operator_precedence->count; r++) {
                if (definition->operator_precedence->rules[r].operators) {
                    free((void *)definition->operator_precedence->rules[r].operators);
                }
            }
            free((void *)definition->operator_precedence->rules);
        }
        free(definition->operator_precedence);
    }

    if (definition->regex_disambiguation) {
        if (definition->regex_disambiguation->regex_tokens) free((void *)definition->regex_disambiguation->regex_tokens);
        if (definition->regex_disambiguation->division_tokens) free((void *)definition->regex_disambiguation->division_tokens);
        if (definition->regex_disambiguation->operand_tokens) free((void *)definition->regex_disambiguation->operand_tokens);
        if (definition->regex_disambiguation->control_keywords) {
            if (!uses_pool) {
                for (int k = 0; definition->regex_disambiguation->control_keywords[k] != nullptr; k++) {
                    free((void *)definition->regex_disambiguation->control_keywords[k]);
                }
            }
            free((void *)definition->regex_disambiguation->control_keywords);
        }
        free(definition->regex_disambiguation);
    }

    if (definition->template_disambiguation) {
        if (definition->template_disambiguation->template_open_tokens) free((void *)definition->template_disambiguation->template_open_tokens);
        if (definition->template_disambiguation->template_close_tokens) free((void *)definition->template_disambiguation->template_close_tokens);
        if (definition->template_disambiguation->valid_inner_tokens) free((void *)definition->template_disambiguation->valid_inner_tokens);
        if (definition->template_disambiguation->invalid_inner_operators) {
            if (!uses_pool) {
                for (int k = 0; definition->template_disambiguation->invalid_inner_operators[k] != nullptr; k++) {
                    free((void *)definition->template_disambiguation->invalid_inner_operators[k]);
                }
            }
            free((void *)definition->template_disambiguation->invalid_inner_operators);
        }
        free(definition->template_disambiguation);
    }

    if (definition->cast_disambiguation) {
        if (definition->cast_disambiguation->type_tokens) free((void *)definition->cast_disambiguation->type_tokens);
        if (definition->cast_disambiguation->type_keywords) {
            if (!uses_pool) {
                for (int k = 0; definition->cast_disambiguation->type_keywords[k] != nullptr; k++) {
                    free((void *)definition->cast_disambiguation->type_keywords[k]);
                }
            }
            free((void *)definition->cast_disambiguation->type_keywords);
        }
        if (definition->cast_disambiguation->type_suffixes) {
            if (!uses_pool) {
                for (int k = 0; definition->cast_disambiguation->type_suffixes[k] != nullptr; k++) {
                    free((void *)definition->cast_disambiguation->type_suffixes[k]);
                }
            }
            free((void *)definition->cast_disambiguation->type_suffixes);
        }
        free(definition->cast_disambiguation);
    }

    if (definition->declaration_disambiguation) {
        if (definition->declaration_disambiguation->return_type_tokens) free((void *)definition->declaration_disambiguation->return_type_tokens);
        if (definition->declaration_disambiguation->declarator_tokens) free((void *)definition->declaration_disambiguation->declarator_tokens);
        free(definition->declaration_disambiguation);
    }

    if (definition->override_start_tokens) {
        for (int r = 0; definition->override_start_tokens[r].file_extensions != nullptr ||
                        definition->override_start_tokens[r].regex != nullptr ||
                        definition->override_start_tokens[r].start_tokens != nullptr; r++) {
            if (definition->override_start_tokens[r].file_extensions) {
                if (!uses_pool) {
                    for (int e = 0; definition->override_start_tokens[r].file_extensions[e] != nullptr; e++) {
                        free((void *)definition->override_start_tokens[r].file_extensions[e]);
                    }
                }
                free((void *)definition->override_start_tokens[r].file_extensions);
            }
            if (!uses_pool && definition->override_start_tokens[r].regex) {
                free((void *)definition->override_start_tokens[r].regex);
            }
            if (definition->override_start_tokens[r].start_tokens) {
                free((void *)definition->override_start_tokens[r].start_tokens);
            }
        }
        free((void *)definition->override_start_tokens);
    }

    if (definition->tokens) {
        int c = 0;
        while(definition->tokens[c].name != nullptr) {
            textparser_token *token = &definition->tokens[c];

            if (!uses_pool) {
                if (token->name) {
                    free((void *)token->name);
                }
                if (token->start_regex) {
                    free((void *)token->start_regex);
                }
                if (token->end_regex) {
                    free((void *)token->end_regex);
                }
            }
            if (token->nested_tokens) {
                free((void *)token->nested_tokens);
            }
            if (token->context_nested_tokens) {
                for (int r = 0; token->context_nested_tokens[r].when_parent_in != nullptr; r++) {
                    if (token->context_nested_tokens[r].when_parent_in) {
                        free((void *)token->context_nested_tokens[r].when_parent_in);
                    }
                    if (token->context_nested_tokens[r].nested_tokens) {
                        free((void *)token->context_nested_tokens[r].nested_tokens);
                    }
                }
                free((void *)token->context_nested_tokens);
            }

            c++;
        }
        free(definition->tokens);
    }

    if (uses_pool) {
        /* Free the continuous string pool arena */
        textparser_string_pool_free((textparser_string_pool *)definition->string_pool);
    }

    free(definition);
}

int textparser_openfile(const char *pathname, int default_text_format, int bom_mask, textparser_t *handle)
{
    if (handle == nullptr || pathname == nullptr) {
        return TEXTPARSER_ERROR_INVALID_ARGUMENT;
    }

    struct textparser_handle local_hnd;
    int err = TEXTPARSER_OK;

    memset(&local_hnd, 0, sizeof(local_hnd));
    if (pathname) {
        local_hnd.filename = strdup(pathname);
        if (local_hnd.filename == nullptr) {
            err = TEXTPARSER_ERROR_OUT_OF_MEMORY;
            goto err;
        }
    }

    local_hnd.mmap_addr = os_map(pathname, &local_hnd.mmap_size);
    if (!local_hnd.mmap_addr && local_hnd.mmap_size != 0) {
        err = TEXTPARSER_ERROR_FILE_OPEN;
        goto err;
    }

    local_hnd.text_addr = local_hnd.mmap_addr;
    local_hnd.text_size = local_hnd.mmap_size;

    if (local_hnd.text_size >= MAX_PARSE_SIZE) {
        err = TEXTPARSER_ERROR_FILE_TOO_LARGE;
        goto err;
    }

    if ((bom_mask & TEXTPARSER_BOM_UTF_32_BE)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x00')&&(local_hnd.text_addr[1] == '\x00')&&(local_hnd.text_addr[2] == '\xfe')&&(local_hnd.text_addr[3] == '\xff')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = TEXTPARSER_BOM_UTF_32_BE;
    } else if ((bom_mask & TEXTPARSER_BOM_UTF_32_LE)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\xff')&&(local_hnd.text_addr[1] == '\xfe')&&(local_hnd.text_addr[2] == '\x00')&&(local_hnd.text_addr[3] == '\x00')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = TEXTPARSER_BOM_UTF_32_LE;
    } else if ((bom_mask & TEXTPARSER_BOM_UTF_8)&&(local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\xef')&&(local_hnd.text_addr[1] == '\xbb')&&(local_hnd.text_addr[2] == '\xbf')) {
        local_hnd.text_addr += 3;
        local_hnd.text_size -= 3;
        local_hnd.bom = TEXTPARSER_BOM_UTF_8;
    } else if ((bom_mask & TEXTPARSER_BOM_UTF_16_BE)&&(local_hnd.text_size >= 2)&&(local_hnd.text_addr[0] == '\xfe')&&(local_hnd.text_addr[1] == '\xff')) {
        local_hnd.text_addr += 2;
        local_hnd.text_size -= 2;
        local_hnd.bom = TEXTPARSER_BOM_UTF_16_BE;
    } else if ((bom_mask & TEXTPARSER_BOM_UTF_16_LE)&&(local_hnd.text_size >= 2)&&(local_hnd.text_addr[0] == '\xff')&&(local_hnd.text_addr[1] == '\xfe')) {
        local_hnd.text_addr += 2;
        local_hnd.text_size -= 2;
        local_hnd.bom = TEXTPARSER_BOM_UTF_16_LE;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_7_1)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x38')) {
    //     local_hnd.text_addr += 4;
    //     local_hnd.text_size -= 4;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_7_1;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_7_2)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x39')) {
    //     local_hnd.text_addr += 4;
    //     local_hnd.text_size -= 4;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_7_2;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_7_3)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x2b')) {
    //     local_hnd.text_addr += 4;
    //     local_hnd.text_size -= 4;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_7_3;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_7_4)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x2f')) {
    //     local_hnd.text_addr += 4;
    //     local_hnd.text_size -= 4;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_7_4;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_7_5)&&(local_hnd.text_size >= 5)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x38')&&(local_hnd.text_addr[4] == '\x2d')) {
    //     local_hnd.text_addr += 5;
    //     local_hnd.text_size -= 5;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_7_5;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_1)&&(local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\xf7')&&(local_hnd.text_addr[1] == '\x64')&&(local_hnd.text_addr[2] == '\x4c')) {
    //     local_hnd.text_addr += 3;
    //     local_hnd.text_size -= 3;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_1;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_EBCDIC)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\xdd')&&(local_hnd.text_addr[1] == '\x73')&&(local_hnd.text_addr[2] == '\x66')&&(local_hnd.text_addr[3] == '\x73')) {
    //     local_hnd.text_addr += 4;
    //     local_hnd.text_size -= 4;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_EBCDIC;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_SCSU)&&(local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\x0e')&&(local_hnd.text_addr[1] == '\xfe')&&(local_hnd.text_addr[2] == '\xff')) {
    //     local_hnd.text_addr += 3;
    //     local_hnd.text_size -= 3;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_SCSU;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_BOCU1)&&(local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\xfb')&&(local_hnd.text_addr[1] == '\xee')&&(local_hnd.text_addr[2] == '\x28')) {
    //     local_hnd.text_addr += 3;
    //     local_hnd.text_size -= 3;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_BOCU1;
    // } else if ((bom_mask & TEXTPARSER_BOM_UTF_GB_18030)&&(local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x84')&&(local_hnd.text_addr[1] == '\x31')&&(local_hnd.text_addr[2] == '\x95')&&(local_hnd.text_addr[3] == '\x33')) {
    //     local_hnd.text_addr += 4;
    //     local_hnd.text_size -= 4;
    //     local_hnd.bom = TEXTPARSER_BOM_UTF_GB_18030;
    } else {
        local_hnd.bom = TEXTPARSER_BOM_NONE;
    }

    local_hnd.text_format = (enum textparser_encoding)default_text_format;

    switch(local_hnd.bom)
    {
        case TEXTPARSER_BOM_NONE:
            break;
        case TEXTPARSER_BOM_UTF_8:
            local_hnd.text_format = TEXTPARSER_ENCODING_UTF_8;
            break;
        case TEXTPARSER_BOM_UTF_16_LE:
            local_hnd.text_format = TEXTPARSER_ENCODING_UTF_16;
            break;
        case TEXTPARSER_BOM_UTF_16_BE:
            local_hnd.text_format = TEXTPARSER_ENCODING_UTF_16;
            break;
        case TEXTPARSER_BOM_UTF_32_LE:
            local_hnd.text_format = TEXTPARSER_ENCODING_UTF_32;
            break;
        default:
            err = TEXTPARSER_ERROR_UNSUPPORTED_BOM;
            goto err;
    }

    local_hnd.no_lines = 0;
    local_hnd.lines = nullptr;
    local_hnd.arena.chunk_size = calculate_chunk_size(local_hnd.text_size);
    local_hnd.scratch.chunk_size = calculate_chunk_size(local_hnd.text_size);

    switch(local_hnd.text_format) {
    case TEXTPARSER_ENCODING_LATIN1:
    case TEXTPARSER_ENCODING_UTF_8:
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
    case TEXTPARSER_ENCODING_UTF_32:
        break;
    default:
        err = TEXTPARSER_ERROR_INVALID_ENCODING;
        goto err;
    }

    if (local_hnd.text_format == TEXTPARSER_ENCODING_UTF_16 || local_hnd.text_format == TEXTPARSER_ENCODING_UNICODE) {
        if (local_hnd.text_size % sizeof(uint16_t) != 0) {
            err = TEXTPARSER_ERROR_INVALID_UTF16_SIZE;
            goto err;
        }
    } else if (local_hnd.text_format == TEXTPARSER_ENCODING_UTF_32) {
        if (local_hnd.text_size % sizeof(uint32_t) != 0) {
            err = TEXTPARSER_ERROR_INVALID_UTF32_SIZE;
            goto err;
        }
    }

    if (local_hnd.bom == TEXTPARSER_BOM_UTF_16_BE && local_hnd.text_size > 0) {
        void *swapped = textparser_convert_utf16be_to_native(local_hnd.text_addr, local_hnd.text_size);
        if (swapped == nullptr) {
            err = TEXTPARSER_ERROR_BYTE_ORDER_CONVERSION;
            goto err;
        }
        local_hnd.owned_buffer = swapped;
        local_hnd.text_addr = swapped;
    }

    *handle = malloc(sizeof(struct textparser_handle));
    if (*handle == nullptr) {
        err = TEXTPARSER_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    memcpy(*handle, &local_hnd, sizeof(struct textparser_handle));

    return TEXTPARSER_OK;

err:
    if (local_hnd.mmap_addr) {
        os_unmap(local_hnd.mmap_addr, local_hnd.mmap_size);
    }

    if (local_hnd.owned_buffer) {
        free(local_hnd.owned_buffer);
        local_hnd.owned_buffer = nullptr;
    }

    if (local_hnd.filename) {
        free(local_hnd.filename);
        local_hnd.filename = nullptr;
    }

    return err;
}


int textparser_openmem(const char *text, int len, int text_format, textparser_t *handle)
{
    if (handle == nullptr || text == nullptr) {
        return -1;
    }

    switch (text_format) {
    case TEXTPARSER_ENCODING_LATIN1:
    case TEXTPARSER_ENCODING_UTF_8:
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
    case TEXTPARSER_ENCODING_UTF_32:
        break;
    default:
        return -1;
    }

    if (len < 0) {
        if (text_format == TEXTPARSER_ENCODING_UTF_16 || text_format == TEXTPARSER_ENCODING_UNICODE) {
            const uint16_t *p = (const uint16_t *)text;
            size_t count = 0;
            while (*p++) {
                count++;
            }
            len = (int)(count * sizeof(uint16_t));
        } else if (text_format == TEXTPARSER_ENCODING_UTF_32) {
            const uint32_t *p = (const uint32_t *)text;
            size_t count = 0;
            while (*p++) {
                count++;
            }
            len = (int)(count * sizeof(uint32_t));
        } else {
            len = (int)strlen(text);
        }
    }

    if ((size_t)len >= MAX_PARSE_SIZE) {
        return -1;
    }

    if (text_format == TEXTPARSER_ENCODING_UTF_16 || text_format == TEXTPARSER_ENCODING_UNICODE) {
        if ((size_t)len % sizeof(uint16_t) != 0) {
            return -1;
        }
    } else if (text_format == TEXTPARSER_ENCODING_UTF_32) {
        if ((size_t)len % sizeof(uint32_t) != 0) {
            return -1;
        }
    }

    struct textparser_handle *ret = nullptr;

    ret = malloc(sizeof(struct textparser_handle));
    if (ret == nullptr) {
        return TEXTPARSER_ERROR_OUT_OF_MEMORY;
    }

    memset(ret, 0, sizeof(struct textparser_handle));

    ret->text_format = (enum textparser_encoding)text_format;
    ret->text_addr = text;
    ret->text_size = (size_t)len;
    ret->arena.chunk_size = calculate_chunk_size(ret->text_size);
    ret->scratch.chunk_size = calculate_chunk_size(ret->text_size);
    ret->regex_ctx = adv_regex_context_create();

    *handle = (textparser_t)ret;

    return 0;
}

EXPORT_TEXTPARSER int textparser_set_text(textparser_t handle, const char *text, int len)
{
    if (handle == nullptr || text == nullptr)
        return -1;

    if (len < 0) {
        if (handle->text_format == TEXTPARSER_ENCODING_UTF_16 || handle->text_format == TEXTPARSER_ENCODING_UNICODE) {
            const uint16_t *p = (const uint16_t *)text;
            size_t count = 0;
            while (*p++) {
                count++;
            }
            len = (int)(count * sizeof(uint16_t));
        } else if (handle->text_format == TEXTPARSER_ENCODING_UTF_32) {
            const uint32_t *p = (const uint32_t *)text;
            size_t count = 0;
            while (*p++) {
                count++;
            }
            len = (int)(count * sizeof(uint32_t));
        } else {
            len = (int)strlen(text);
        }
    }

    if ((size_t)len >= MAX_PARSE_SIZE)
        return -1;

    if (handle->text_format == TEXTPARSER_ENCODING_UTF_16 || handle->text_format == TEXTPARSER_ENCODING_UNICODE) {
        if ((size_t)len % sizeof(uint16_t) != 0) {
            return -1;
        }
    } else if (handle->text_format == TEXTPARSER_ENCODING_UTF_32) {
        if ((size_t)len % sizeof(uint32_t) != 0) {
            return -1;
        }
    }

    if (handle->lines) {
        free(handle->lines);
        handle->lines = nullptr;
        handle->no_lines = 0;
    }

    textparser_clear_lexer_streams(handle);

    if (handle->owned_buffer && handle->owned_buffer != text) {
        free(handle->owned_buffer);
        handle->owned_buffer = nullptr;
        handle->owned_buffer_capacity = 0;
    }

    handle->text_addr = text;
    handle->text_size = (size_t)len;
    return 0;
}

/**
 * Recursively free token items removed during AST post-processing unwrapping.
 *
 * @param node Pointer to CST node being evaluated or manipulated.
 */
static void free_post_processed_tokens(textparser_token_item *node)
{
    if (node == nullptr) return;
    textparser_token_item *c = node->child;
    while (c != nullptr) {
        textparser_token_item *next_sibling = c->next;
        free_post_processed_tokens(c);
        c = next_sibling;
    }
    if (node->text_flags & 0x80000000) {
        free(node);
    }
}

/**
 * Recursively flatten synthesized post-processing nodes back into plain CST
 * sibling lists, freeing the synthesized wrappers.
 *
 * Also reports whether the subtree was in AST mode, detected either through the
 * POST_PROCESSED marker or a synthesized (0x80000000) node. This lets the
 * incremental engine decide whether to re-derive post-processing in a single
 * traversal.
 *
 * @param root In/out pointer to head of node or token list.
 * @return true if the subtree was post-processed; false otherwise.
 */
static bool unwrap_post_processed_tokens(textparser_token_item **root)
{
    if (root == nullptr || *root == nullptr) return false;
    bool post_processed = false;
    textparser_token_item *curr = *root;
    while (curr != nullptr) {
        textparser_token_item *next_sibling = curr->next;
        if (curr->node_flags & TEXTPARSER_NODE_POST_PROCESSED) post_processed = true;
        if (curr->child != nullptr) {
            if (unwrap_post_processed_tokens(&curr->child)) post_processed = true;
        }
        if (curr->text_flags & 0x80000000) {
            post_processed = true;
            textparser_token_item *first_child = curr->child;
            if (first_child != nullptr) {
                textparser_token_item *last_child = first_child;
                while (last_child->next != nullptr) {
                    last_child->parent = curr->parent;
                    last_child = last_child->next;
                }
                last_child->parent = curr->parent;
                first_child->prev = curr->prev;
                last_child->next = curr->next;

                if (curr->prev != nullptr) {
                    curr->prev->next = first_child;
                } else if (curr->parent != nullptr) {
                    curr->parent->child = first_child;
                } else if (root != nullptr) {
                    *root = first_child;
                }

                if (curr->next != nullptr) {
                    curr->next->prev = last_child;
                }
            } else {
                if (curr->prev != nullptr) {
                    curr->prev->next = curr->next;
                } else if (curr->parent != nullptr) {
                    curr->parent->child = curr->next;
                } else if (root != nullptr) {
                    *root = curr->next;
                }
                if (curr->next != nullptr) {
                    curr->next->prev = curr->prev;
                }
            }
            free(curr);
        }
        curr = next_sibling;
    }
    return post_processed;
}

void textparser_close(textparser_t handle)
{
    void *mmap_addr = nullptr;
    size_t mmap_size = 0;

    if (handle == nullptr)
        return;

    textparser_clear_lexer_streams(handle);

    textparser_free_regex(handle);

    mmap_addr = handle->mmap_addr;
    mmap_size = handle->mmap_size;

    if (mmap_addr) {
        os_unmap(mmap_addr, mmap_size);
    }

    if (handle->owned_buffer) {
        free(handle->owned_buffer);
        handle->owned_buffer = nullptr;
    }

    if (handle->first_item) {
        textparser_token_item *it = handle->first_item;
        while (it != nullptr) {
            textparser_token_item *next_item = it->next;
            free_post_processed_tokens(it);
            it = next_item;
        }
    }

    /* Clean up any user_data attached to nodes */
    if (handle->first_item) {
        /* Recursively free user_data if free_user_data callback is provided */
        // Handled below if user_data attachments were created
    }

    free_arena(handle);
    arena_free(&handle->scratch);
    textparser_parse_memo_clear(handle);

    if (handle->lines) {
        free(handle->lines);
        handle->lines = nullptr;
    }

    if (handle->filename) {
        free(handle->filename);
        handle->filename = nullptr;
    }

    /* Free registered semantic handlers */
    textparser_handler_entry *entry = handle->handlers;
    while (entry != nullptr) {
        textparser_handler_entry *next_entry = entry->next;
        if (entry->name) free(entry->name);
        free(entry);
        entry = next_entry;
    }
    handle->handlers = nullptr;
    free(handle->pending_events);
    handle->pending_events = nullptr;
    handle->pending_event_capacity = 0;

    /* Free registered decoders */
    textparser_decoder_entry *dec = handle->decoders;
    while (dec != nullptr) {
        textparser_decoder_entry *next_dec = dec->next;
        if (dec->name) free(dec->name);
        free(dec);
        dec = next_dec;
    }
    handle->decoders = nullptr;

    /* Free registered validators */
    textparser_validator_entry *val = handle->validators;
    while (val != nullptr) {
        textparser_validator_entry *next_val = val->next;
        if (val->name) free(val->name);
        free(val);
        val = next_val;
    }
    handle->validators = nullptr;

    /* Free mode stack strings */
    for (size_t m = 0; m < handle->mode_stack_depth; m++) {
        if (handle->mode_stack[m]) {
            free(handle->mode_stack[m]);
            handle->mode_stack[m] = nullptr;
        }
    }
    handle->mode_stack_depth = 0;

    /* Free the active lexical goal */
    if (handle->lexical_goal) {
        free(handle->lexical_goal);
        handle->lexical_goal = nullptr;
    }

    /* Free registered predicates */
    textparser_predicate_entry *pred = handle->predicates;
    while (pred != nullptr) {
        textparser_predicate_entry *next_pred = pred->next;
        if (pred->name) free(pred->name);
        free(pred);
        pred = next_pred;
    }
    handle->predicates = nullptr;

    /* Free scoped context entries */
    textparser_context_entry *ctx = handle->contexts;
    while (ctx != nullptr) {
        textparser_context_entry *next_ctx = ctx->next;
        if (ctx->name) free(ctx->name);
        free(ctx);
        ctx = next_ctx;
    }
    handle->contexts = nullptr;

    /* Free registered operator definitions */
    if (handle->operators) {
        free(handle->operators);
        handle->operators = nullptr;
    }
    handle->operator_count = 0;
    handle->operator_capacity = 0;

    /* Free registered operand validators */
    textparser_operand_validator_entry *op_val = handle->operand_validators;
    while (op_val != nullptr) {
        textparser_operand_validator_entry *next_op_val = op_val->next;
        if (op_val->name) free(op_val->name);
        free(op_val);
        op_val = next_op_val;
    }
    handle->operand_validators = nullptr;

    /* Free diagnostic vector */
    if (handle->diagnostics) {
        for (size_t d = 0; d < handle->diagnostic_count; d++) {
            if (handle->diagnostics[d].code) free((void *)handle->diagnostics[d].code);
            if (handle->diagnostics[d].message) free((void *)handle->diagnostics[d].message);
        }
        free(handle->diagnostics);
        handle->diagnostics = nullptr;
    }
    handle->diagnostic_count = 0;
    handle->diagnostic_capacity = 0;

    /* Free memo table */
    textparser_memo_clear(handle);

    free(handle);
}

void textparser_cleanup(textparser_t *handle)
{
    if (handle)
    {
        textparser_close(*handle);
        *handle = nullptr;
    }
}

void textparser_set_filename(textparser_t handle, const char *filename)
{
    if (handle == nullptr)
        return;

    if (handle->filename) {
        free(handle->filename);
        handle->filename = nullptr;
    }

    if (filename) {
        handle->filename = strdup(filename);
        if (handle->filename == nullptr) {
            return;
        }
    }
}

const char *textparser_get_filename(const textparser_t handle)
{
    if (handle == nullptr)
        return nullptr;

    return handle->filename;
}

/**
 * Find the innermost AST node enclosing a specific character unit offset.
 *
 * @param token Pointer to token item node or lexer token snapshot.
 * @param position Character unit offset in text buffer.
 * @param depth Current recursion or speculation nesting depth.
 * @return Pointer to the enclosing token item node, or NULL if not found.
 */
static const textparser_token_item *find_token_at_position_internal(const textparser_token_item *token, size_t position, int depth)
{
    if (depth >= MAX_RECURSION_DEPTH || token == nullptr) {
        return nullptr;
    }
    const textparser_token_item *best = nullptr;
    size_t curr_pos = (token->parent != nullptr) ? textparser_get_token_position(token) : 0;
    while (token != nullptr) {
        if (curr_pos > position) {
            break;
        }
        if (position >= curr_pos && position < curr_pos + token->len) {
            best = token;
            if (token->child != nullptr) {
                // Pass curr_pos directly to child search avoiding re-walking up the tree
                const textparser_token_item *child = token->child;
                size_t child_pos = curr_pos;
                while (child != nullptr) {
                    if (child_pos > position) break;
                    if (position >= child_pos && position < child_pos + child->len) {
                        const textparser_token_item *deeper = find_token_at_position_internal(child, position, depth + 1);
                        if (deeper) best = deeper;
                        else best = child;
                        break;
                    }
                    child_pos += child->len;
                    child = child->next;
                }
            }
            break;
        }
        curr_pos += token->len;
        token = token->next;
    }
    return best;
}

static int textparser_parse_contextual(struct textparser_handle *handle,
                                       const textparser_language_definition *definition);

int textparser_parse(textparser_t handle, const textparser_language_definition *definition)
{
    if (handle == nullptr || definition == nullptr)
        return -1;
    return textparser_parse_incremental(handle, definition, 0, textparser_get_total_units(handle), handle->text_addr, textparser_get_total_units(handle), nullptr);
}

typedef struct {
    textparser_lex_token *tokens;
    size_t token_count;
    size_t token_capacity;
    textparser_lex_trivia *trivia;
    size_t trivia_count;
    size_t trivia_capacity;
    size_t pending_trivia_start;
    /* Active lexer mode reconstructed while walking the CST. Entries are
     * borrowed pointers into the language definition (never freed here). */
    const char *mode_stack[TEXTPARSER_MAX_MODE_STACK];
    size_t mode_depth;
} textparser_lexer_stream_builder;

/**
 * Ensure dynamic array capacity in the lexer snapshot token stream builder.
 *
 * @param builder Pointer to lexer stream builder structure.
 * @return true if capacity is available or reallocated successfully; false on OOM.
 */
static bool textparser_lexer_builder_reserve_tokens(textparser_lexer_stream_builder *builder)
{
    if (builder->token_count < builder->token_capacity) return true;
    size_t capacity = builder->token_capacity == 0 ? 32 : builder->token_capacity * 2;
    textparser_lex_token *items = realloc(builder->tokens, capacity * sizeof(*items));
    if (items == nullptr) return false;
    builder->tokens = items;
    builder->token_capacity = capacity;
    return true;
}

/**
 * Ensure dynamic array capacity in the lexer snapshot trivia stream builder.
 *
 * @param builder Pointer to lexer stream builder structure.
 * @return true if capacity is available or reallocated successfully; false on OOM.
 */
static bool textparser_lexer_builder_reserve_trivia(textparser_lexer_stream_builder *builder)
{
    if (builder->trivia_count < builder->trivia_capacity) return true;
    size_t capacity = builder->trivia_capacity == 0 ? 32 : builder->trivia_capacity * 2;
    textparser_lex_trivia *items = realloc(builder->trivia, capacity * sizeof(*items));
    if (items == nullptr) return false;
    builder->trivia = items;
    builder->trivia_capacity = capacity;
    return true;
}

/**
 * Inspect a character unit span to detect line terminators and compute lexer trivia flags.
 *
 * @param handle Pointer to the textparser handle.
 * @param start Starting unit offset.
 * @param end Ending unit offset (half-open).
 * @return Bitmask of flags such as TEXTPARSER_LEX_FLAG_CONTAINS_LINE_TERMINATOR.
 */
static uint32_t textparser_lexer_span_flags(
    const struct textparser_handle *handle,
    size_t start,
    size_t end)
{
    for (size_t pos = start; pos < end; pos++) {
        uint32_t ch = textparser_get_unit_at(handle, pos);
        if (ch == '\n' || ch == '\r' || ch == 0x2028 || ch == 0x2029) {
            return TEXTPARSER_LEX_FLAG_CONTAINS_LINE_TERMINATOR;
        }
    }
    return 0;
}

/**
 * Map a lexer mode name to the snapshot token's one-based mode id.
 *
 * @param language Active language definition.
 * @param name Mode name, or NULL for the default mode.
 * @return One-based index into `language->lexer_modes`, or 0 for the default mode.
 */
static int textparser_lexer_mode_id(
    const textparser_language_definition *language,
    const char *name)
{
    if (language == nullptr || language->lexer_modes == nullptr || name == nullptr) return 0;
    for (size_t i = 0; i < language->lexer_mode_count; i++) {
        if (language->lexer_modes[i].name != nullptr &&
            strcmp(language->lexer_modes[i].name, name) == 0) {
            return (int)i + 1;
        }
    }
    return 0;
}

/**
 * Traverse parsed CST nodes to populate contiguous token and trivia snapshot arrays.
 *
 * @param handle Pointer to the textparser handle.
 * @param node Current token item node being traversed.
 * @param node_start Character unit start offset of node.
 * @param builder Pointer to lexer stream builder accumulating entries.
 * @return true on success, or false on memory allocation failure.
 */
static bool textparser_collect_lexer_streams(
    const struct textparser_handle *handle,
    const textparser_token_item *node,
    size_t node_start,
    textparser_lexer_stream_builder *builder)
{
    const textparser_token_item *curr = node;
    size_t pos = node_start;
    while (curr != nullptr) {
        size_t end = pos + curr->len;
        if (curr->child != nullptr) {
            if (!textparser_collect_lexer_streams(handle, curr->child, pos, builder)) return false;
        } else if (curr->token_id == TEXTPARSER_TOKEN_ID_WHITESPACE ||
                   (curr->node_flags & TEXTPARSER_NODE_TRIVIA) != 0) {
            if (!textparser_lexer_builder_reserve_trivia(builder)) return false;
            textparser_lex_trivia *trivia = &builder->trivia[builder->trivia_count++];
            trivia->kind = curr->token_id;
            trivia->start = pos;
            trivia->end = end;
            trivia->flags = textparser_lexer_span_flags(handle, pos, end);
        } else {
            if (!textparser_lexer_builder_reserve_tokens(builder)) return false;
            textparser_lex_token *token = &builder->tokens[builder->token_count++];
            token->kind = curr->token_id;
            token->start = pos;
            token->end = end;
            token->leading_trivia_start = builder->pending_trivia_start;
            token->leading_trivia_count = builder->trivia_count - builder->pending_trivia_start;
            /* The mode recorded on a token is the mode active before its own
             * push/pop transition, matching textparser_contextual_scan_one. */
            const char *active_mode = builder->mode_depth > 0
                ? builder->mode_stack[builder->mode_depth - 1]
                : (handle->language != nullptr && handle->language->initial_lexer_mode != nullptr
                    ? handle->language->initial_lexer_mode : "default");
            token->mode = textparser_lexer_mode_id(handle->language, active_mode);
            token->lexical_goal = 0;
            token->flags = 0;
            for (size_t i = builder->pending_trivia_start; i < builder->trivia_count; i++) {
                token->flags |= builder->trivia[i].flags;
            }
            token->decoded_value = curr->decoded_value;
            builder->pending_trivia_start = builder->trivia_count;

            /* Reconstruct the contextual mode stack from the token's lexer rule,
             * mirroring textparser_lexer_consume: pop first, then push. */
            if (handle->language != nullptr && handle->language->lexer_rules != nullptr &&
                curr->token_id >= 0 && (size_t)curr->token_id < handle->token_count) {
                const textparser_contextual_lexer_rule *rule =
                    &handle->language->lexer_rules[curr->token_id];
                if (rule->pop_mode && builder->mode_depth > 0) builder->mode_depth--;
                if (rule->push_mode != nullptr &&
                    builder->mode_depth < TEXTPARSER_MAX_MODE_STACK) {
                    builder->mode_stack[builder->mode_depth++] = rule->push_mode;
                }
            }
        }
        pos = end;
        curr = curr->next;
    }
    return true;
}

/**
 * Rebuild immutable syntax token and trivia snapshots from CST following a parse pass.
 *
 * The snapshot buffers are retained on the handle across edits: the builder is
 * seeded with the existing arrays and capacities so a rebuild re-fills them in
 * place instead of freeing and reallocating on every edit.
 *
 * @param handle Pointer to the textparser handle.
 * @return TEXTPARSER_OK (0) on success, or error code on failure.
 */
static int textparser_rebuild_lexer_streams(struct textparser_handle *handle)
{
    // Phase 3: ensure any pending position bias is cleared before we overwrite
    // the snapshot arrays. The collector rebuilds from the CST and produces
    // fresh absolute offsets, so the bias is no longer valid after a rebuild.
    // Clearing here (without materializing) is safe because the rebuild
    // overwrites every entry anyway.
    handle->lexer_snapshot_bias = 0;
    handle->lexer_snapshot_token_bias_start  = 0;
    handle->lexer_snapshot_trivia_bias_start = 0;

    textparser_lexer_stream_builder builder = {0};
    builder.tokens = handle->lexer_tokens;
    builder.token_capacity = handle->lexer_token_capacity;
    builder.trivia = handle->lexer_trivia;
    builder.trivia_capacity = handle->lexer_trivia_capacity;

    if (!textparser_collect_lexer_streams(handle, handle->first_item, 0, &builder)) {
        // The collector may have reallocated the seeded buffers, so publish the
        // builder's pointers before releasing them; freeing the stale handle
        // pointers here would be a use-after-free.
        handle->lexer_tokens = builder.tokens;
        handle->lexer_token_capacity = builder.token_capacity;
        handle->lexer_trivia = builder.trivia;
        handle->lexer_trivia_capacity = builder.trivia_capacity;
        textparser_clear_lexer_streams(handle);
        return TEXTPARSER_ERROR_OUT_OF_MEMORY;
    }

    handle->lexer_tokens = builder.tokens;
    handle->lexer_token_count = builder.token_count;
    handle->lexer_token_capacity = builder.token_capacity;
    handle->lexer_trivia = builder.trivia;
    handle->lexer_trivia_count = builder.trivia_count;
    handle->lexer_trivia_capacity = builder.trivia_capacity;
    textparser_clear_lexer_cache(handle);
    handle->parser.source_offset = textparser_get_total_units(handle);
    handle->parser.token_index = builder.token_count;
    return TEXTPARSER_OK;
}

/* Growable array of sibling token pointers backed by the per-call scratch arena. */
typedef struct {
    textparser_token_item **items;
    size_t count;
    size_t capacity;
} textparser_token_run;

static bool textparser_run_push(textparser_token_run *run, textparser_arena *scratch, textparser_token_item *item)
{
    if (run->count == run->capacity)
    {
        size_t new_capacity = run->capacity ? run->capacity * 2 : 16;
        textparser_token_item **new_items = arena_alloc(scratch, new_capacity * sizeof(*new_items));
        if (new_items == nullptr) return false;
        if (run->items != nullptr) {
            memcpy(new_items, run->items, run->count * sizeof(*new_items));
        }
        run->items = new_items;
        run->capacity = new_capacity;
    }
    run->items[run->count++] = item;
    return true;
}

/* Recursively compare token identity by kind and span (not node id). */
static bool textparser_tokens_shape_equal(const textparser_token_item *a, const textparser_token_item *b)
{
    if (a == b) return true;
    if (a == nullptr || b == nullptr) return false;
    if (a->token_id != b->token_id || a->len != b->len) return false;

    const textparser_token_item *ca = a->child;
    const textparser_token_item *cb = b->child;
    while (ca != nullptr && cb != nullptr) {
        if (!textparser_tokens_shape_equal(ca, cb)) return false;
        ca = ca->next;
        cb = cb->next;
    }
    return ca == nullptr && cb == nullptr;
}

/**
 * Reset transient lexical state (mode stack and lexical goal) to the language's
 * initial state.
 *
 * `textparser_parse_incremental` anchors at the root and re-tokenizes from
 * offset 0, so the lexical state at the anchor is always the initial state.
 * Clearing state left over from a previous parse or `textparser_execute_production`
 * call keeps the anchor state-safe and stops a stale mode/goal from leaking into
 * the reparse and the rebuilt lexer snapshot.
 *
 * @param handle Pointer to the textparser handle.
 */
static void textparser_reset_lexical_state(struct textparser_handle *handle)
{
    if (handle == nullptr) return;
    for (size_t m = 0; m < handle->mode_stack_depth; m++) {
        if (handle->mode_stack[m] != nullptr) {
            free(handle->mode_stack[m]);
            handle->mode_stack[m] = nullptr;
        }
    }
    handle->mode_stack_depth = 0;
    if (handle->lexical_goal != nullptr) {
        free(handle->lexical_goal);
        handle->lexical_goal = nullptr;
    }
}

/**
 * Binary-search the token snapshot for the entry beginning at `start`.
 *
 * The snapshot is ordered by start and leaves tile the document, so a match is
 * unique.
 *
 * @param handle Pointer to the textparser handle.
 * @param start Start offset to locate.
 * @return Entry index, or SIZE_MAX when absent.
 */
static size_t textparser_lexer_find_token_at(const struct textparser_handle *handle, size_t start)
{
    size_t lo = 0, hi = handle->lexer_token_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (LEXER_TOKEN_START(handle, mid) < start) lo = mid + 1;
        else hi = mid;
    }
    if (lo < handle->lexer_token_count && LEXER_TOKEN_START(handle, lo) == start) return lo;
    return SIZE_MAX;
}

/**
 * Binary-search the trivia snapshot for the entry beginning at `start`.
 *
 * @param handle Pointer to the textparser handle.
 * @param start Start offset to locate.
 * @return Entry index, or SIZE_MAX when absent.
 */
static size_t textparser_lexer_find_trivia_at(const struct textparser_handle *handle, size_t start)
{
    size_t lo = 0, hi = handle->lexer_trivia_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (LEXER_TRIVIA_START(handle, mid) < start) lo = mid + 1;
        else hi = mid;
    }
    if (lo < handle->lexer_trivia_count && LEXER_TRIVIA_START(handle, lo) == start) return lo;
    return SIZE_MAX;
}

/**
 * Recompute a token's aggregate flags from its leading trivia range.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_index Index of the token whose flags should be refreshed.
 */
static void textparser_lexer_refresh_token_flags(struct textparser_handle *handle, size_t token_index)
{
    textparser_lex_token *tok = &handle->lexer_tokens[token_index];
    uint32_t flags = 0;
    for (size_t i = tok->leading_trivia_start;
         i < tok->leading_trivia_start + tok->leading_trivia_count &&
         i < handle->lexer_trivia_count; i++) {
        flags |= handle->lexer_trivia[i].flags;
    }
    tok->flags = flags;
}

/**
 * Patch the flat lexer snapshot after an in-leaf length change.
 *
 * The in-leaf fast path guarantees the edited leaf still matches as a single
 * token of the same kind and that no post-processing runs, so the snapshot's
 * only difference is positional: the edited leaf's end moves by `delta_units`
 * and every later entry shifts by the same amount. Kinds, trivia structure,
 * leading-trivia indices and modes are untouched.
 *
 * @param handle Pointer to the textparser handle.
 * @param leaf_start Start offset of the edited leaf.
 * @param old_leaf_end End offset of the edited leaf before the edit.
 * @param new_leaf_end End offset of the edited leaf after the edit.
 * @param delta_units Signed length change in encoding units.
 * @param leaf The edited CST leaf node.
 * @return true when the snapshot was patched; false when the leaf could not be
 *         located (the caller must fall back to a full rebuild).
 */
static bool textparser_patch_lexer_streams_leaf(
    struct textparser_handle *handle,
    size_t leaf_start,
    size_t old_leaf_end,
    size_t new_leaf_end,
    ssize_t delta_units,
    const textparser_token_item *leaf)
{
    if (handle == nullptr || leaf == nullptr) return false;

    bool is_trivia = (leaf->token_id == TEXTPARSER_TOKEN_ID_WHITESPACE) ||
                     (leaf->node_flags & TEXTPARSER_NODE_TRIVIA) != 0;

    if (is_trivia) {
        size_t idx = textparser_lexer_find_trivia_at(handle, leaf_start);
        if (idx == SIZE_MAX) return false;

        // If a bias was already active at a different boundary, materialize it first
        if (handle->lexer_snapshot_bias != 0 &&
            (handle->lexer_snapshot_trivia_bias_start != idx + 1)) {
            textparser_snapshot_materialize_bias(handle);
        }

        handle->lexer_trivia[idx].end = new_leaf_end;
        handle->lexer_trivia[idx].flags =
            textparser_lexer_span_flags(handle, leaf_start, new_leaf_end);

        // Find the first token starting at or after old_leaf_end
        size_t tok_start_idx = handle->lexer_token_count;
        for (size_t i = 0; i < handle->lexer_token_count; i++) {
            if (LEXER_TOKEN_START(handle, i) >= old_leaf_end) {
                tok_start_idx = i;
                break;
            }
        }

        if (handle->lexer_snapshot_bias == 0) {
            handle->lexer_snapshot_trivia_bias_start = idx + 1;
            handle->lexer_snapshot_token_bias_start = tok_start_idx;
            handle->lexer_snapshot_bias = delta_units;
        } else {
            handle->lexer_snapshot_bias += delta_units;
        }

        /* The edited trivia belongs to the leading run of the next token; its
         * aggregate flags may have changed (e.g. an inserted line terminator). */
        for (size_t i = 0; i < handle->lexer_token_count; i++) {
            textparser_lex_token *tok = &handle->lexer_tokens[i];
            if (idx >= tok->leading_trivia_start &&
                idx < tok->leading_trivia_start + tok->leading_trivia_count) {
                textparser_lexer_refresh_token_flags(handle, i);
            }
        }
    } else {
        size_t idx = textparser_lexer_find_token_at(handle, leaf_start);
        if (idx == SIZE_MAX) return false;

        // If a bias was already active at a different boundary, materialize it first
        if (handle->lexer_snapshot_bias != 0 &&
            (handle->lexer_snapshot_token_bias_start != idx + 1)) {
            textparser_snapshot_materialize_bias(handle);
        }

        handle->lexer_tokens[idx].end = new_leaf_end;
        handle->lexer_tokens[idx].decoded_value = leaf->decoded_value;

        // Find the first trivia starting at or after old_leaf_end
        size_t trivia_start_idx = handle->lexer_trivia_count;
        for (size_t j = 0; j < handle->lexer_trivia_count; j++) {
            if (LEXER_TRIVIA_START(handle, j) >= old_leaf_end) {
                trivia_start_idx = j;
                break;
            }
        }

        if (handle->lexer_snapshot_bias == 0) {
            handle->lexer_snapshot_token_bias_start = idx + 1;
            handle->lexer_snapshot_trivia_bias_start = trivia_start_idx;
            handle->lexer_snapshot_bias = delta_units;
        } else {
            handle->lexer_snapshot_bias += delta_units;
        }
    }

    handle->parser.source_offset = textparser_get_total_units(handle);
    handle->parser.token_index = handle->lexer_token_count;
    return true;
}

EXPORT_TEXTPARSER int textparser_parse_incremental(textparser_t handle,
                                                   const textparser_language_definition *definition,
                                                   size_t edit_offset,
                                                   size_t old_len,
                                                   const void *new_text,
                                                   size_t new_len,
                                                   textparser_dirty_range *out_range)
{
    if (handle == nullptr || definition == nullptr)
        return -1;

    handle->parser.owner = handle;
    handle->parser.language = definition;
    handle->parser.source_offset = 0;
    handle->parser.token_index = 0;
    handle->parser.pending_event_count = 0;
    handle->parser.speculation_depth = 0;
    handle->parser.recovery_depth = 0;

    // Reset error state
    handle->error = nullptr;
    handle->error_offset = 0;
    handle->error_length = 0;

    // Reset the parse work budget and the packrat failure memo.
    handle->parse_steps = 0;
    handle->parse_step_limit = textparser_get_total_units(handle) * 20 + 10000;
    handle->parse_budget_exceeded = false;
    textparser_parse_memo_reset(handle);

    // Per-call temporaries (dirty runs, alignment) reuse the scratch arena.
    arena_reset(&handle->scratch);

    // The reparse anchors at the root, so it starts in the language's initial
    // lexical state. Drop any mode/goal left over from a prior parse or grammar
    // execution before re-tokenizing.
    textparser_reset_lexical_state(handle);

    size_t unit_size = 1;
    switch (handle->text_format) {
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
        unit_size = sizeof(uint16_t);
        break;
    case TEXTPARSER_ENCODING_UTF_32:
        unit_size = sizeof(uint32_t);
        break;
    default:
        unit_size = 1;
        break;
    }

    size_t old_total_units = textparser_get_total_units(handle);
    if (edit_offset > old_total_units || old_len > old_total_units - edit_offset)
        return -1;

    if (new_len > (MAX_PARSE_SIZE / unit_size))
        return -1;

    size_t byte_offset = edit_offset * unit_size;
    size_t old_byte_len = old_len * unit_size;
    size_t new_byte_len = new_len * unit_size;

    size_t remaining_bytes = handle->text_size - old_byte_len;
    if (new_byte_len >= MAX_PARSE_SIZE || remaining_bytes + new_byte_len >= MAX_PARSE_SIZE)
        return -1;

    size_t new_total_bytes = remaining_bytes + new_byte_len;
    ssize_t delta_units = (ssize_t)new_len - (ssize_t)old_len;
    ssize_t delta_bytes = (ssize_t)new_byte_len - (ssize_t)old_byte_len;

    // Splicing the text buffer if this is an actual edit
    if (new_text != handle->text_addr || delta_bytes != 0) {
        void *temp_new_text = nullptr;
        if (new_text != nullptr && new_byte_len > 0) {
            const char *nt = (const char *)new_text;
            bool is_aliased = false;
            if (handle->text_addr && nt >= handle->text_addr && nt < handle->text_addr + handle->text_size) {
                is_aliased = true;
            } else if (handle->owned_buffer && nt >= (const char *)handle->owned_buffer &&
                       nt < (const char *)handle->owned_buffer + handle->owned_buffer_capacity) {
                is_aliased = true;
            }
            if (is_aliased) {
                temp_new_text = malloc(new_byte_len);
                if (temp_new_text == nullptr)
                    return -1;
                memcpy(temp_new_text, new_text, new_byte_len);
                new_text = temp_new_text;
            }
        }

        if (handle->owned_buffer == nullptr) {
            size_t cap = (new_total_bytes + unit_size + 1024) * 2;
            void *buf = malloc(cap);
            if (buf == nullptr) {
                if (temp_new_text) free(temp_new_text);
                return -1;
            }
            if (handle->text_addr && handle->text_size > 0) {
                memcpy(buf, handle->text_addr, handle->text_size);
            }
            handle->owned_buffer = buf;
            handle->owned_buffer_capacity = cap;
            handle->text_addr = (const char *)buf;
        } else if (new_total_bytes + unit_size > handle->owned_buffer_capacity) {
            size_t cap = (new_total_bytes + unit_size + 1024) * 2;
            void *buf = realloc(handle->owned_buffer, cap);
            if (buf == nullptr) {
                if (temp_new_text) free(temp_new_text);
                return -1;
            }
            handle->owned_buffer = buf;
            handle->owned_buffer_capacity = cap;
            handle->text_addr = (const char *)buf;
        }

        char *buf = (char *)handle->owned_buffer;
        size_t suffix_bytes = handle->text_size - (byte_offset + old_byte_len);
        if (suffix_bytes > 0 && delta_bytes != 0) {
            memmove(buf + byte_offset + new_byte_len, buf + byte_offset + old_byte_len, suffix_bytes);
        }
        if (new_byte_len > 0 && new_text != nullptr) {
            memmove(buf + byte_offset, new_text, new_byte_len);
        }
        memset(buf + new_total_bytes, 0, unit_size);
        handle->text_size = new_total_bytes;
        handle->text_addr = buf;

        if (temp_new_text) {
            free(temp_new_text);
            temp_new_text = nullptr;
        }
    }

    size_t start_pos = edit_offset;
    size_t end_pos = edit_offset + new_len;
    size_t old_end_bound = edit_offset + old_len;
    size_t total = textparser_get_total_units(handle);

    // If doing a full parse from offset 0 to EOF, reset existing arena tree.
    // Flattening the old tree frees any synthesized heap nodes and reports
    // whether the caller had post-processed it, so AST mode can be preserved
    // across the reset instead of silently dropping back to a raw CST.
    bool was_post_processed = false;
    if (start_pos == 0 && end_pos >= total)
    {
        if (handle->first_item != nullptr) {
            was_post_processed = unwrap_post_processed_tokens(&handle->first_item);
            handle->first_item = nullptr;
        }
        free_arena(handle);
    }

    if (handle->language != definition)
    {
        textparser_free_regex(handle);
        handle->language = definition;
        if (textparser_init_regex(handle) != 0)
            return -1;
    }

    // Record text validity on the shared regex context (and the thread-local
    // fast-path flag) so PCRE2 can skip re-validating the subject on every
    // match; without this the full parse is O(n^2) on UTF-8 text.
    adv_regex_set_utf8_valid(handle->regex_ctx, textparser_validate_text_encoding(handle));

    // The text may have been spliced; drop the cached line-end bound.
    handle->line_cache_anchor = 0;
    handle->line_cache_end = 0;

    // v2 definitions (contextual lexer) are tokenized by the mode/priority-aware
    // contextual lexer instead of the legacy scanner. The legacy scanner ignores
    // lexer modes and token priorities, so it mislabels mode-dependent trivia.
    if (definition->lexer_rules != nullptr)
        return textparser_parse_contextual(handle, definition);

    // Resolve active token from existing AST
    const textparser_token_item *active_token = nullptr;
    if (handle->first_item != nullptr && start_pos > 0) {
        active_token = find_token_at_position_internal(handle->first_item, start_pos - 1, 0);
    }

    // Fast path: an edit contained within a single leaf only changes lengths.
    // Re-match the leaf pattern at its start; when the whole edited leaf still
    // matches as one token, resize that leaf and all of its ancestors instead
    // of reparsing. This also prevents the leaf from being split or its
    // unchanged prefix from being dropped by the reparse stitch.
    if (active_token != nullptr && active_token->child == nullptr &&
        active_token->token_id >= 0)
    {
        size_t leaf_start = textparser_get_token_position(active_token);
        size_t leaf_end = leaf_start + active_token->len;
        if (leaf_start < edit_offset && edit_offset + old_len <= leaf_end)
        {
            const textparser_token *leaf_def = &definition->tokens[active_token->token_id];
            size_t found_at = 0;
            size_t match_len = 0;
            bool matched = textparser_match_start_token(
                handle, active_token->token_id,
                handle->text_addr + textparser_get_byte_offset(handle, leaf_start),
                textparser_get_search_len(handle, leaf_start, leaf_def),
                &found_at, &match_len, true, false);

            if (matched && found_at == 0 && match_len > 0 &&
                match_len == (size_t)((ssize_t)active_token->len + delta_units))
            {
                textparser_token_item *leaf_item = (textparser_token_item *)active_token;
                leaf_item->len = match_len;
                for (textparser_token_item *p = leaf_item->parent; p != nullptr; p = p->parent) {
                    p->len = (size_t)((ssize_t)p->len + delta_units);
                }

                if (out_range != nullptr) {
                    out_range->dirty_start = leaf_start;
                    out_range->dirty_end = leaf_start + match_len;
                }

                if (handle->lines) {
                    free(handle->lines);
                    handle->lines = nullptr;
                    handle->no_lines = 0;
                }

                // Patch the flat snapshot in place when the tree is a raw CST.
                // An AST tree needs a full rebuild because post-processing may
                // have changed token kinds relative to the snapshot; the head
                // carries the POST_PROCESSED marker in AST mode.
                bool ast_mode = was_post_processed ||
                    (handle->first_item != nullptr &&
                     ((handle->first_item->node_flags & TEXTPARSER_NODE_POST_PROCESSED) != 0 ||
                      (handle->first_item->text_flags & 0x80000000u) != 0));
                int rebuild_status = TEXTPARSER_OK;
                if (ast_mode ||
                    !textparser_patch_lexer_streams_leaf(handle, leaf_start, leaf_end,
                                                         leaf_start + match_len, delta_units, leaf_item)) {
                    rebuild_status = textparser_rebuild_lexer_streams(handle);
                }
                if (rebuild_status == TEXTPARSER_OK) {
                    textparser_memo_shift_and_invalidate(handle, 0, (size_t)-1, delta_units);
                }
                return rebuild_status;
            }
        }
    }

    // Materialize any pending snapshot bias before structural reparse begins
    textparser_snapshot_materialize_bias(handle);

    // Unwrap any synthesized post-processing nodes before re-tokenization so the
    // tree structure matches the base parser output before splicing and re-deriving.
    // The traversal also reports AST mode (marker and/or synthesized node); cast
    // and declaration disambiguation leave only the marker behind. Only when the
    // caller had run textparser_post_process must post-processing be re-derived
    // after the splice; a raw CST stays raw, preserving the documented contract of
    // textparser_parse / textparser_parse_incremental.
    if (handle->first_item != nullptr) {
        if (unwrap_post_processed_tokens(&handle->first_item)) {
            was_post_processed = true;
        }
    }

    // Anchor at the root: re-tokenize the top-level token containing the edit.
    // Anchoring at the nearest container was insufficient because a token's match
    // can depend on text outside its span (the JSON Key lookahead over its ':'
    // sibling) or on its own start/end delimiter (a CFML OutputStartTag inside an
    // OutputTagPair). The alignment below keeps the unchanged prefix/suffix nodes.
    const int *effective_starts_with = definition->starts_with;
    textparser_token_item *parent_container = nullptr;

    if (parent_container) {
        effective_starts_with = get_effective_nested_tokens(handle, parent_container->token_id, parent_container);
    } else if (definition->override_start_tokens && handle->filename) {
        const char *file_ext = strrchr(handle->filename, '.');
        if (file_ext) {
            file_ext++;
            for (int r = 0; definition->override_start_tokens[r].file_extensions != nullptr ||
                            definition->override_start_tokens[r].regex != nullptr ||
                            definition->override_start_tokens[r].start_tokens != nullptr; r++) {
                const textparser_override_start_token_rule *rule = &definition->override_start_tokens[r];
                bool ext_matches = false;
                if (rule->file_extensions) {
                    for (int e = 0; rule->file_extensions[e] != nullptr; e++) {
                        bool match = false;
                        if (definition->case_sensitivity) {
                            match = (strcmp(file_ext, rule->file_extensions[e]) == 0);
                        } else {
#ifdef _WIN32
                            match = (_stricmp(file_ext, rule->file_extensions[e]) == 0);
#else
                            match = (strcasecmp(file_ext, rule->file_extensions[e]) == 0);
#endif
                        }
                        if (match) {
                            ext_matches = true;
                            break;
                        }
                    }
                }

                if (ext_matches && rule->regex && rule->start_tokens) {
                    void *rule_regex = nullptr;
                    size_t found_at = 0;
                    size_t found_len = 0;
                    bool matched = adv_regex_find_pattern_ctx(handle->regex_ctx, rule->regex, &rule_regex, handle->text_format, handle->text_addr, handle->text_size, &found_at, &found_len, !definition->case_sensitivity, true);
                    if (rule_regex) {
                        adv_regex_free(handle->regex_ctx, &rule_regex, handle->text_format);
                    }
                    if (matched) {
                        effective_starts_with = rule->start_tokens;
                        break;
                    }
                }
            }
        }
    }

    const bool container_other_text_inside = parent_container
        ? definition->tokens[parent_container->token_id].other_text_inside
        : definition->other_text_inside;

    textparser_token_item *prev_item = nullptr;
    textparser_token_item *sibling_list = parent_container ? parent_container->child : handle->first_item;

    // Expand the dirty region to whole sibling token boundaries so the reparse
    // never starts or ends in the middle of a token. Backing up to the leaf that
    // contains the character before the edit (and out to the end of the leaf at
    // the edit's right edge) prevents greedy leaves from being split and their
    // unchanged prefixes from being dropped.
    textparser_token_run old_run = {0};
    textparser_token_item *dirty_first = nullptr;
    textparser_token_item *dirty_last = nullptr;
    if (sibling_list != nullptr)
    {
        size_t base_pos = parent_container ? textparser_get_token_position(parent_container) : 0;
        size_t probe_left = (edit_offset > 0) ? edit_offset - 1 : edit_offset;
        size_t probe_right = old_end_bound;

        size_t p = base_pos;
        for (textparser_token_item *curr = sibling_list; curr != nullptr; curr = curr->next)
        {
            size_t end = p + curr->len;
            if (dirty_first == nullptr && probe_left >= p && probe_left < end) dirty_first = curr;
            if (probe_right >= p && probe_right <= end) dirty_last = curr;
            p = end;
        }

        // The reparse runs with the container's nested-token rules, which never
        // match the container delimiters themselves. Back away from them.
        if (dirty_first != nullptr && dirty_first->token_id == TEXTPARSER_TOKEN_ID_END_DELIMITER)
            dirty_first = dirty_first->prev;

        // Start-token end searches have unbounded lookahead, so an edit can
        // change the extent of an earlier top-level token. Reparse from the
        // first sibling; the alignment keeps the unchanged prefix/suffix nodes.
        dirty_first = sibling_list;

        // Extend the reparse to the end of the container's children so an
        // overrun (a greedy token or a newly opened container) can resync with
        // the old suffix. The alignment then keeps the unchanged prefix/suffix.
        if (dirty_last != nullptr) {
            textparser_token_item *scan = dirty_last;
            while (scan->next != nullptr &&
                   scan->next->token_id != TEXTPARSER_TOKEN_ID_END_DELIMITER) {
                scan = scan->next;
            }
            dirty_last = scan;
        }

        size_t expanded_start = start_pos;
        size_t expanded_old_end = old_end_bound;
        if (dirty_first != nullptr)
        {
            expanded_start = textparser_get_token_position(dirty_first);
            if (dirty_first->token_id == TEXTPARSER_TOKEN_ID_START_DELIMITER)
                expanded_start += dirty_first->len;
        }
        if (dirty_last != nullptr)
        {
            expanded_old_end = textparser_get_token_position(dirty_last) + dirty_last->len;
            if (dirty_last->token_id == TEXTPARSER_TOKEN_ID_END_DELIMITER)
                expanded_old_end = textparser_get_token_position(dirty_last);
        }
        if (expanded_start < start_pos) start_pos = expanded_start;
        if (expanded_old_end > old_end_bound) old_end_bound = expanded_old_end;
        end_pos = old_end_bound - old_len + new_len;

        if (dirty_first != nullptr && dirty_last != nullptr)
        {
            for (textparser_token_item *curr = dirty_first; ; curr = curr->next)
            {
                if (!textparser_run_push(&old_run, &handle->scratch, curr)) {
                    return TEXTPARSER_ERROR_OUT_OF_MEMORY;
                }
                if (curr == dirty_last) break;
            }
        }
    }

    if (start_pos > 0 && sibling_list != nullptr)
    {
        textparser_token_item *curr = sibling_list;
        size_t curr_pos = parent_container ? textparser_get_token_position(parent_container) : 0;
        while (curr)
        {
            if (curr_pos + curr->len <= start_pos)
            {
                prev_item = curr;
            }
            else
            {
                break;
            }
            curr_pos += curr->len;
            curr = curr->next;
        }
    }

    textparser_token_item *tail_first = nullptr;
    if (sibling_list != nullptr && old_end_bound < old_total_units)
    {
        textparser_token_item *curr = prev_item ? prev_item->next : sibling_list;
        size_t curr_pos = prev_item ? (textparser_get_token_position(prev_item) + prev_item->len) : (parent_container ? textparser_get_token_position(parent_container) : 0);
        while (curr)
        {
            if (curr_pos >= old_end_bound)
            {
                tail_first = curr;
                break;
            }
            curr_pos += curr->len;
            curr = curr->next;
        }
    }

    textparser_token_item *stitch_left_before = prev_item;
    textparser_token_item *stitch_right_before = tail_first;
    size_t pos = start_pos;
    textparser_token_run new_run = {0};

    // Phase 1 bounded forward-resync: walk an old-sibling cursor in parallel
    // with the new-token emission. Once we have emitted a new token whose start
    // is >= resync_safe_pos (strictly past the dirty edit window), compare it to
    // the corresponding old sibling. A pure leaf (no children) whose token_id
    // and len both match is a safe resync point: we clamp end_pos to stop the
    // reparse there and let the existing suffix-alignment reuse the unchanged old
    // suffix. Containers are excluded because their interior may have changed
    // even when the outer span happens to be identical.
    //
    // The cursor starts at the first old sibling that covers start_pos.
    textparser_token_item *resync_old_cursor = nullptr;
    size_t resync_old_cursor_pos = 0;
    {
        textparser_token_item *c = sibling_list;
        size_t cp = parent_container ? textparser_get_token_position(parent_container) : 0;
        while (c != nullptr) {
            if (cp + c->len > start_pos) {
                resync_old_cursor = c;
                resync_old_cursor_pos = cp;
                break;
            }
            cp += c->len;
            c = c->next;
        }
    }
    // Minimum new-coordinate position that is guaranteed to be past the dirty window.
    const size_t resync_safe_pos = edit_offset + new_len;

    while(pos < end_pos) {
        size_t ws_skipped = textparser_skip_whitespace(handle, pos) - pos;
        if (ws_skipped > 0) {
            textparser_token_item **head_ptr = parent_container ? &parent_container->child : &handle->first_item;
            textparser_token_item *before = prev_item;
            textparser_token_item *appended = append_whitespace_if_needed(handle, parent_container, head_ptr, &prev_item, ws_skipped);
            if (appended != nullptr && appended != before) {
                if (!textparser_run_push(&new_run, &handle->scratch, appended)) return TEXTPARSER_ERROR_OUT_OF_MEMORY;
            }
            pos += ws_skipped;
        }
        if (pos >= end_pos)
            break;

        textparser_token_item *token_item = nullptr;
        textparser_token_item *error_token_item = nullptr;
        const char *error_token_msg = nullptr;
        size_t error_token_off = 0;
        for (int c = 0; effective_starts_with && effective_starts_with[c] != TextParser_END; c++) {
            int token_id = effective_starts_with[c];
            ssize_t offset = textparser_find_token(handle, token_id, pos, container_other_text_inside, parent_container, prev_item);
            if (offset == 0)
            {
                textparser_arena_checkpoint cp = textparser_arena_checkpoint_save(handle);
                const char *saved_err = handle->error;
                size_t saved_err_off = handle->error_offset;

                int parent_tok_id = parent_container ? parent_container->token_id : TextParser_END;
                textparser_token_item *attempt = textparser_parse_token(handle, token_id, parent_tok_id, TEXTPARSER_SEARCH_END_TOKEN, pos, parent_container, prev_item);
                if (attempt != nullptr && handle->error == nullptr && attempt->len > 0) {
                    token_item = attempt;
                    break;
                }

                if (attempt != nullptr && handle->error != nullptr && error_token_item == nullptr) {
                    // Remember the first failing candidate and its diagnostic,
                    // but clear the error so the remaining candidates are still
                    // given a chance (the checkpoint is intentionally not
                    // restored so the remembered node stays valid).
                    error_token_item = attempt;
                    error_token_msg = handle->error;
                    error_token_off = handle->error_offset;
                    handle->error = saved_err;
                    handle->error_offset = saved_err_off;
                } else {
                    textparser_arena_checkpoint_restore(handle, &cp);
                    handle->error = saved_err;
                    handle->error_offset = saved_err_off;
                }
            }
        }

        if (token_item == nullptr && error_token_item != nullptr) {
            token_item = error_token_item;
            handle->error = error_token_msg;
            handle->error_offset = error_token_off;
        }

        // Match the full parser's recovery: inside a nested otherTextInside
        // container a failed candidate emits one unit as Unprocessed and parsing
        // continues, instead of aborting the whole edit. Top-level errors fall
        // through to the normal link-then-fail path so the partial tree remains.
        if (token_item != nullptr && handle->error != nullptr &&
            !handle->parse_budget_exceeded &&
            parent_container != nullptr && container_other_text_inside &&
            pos < textparser_get_total_units(handle)) {
            handle->error = nullptr;
            handle->error_offset = 0;
            handle->error_length = 0;
            size_t char_l = textparser_char_len(handle, pos);
            textparser_token_item **head_ptr = parent_container ? &parent_container->child : &handle->first_item;
            textparser_token_item *before = prev_item;
            textparser_token_item *appended = append_unprocessed_if_needed(handle, parent_container, head_ptr, &prev_item, char_l);
            if (appended != nullptr && appended != before) {
                if (!textparser_run_push(&new_run, &handle->scratch, appended)) return TEXTPARSER_ERROR_OUT_OF_MEMORY;
            }
            pos += char_l;
            continue;
        }

        if (token_item != nullptr) {
            if (parent_container) {
                token_item->parent = parent_container;
            }

            if (prev_item) {
                prev_item->next = token_item;
                token_item->prev = prev_item;
            } else if (parent_container) {
                parent_container->child = token_item;
            } else {
                handle->first_item = token_item;
            }

            size_t token_advance = token_item->len;

            if ((handle->error)||(token_item->len <= 0))
                return -1;

            if (!textparser_run_push(&new_run, &handle->scratch, token_item)) return TEXTPARSER_ERROR_OUT_OF_MEMORY;

            pos += token_advance;
            prev_item = token_item;

            // Advance the old-sibling cursor to stay aligned with the new pos.
            // The old cursor lives in old coordinates: new pos maps to
            // (pos - delta_units) in old-document space.  Walk the cursor
            // forward until it reaches the sibling that starts at that position.
            if (resync_old_cursor != nullptr) {
                ssize_t old_pos_signed = (ssize_t)pos - delta_units;
                size_t old_pos = (old_pos_signed > 0) ? (size_t)old_pos_signed : 0;
                while (resync_old_cursor != nullptr &&
                       resync_old_cursor_pos + resync_old_cursor->len <= old_pos) {
                    resync_old_cursor_pos += resync_old_cursor->len;
                    resync_old_cursor = resync_old_cursor->next;
                }

                // Attempt a resync once the newly emitted token starts past the
                // dirty edit window.  Only pure leaf nodes (no children) are
                // considered: containers may have changed internal structure even
                // when the outer span is unchanged, and tokens with errors must
                // not be used as anchors.
                if (pos >= resync_safe_pos &&
                    resync_old_cursor != nullptr &&
                    handle->error == nullptr &&
                    token_item->child == nullptr &&
                    resync_old_cursor->child == nullptr &&
                    resync_old_cursor->token_id == token_item->token_id &&
                    resync_old_cursor->len == token_item->len &&
                    resync_old_cursor_pos == old_pos - token_item->len) {
                    // The freshly emitted token matches the old sibling exactly.
                    // Stop the reparse here; the suffix alignment below will reuse
                    // all old siblings from resync_old_cursor->next onward.
                    end_pos = pos;
                }
            }
        } else {
            if (container_other_text_inside) {
                size_t char_l = textparser_char_len(handle, pos);
                textparser_token_item **head_ptr = parent_container ? &parent_container->child : &handle->first_item;
                textparser_token_item *before = prev_item;
                textparser_token_item *appended = append_unprocessed_if_needed(handle, parent_container, head_ptr, &prev_item, char_l);
                if (appended != nullptr && appended != before) {
                    if (!textparser_run_push(&new_run, &handle->scratch, appended)) return TEXTPARSER_ERROR_OUT_OF_MEMORY;
                }
                pos += char_l;
            } else {
                break;
            }
        }
    }

    if (pos < end_pos) {
        textparser_token_item **head_ptr = parent_container ? &parent_container->child : &handle->first_item;
        textparser_token_item *before = prev_item;
        textparser_token_item *appended = append_unprocessed_if_needed(handle, parent_container, head_ptr, &prev_item, end_pos - pos);
        if (appended != nullptr && appended != before) {
            if (!textparser_run_push(&new_run, &handle->scratch, appended)) return TEXTPARSER_ERROR_OUT_OF_MEMORY;
        }
        pos = end_pos;
    }

    // Resync the suffix: a greedy token or a newly opened container can consume
    // text past the old dirty end, so advance the old suffix anchor to the token
    // at the reparse's actual end (mapped back to old coordinates). Old tokens
    // the new run now covers are dropped.
    if (stitch_right_before != nullptr) {
        ssize_t old_suffix_signed = (ssize_t)pos - delta_units;
        size_t old_suffix_pos = (old_suffix_signed > 0) ? (size_t)old_suffix_signed : 0;
        size_t right_pos = textparser_get_token_position(stitch_right_before);
        while (stitch_right_before != nullptr && right_pos < old_suffix_pos) {
            right_pos += stitch_right_before->len;
            stitch_right_before = stitch_right_before->next;
        }
    }

    // Run items are consecutive siblings, so their positions can be accumulated
    // once. The alignment loops below previously called
    // textparser_get_token_position (a prev/parent chain walk) several times per
    // candidate, making alignment O(window * depth).
    size_t *old_pos = nullptr;
    size_t *new_pos = nullptr;
    if (old_run.count > 0) {
        old_pos = arena_alloc(&handle->scratch, old_run.count * sizeof(size_t));
        if (old_pos == nullptr) return TEXTPARSER_ERROR_OUT_OF_MEMORY;
        size_t p = textparser_get_token_position(old_run.items[0]);
        for (size_t i = 0; i < old_run.count; i++) {
            old_pos[i] = p;
            p += old_run.items[i]->len;
        }
    }
    if (new_run.count > 0) {
        new_pos = arena_alloc(&handle->scratch, new_run.count * sizeof(size_t));
        if (new_pos == nullptr) return TEXTPARSER_ERROR_OUT_OF_MEMORY;
        size_t p = textparser_get_token_position(new_run.items[0]);
        for (size_t i = 0; i < new_run.count; i++) {
            new_pos[i] = p;
            p += new_run.items[i]->len;
        }
    }

    // Align the reparsed run against the old run: keep the unchanged common
    // prefix and suffix (reusing the existing nodes) and splice only the middle.
    size_t prefix = 0;
    while (prefix < old_run.count && prefix < new_run.count &&
           old_pos[prefix] + old_run.items[prefix]->len <= edit_offset &&
           new_pos[prefix] == old_pos[prefix] &&
           textparser_tokens_shape_equal(old_run.items[prefix], new_run.items[prefix])) {
        prefix++;
    }
    size_t suffix = 0;
    while (suffix < old_run.count - prefix && suffix < new_run.count - prefix) {
        size_t old_index = old_run.count - 1 - suffix;
        size_t new_index = new_run.count - 1 - suffix;
        if (old_pos[old_index] < edit_offset + old_len) break;
        if (new_pos[new_index] != old_pos[old_index] + delta_units) break;
        if (!textparser_tokens_shape_equal(old_run.items[old_index], new_run.items[new_index])) break;
        suffix++;
    }

    textparser_token_item *keep_left = (prefix > 0) ? old_run.items[prefix - 1] : stitch_left_before;
    textparser_token_item *keep_right = (suffix > 0) ? old_run.items[old_run.count - suffix] : stitch_right_before;

    // When the common prefix is reused, the old nodes are already linked. Only
    // the reparse of a run that started at the container's first child can have
    // overwritten the head, so only then is it restored.
    if (prefix > 0 && stitch_left_before == nullptr) {
        if (parent_container != nullptr) {
            parent_container->child = old_run.items[0];
        } else {
            handle->first_item = old_run.items[0];
        }
    }

    // Free any synthesized post-processed tokens in discarded old_run items.
    for (size_t i = prefix; i + suffix < old_run.count; i++) {
        free_post_processed_tokens(old_run.items[i]);
    }

    textparser_token_item *left = keep_left;
    textparser_token_item *first_spliced = nullptr;
    textparser_token_item *last_spliced = nullptr;
    for (size_t i = prefix; i + suffix < new_run.count; i++) {
        textparser_token_item *item = new_run.items[i];
        if (first_spliced == nullptr) first_spliced = item;
        last_spliced = item;
        item->parent = parent_container;
        item->prev = left;
        if (left != nullptr) {
            left->next = item;
        } else if (parent_container != nullptr) {
            parent_container->child = item;
        } else {
            handle->first_item = item;
        }
        left = item;
    }
    if (left != nullptr) {
        left->next = keep_right;
    } else if (parent_container != nullptr) {
        parent_container->child = keep_right;
    } else {
        handle->first_item = keep_right;
    }
    if (keep_right != nullptr) {
        keep_right->prev = left;
    }

    // Rebuild `prev` links from the head before sign merging. The splice can
    // leave a reused node's `prev` pointing at a replaced node, and
    // `maybe_merge_sign` unlinks through both `prev->next` and `next->prev`, so
    // an inconsistent `prev` would leave the sign in the list.
    {
        textparser_token_item *prev = nullptr;
        for (textparser_token_item *t = handle->first_item; t != nullptr; t = t->next) {
            t->prev = prev;
            prev = t;
        }
    }

    // Sign merging runs after the splice so it cannot be undone by re-linking.
    // The walk is bounded by the spliced region: it stops at the last spliced
    // node (or the kept right anchor), never scanning to the end of the sibling
    // list when there is no right anchor.
    {
        textparser_token_item *merge_start = keep_left;
        if (merge_start == nullptr) {
            merge_start = parent_container ? parent_container->child : handle->first_item;
        }
        textparser_token_item *merge_last = (new_run.count > 0)
            ? new_run.items[new_run.count - 1]
            : keep_right;
        for (textparser_token_item *t = merge_start; t != nullptr; ) {
            textparser_token_item *next = t->next;
            maybe_merge_sign(handle, t);
            if (t == merge_last || t == keep_right) break;
            t = next;
        }
    }

    size_t aligned_dirty_start = keep_left
        ? textparser_get_token_position(keep_left) + keep_left->len
        : start_pos;
    size_t aligned_dirty_end = keep_right
        ? textparser_get_token_position(keep_right)
        : textparser_get_total_units(handle);

    if (parent_container && delta_units != 0) {
        textparser_token_item *p = parent_container;
        while (p) {
            p->len = (size_t)((ssize_t)p->len + delta_units);
            p = p->parent;
        }
    }

    if (out_range != nullptr) {
        out_range->dirty_start = aligned_dirty_start;
        out_range->dirty_end = aligned_dirty_end;
    }

    if (handle->lines && (delta_units != 0 || start_pos == 0)) {
        free(handle->lines);
        handle->lines = nullptr;
        handle->no_lines = 0;
    }

    // The dirty lexer-token range must reflect the tokens that actually
    // changed, not the (possibly larger) reparse window, so unchanged suffix
    // tokens keep their memo entries.
    size_t changed_start = start_pos;
    size_t changed_end = old_end_bound;
    if (prefix < old_run.count && old_run.count - suffix > prefix) {
        changed_start = old_pos[prefix];
        size_t last_changed = old_run.count - suffix - 1;
        changed_end = old_pos[last_changed] + old_run.items[last_changed]->len;
    } else {
        changed_start = edit_offset;
        changed_end = edit_offset;
    }

    size_t old_token_count = handle->lexer_token_count;
    size_t old_dirty_tok_start = old_token_count;
    size_t old_dirty_tok_end = old_token_count;
    if (handle->lexer_tokens != nullptr) {
        for (size_t i = 0; i < old_token_count; i++) {
            if (handle->lexer_tokens[i].end > changed_start) {
                old_dirty_tok_start = i;
                break;
            }
        }
        for (size_t i = old_dirty_tok_start; i < old_token_count; i++) {
            if (handle->lexer_tokens[i].start >= changed_end) {
                old_dirty_tok_end = i;
                break;
            }
        }
    }

    // Re-derive the AST post-processing only when the caller had already applied
    // it to this tree before the edit. This keeps expression trees, template
    // groups, disambiguated tokens and delete_if_only_one_child unwrapping
    // consistent across edits without changing the raw CST contract for callers
    // that never call textparser_post_process.
    //
    // Phase 4: When the splice is isolated within an existing container (parent_container != nullptr),
    // we can scope post-processing to that container's subtree rather than re-traversing the whole
    // document from the root.
    if (was_post_processed)
    {
        if (parent_container != nullptr && parent_container->child != nullptr) {
            textparser_post_process(&parent_container->child, definition);
            if (handle->first_item != nullptr) {
                handle->first_item->node_flags |= TEXTPARSER_NODE_POST_PROCESSED;
            }
        } else {
            textparser_post_process(&handle->first_item, definition);
        }
    }

    int rebuild_status = textparser_rebuild_lexer_streams(handle);
    if (rebuild_status == TEXTPARSER_OK) {
        ssize_t delta_tokens = (ssize_t)handle->lexer_token_count - (ssize_t)old_token_count;
        textparser_memo_shift_and_invalidate(handle, old_dirty_tok_start, old_dirty_tok_end, delta_tokens);
    }

    return rebuild_status;
}

/**
 * Look up operator precedence, associativity, and prefix role for a token ID in language rules.
 *
 * @param language Active language definition.
 * @param token_id Token ID to look up.
 * @param out_precedence Output pointer receiving numeric precedence rank.
 * @param out_assoc Output pointer receiving operator associativity.
 * @param out_is_prefix Output pointer receiving true if operator acts as prefix.
 * @return true if operator rule was found for token_id; false otherwise.
 */
static bool get_operator_info(
    const textparser_language_definition *language,
    int token_id,
    int *out_precedence,
    enum textparser_associativity *out_assoc,
    bool *out_is_prefix
)
{
    if (language == nullptr || language->operator_precedence == nullptr || token_id < 0) {
        return false;
    }

    const textparser_operator_precedence *op_prec = language->operator_precedence;
    for (size_t r = 0; r < op_prec->count; r++) {
        const textparser_precedence_rule *rule = &op_prec->rules[r];
        if (rule->operators == nullptr) continue;
        for (int i = 0; rule->operators[i] != TextParser_END; i++) {
            if (rule->operators[i] == token_id) {
                if (out_precedence) *out_precedence = (int)(r + 1);
                if (out_assoc) *out_assoc = rule->associativity;
                if (out_is_prefix) {
                    const char *name = (language->tokens != nullptr) ? language->tokens[token_id].name : nullptr;
                    *out_is_prefix = (name != nullptr && (strstr(name, "Not") != nullptr || strstr(name, "Unary") != nullptr));
                }
                return true;
            }
        }
    }
    return false;
}

/**
 * Wrap an operand node with a unary operator node during expression post-processing.
 *
 * @param op_token Operator token item node.
 * @param operand Operand token item node.
 * @return Pointer to the constructed unary operator node, or NULL on failure.
 */
static textparser_token_item *make_unary_node(
    textparser_token_item *op_token,
    textparser_token_item *operand
)
{
    if (op_token == nullptr) return operand;

    textparser_token_item *parent_node = calloc(1, sizeof(textparser_token_item));
    if (parent_node == nullptr) return op_token;

    parent_node->token_id = op_token->token_id;
    parent_node->text_color = op_token->text_color;
    parent_node->text_background = op_token->text_background;
    parent_node->text_flags = op_token->text_flags | 0x80000000;

    op_token->prev = nullptr;
    op_token->next = operand;
    if (operand != nullptr) {
        operand->prev = op_token;
    }

    parent_node->child = op_token;

    size_t total_len = 0;
    for (textparser_token_item *c = op_token; c != nullptr; c = c->next) {
        c->parent = parent_node;
        total_len += c->len;
    }
    parent_node->len = total_len;

    return parent_node;
}

/**
 * Link left and right operand nodes under a binary operator parent during expression post-processing.
 *
 * @param left Left-hand operand node.
 * @param op_token Operator token item node.
 * @param right Right-hand operand node.
 * @return Pointer to the binary operator node linking left and right children.
 */
static textparser_token_item *make_binary_node(
    textparser_token_item *left,
    textparser_token_item *op_token,
    textparser_token_item *right
)
{
    if (op_token == nullptr) return left ? left : right;
    if (left == nullptr) return make_unary_node(op_token, right);

    textparser_token_item *parent_node = calloc(1, sizeof(textparser_token_item));
    if (parent_node == nullptr) return op_token;

    parent_node->token_id = op_token->token_id;
    parent_node->text_color = op_token->text_color;
    parent_node->text_background = op_token->text_background;
    parent_node->text_flags = op_token->text_flags | 0x80000000;

    textparser_token_item *left_last = left;
    while (left_last->next != nullptr) {
        left_last = left_last->next;
    }

    left_last->next = op_token;
    op_token->prev = left_last;

    if (right != nullptr) {
        op_token->next = right;
        right->prev = op_token;
    } else {
        op_token->next = nullptr;
    }

    parent_node->child = left;

    size_t total_len = 0;
    for (textparser_token_item *c = left; c != nullptr; c = c->next) {
        c->parent = parent_node;
        total_len += c->len;
    }
    parent_node->len = total_len;

    return parent_node;
}

/**
 * Parse an expression subtree from a linear sequence of nodes using top-down operator precedence.
 *
 * @param language Pointer to the active language definition structure.
 * @param items Array of token item nodes.
 * @param count Total count of items in array.
 * @param idx In/out index in items array.
 * @param min_precedence Minimum binding power required to consume next operator.
 * @return Root node of the constructed expression subtree.
 */
static textparser_token_item *pratt_parse_expression_stream(
    const textparser_language_definition *language,
    textparser_token_item **items,
    int count,
    int *idx,
    int min_precedence
)
{
    if (*idx >= count) return nullptr;

    textparser_token_item *left = nullptr;

    // Collect any leading trivia before first operand/operator
    textparser_token_item *leading_trivia_head = nullptr;
    textparser_token_item *leading_trivia_tail = nullptr;
    while (*idx < count && is_trivia_token_id(items[*idx]->token_id)) {
        textparser_token_item *t = items[(*idx)++];
        t->prev = leading_trivia_tail;
        t->next = nullptr;
        if (leading_trivia_tail) leading_trivia_tail->next = t;
        else leading_trivia_head = t;
        leading_trivia_tail = t;
    }

    if (*idx >= count) {
        return leading_trivia_head;
    }

    textparser_token_item *token = items[(*idx)++];

    int op_prec = 0;
    enum textparser_associativity assoc = TEXTPARSER_ASSOC_LEFT;
    bool is_prefix = false;

    if (get_operator_info(language, token->token_id, &op_prec, &assoc, &is_prefix) && is_prefix) {
        token->prev = nullptr;
        token->next = nullptr;
        textparser_token_item *operand = pratt_parse_expression_stream(language, items, count, idx, op_prec);
        left = make_unary_node(token, operand);
    } else {
        token->prev = nullptr;
        token->next = nullptr;
        left = token;
    }

    if (leading_trivia_head != nullptr) {
        leading_trivia_tail->next = left;
        left->prev = leading_trivia_tail;
        left = leading_trivia_head;
    }

    while (*idx < count) {
        int scan_idx = *idx;
        while (scan_idx < count && is_trivia_token_id(items[scan_idx]->token_id)) {
            scan_idx++;
        }
        if (scan_idx >= count) break;

        textparser_token_item *op_cand = items[scan_idx];
        int cand_prec = 0;
        enum textparser_associativity cand_assoc = TEXTPARSER_ASSOC_LEFT;
        bool cand_is_prefix = false;

        if (!get_operator_info(language, op_cand->token_id, &cand_prec, &cand_assoc, &cand_is_prefix) || cand_is_prefix) {
            break;
        }

        if (cand_prec < min_precedence) {
            break;
        }

        while (*idx < scan_idx) {
            textparser_token_item *trivia = items[(*idx)++];
            trivia->prev = nullptr;
            trivia->next = nullptr;
            textparser_token_item *left_last = left;
            while (left_last->next != nullptr) left_last = left_last->next;
            left_last->next = trivia;
            trivia->prev = left_last;
        }

        textparser_token_item *op_token = items[(*idx)++];
        op_token->prev = nullptr;
        op_token->next = nullptr;

        int next_min_prec = (cand_assoc == TEXTPARSER_ASSOC_LEFT) ? (cand_prec + 1) : cand_prec;
        textparser_token_item *right = pratt_parse_expression_stream(language, items, count, idx, next_min_prec);

        left = make_binary_node(left, op_token, right);
    }

    return left;
}

/**
 * Recursively traverse CST containers to reorganize linear operator sequences into expression trees.
 *
 * @param head In/out pointer to head of node or token list.
 * @param language Pointer to the active language definition structure.
 */
static void textparser_post_process_expressions(textparser_token_item **head, const textparser_language_definition *language)
{
    if (head == nullptr || *head == nullptr || language == nullptr || language->operator_precedence == nullptr) return;

    bool has_operator = false;
    int count = 0;
    for (textparser_token_item *curr = *head; curr != nullptr; curr = curr->next) {
        count++;
        if (curr->token_id >= 0) {
            int prec = 0;
            if (get_operator_info(language, curr->token_id, &prec, nullptr, nullptr)) {
                has_operator = true;
            }
        }
    }

    if (!has_operator || count <= 1) return;

    textparser_token_item **items = malloc(count * sizeof(textparser_token_item *));
    if (items == nullptr) return;

    int i = 0;
    for (textparser_token_item *curr = *head; curr != nullptr; curr = curr->next) {
        items[i++] = curr;
    }

    int idx = 0;
    textparser_token_item *new_head = nullptr;
    textparser_token_item *new_tail = nullptr;

    while (idx < count) {
        textparser_token_item *parsed = pratt_parse_expression_stream(language, items, count, &idx, 0);
        if (parsed == nullptr) break;

        if (new_head == nullptr) {
            new_head = parsed;
            new_tail = parsed;
        } else {
            new_tail->next = parsed;
            parsed->prev = new_tail;
            new_tail = parsed;
        }
        while (new_tail->next != nullptr) {
            new_tail = new_tail->next;
        }
    }

    textparser_token_item *orig_parent = (*head)->parent;
    for (textparser_token_item *c = new_head; c != nullptr; c = c->next) {
        c->parent = orig_parent;
    }

    *head = new_head;
    free(items);
}

/**
 * Check whether a container token node represents an expression or operator group.
 *
 * @param node Token item node to evaluate.
 * @param language Pointer to the active language definition structure.
 * @return true if node represents an operator group container; false otherwise.
 */
static bool is_operator_group(const textparser_token_item *node, const textparser_language_definition *language)
{
    if (node == nullptr || node->token_id < 0 || language == nullptr || language->tokens == nullptr) return false;
    enum textparser_token_type ttype = language->tokens[node->token_id].type;
    if (ttype != TEXTPARSER_TOKEN_TYPE_GROUP &&
        ttype != TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER &&
        ttype != TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY) {
        return false;
    }
    const char *name = language->tokens[node->token_id].name;
    if (name != nullptr && strcmp(name, "Operator") == 0) return true;

    if (node->child == nullptr) return false;
    for (const textparser_token_item *c = node->child; c != nullptr; c = c->next) {
        if (is_trivia_token_id(c->token_id)) continue;
        int prec = 0;
        if (!get_operator_info(language, c->token_id, &prec, nullptr, nullptr)) {
            return false;
        }
    }
    return true;
}

/**
 * Unwrap and collapse single-child container nodes marked deleteIfOnlyOneChild.
 *
 * @param root Root CST node of the document or subtree.
 * @param curr Current CST node in traversal.
 * @return Replacement child node if unwrapped, or original node if unchanged.
 */
static void unwrap_node(textparser_token_item **root, textparser_token_item *curr)
{
    if (curr == nullptr || curr->child == nullptr) return;
    textparser_token_item *first_child = curr->child;
    textparser_token_item *last_child = curr->child;
    while (last_child->next) {
        last_child->parent = curr->parent;
        last_child = last_child->next;
    }
    last_child->parent = curr->parent;
    first_child->prev = curr->prev;
    last_child->next = curr->next;

    if (curr->prev) {
        curr->prev->next = first_child;
    } else if (curr->parent) {
        curr->parent->child = first_child;
    } else if (root) {
        *root = first_child;
    }

    if (curr->next) {
        curr->next->prev = last_child;
    }
}

/**
 * Disambiguate C-style type cast expressions from parenthesized expressions in CST.
 *
 * @param root Root CST node of the document or subtree.
 * @param language Pointer to the active language definition structure.
 */
static void textparser_disambiguate_casts(textparser_token_item **root, const textparser_language_definition *language)
{
    if (root == nullptr || *root == nullptr || language == nullptr || language->cast_disambiguation == nullptr) return;
    const textparser_cast_disambiguation *cst = language->cast_disambiguation;
    if (cst->cast_token_id < 0) return;

    for (textparser_token_item *curr = *root; curr != nullptr; curr = curr->next) {
        if (curr->token_id < 0) continue;
        const char *name = language->tokens[curr->token_id].name;
        if (name == nullptr || (strcmp(name, "Parenthesis") != 0 && strcmp(name, "parenthesis") != 0)) continue;

        if (curr->child == nullptr) continue;

        bool has_type_token = false;
        bool all_valid_types = true;

        for (textparser_token_item *c = curr->child; c != nullptr; c = c->next) {
            if (is_trivia_token_id(c->token_id)) continue;
            if (c->token_id < 0) {
                all_valid_types = false;
                break;
            }
            const char *c_name = language->tokens[c->token_id].name;
            if (c_name == nullptr) {
                all_valid_types = false;
                break;
            }

            if (strcmp(c_name, "Operator") == 0 || strcmp(c_name, "operator") == 0) {
                continue;
            }

            if (strcmp(c_name, "Keyword") == 0 || strcmp(c_name, "keyword") == 0) {
                has_type_token = true;
                continue;
            }

            if (textparser_token_in_id_list(cst->type_tokens, c->token_id)) {
                has_type_token = true;
                continue;
            }

            all_valid_types = false;
            break;
        }

        if (has_type_token && all_valid_types) {
            textparser_token_item *after = curr->next;
            while (after && is_trivia_token_id(after->token_id)) {
                after = after->next;
            }
            if (after != nullptr && after->token_id >= 0) {
                curr->token_id = cst->cast_token_id;
                curr->text_color = language->tokens[cst->cast_token_id].text_color;
            }
        }
    }
}

/**
 * Locate the closest preceding sibling token node that is not whitespace or comment trivia.
 *
 * @param token Current token item node.
 * @return Pointer to previous significant sibling token, or NULL if none.
 */
static textparser_token_item *previous_significant_token(textparser_token_item *token)
{
    if (token != nullptr) token = token->prev;
    while (token != nullptr && is_trivia_token_id(token->token_id)) token = token->prev;
    return token;
}

/**
 * Locate the closest following sibling token node that is not whitespace or comment trivia.
 *
 * @param token Current token item node.
 * @return Pointer to next significant sibling token, or NULL if none.
 */
static textparser_token_item *next_significant_token(textparser_token_item *token)
{
    if (token != nullptr) token = token->next;
    while (token != nullptr && is_trivia_token_id(token->token_id)) token = token->next;
    return token;
}

/**
 * Disambiguate variable or function declarations from expressions based on declarator tokens.
 *
 * @param root Root CST node of the document or subtree.
 * @param language Pointer to the active language definition structure.
 */
static void textparser_disambiguate_declarations(textparser_token_item **root, const textparser_language_definition *language)
{
    if (root == nullptr || *root == nullptr || language == nullptr || language->declaration_disambiguation == nullptr) return;
    const textparser_declaration_disambiguation *decl = language->declaration_disambiguation;
    if (decl->identifier_token_id < 0 || decl->type_name_token_id < 0 ||
        decl->function_token_id < 0 || decl->parameter_list_token_id < 0) return;

    textparser_token_item *parameter_list = (*root)->parent;
    if (parameter_list == nullptr || parameter_list->token_id != decl->parameter_list_token_id) return;

    textparser_token_item *function = previous_significant_token(parameter_list);
    if (function == nullptr || function->token_id != decl->function_token_id) return;

    textparser_token_item *return_type = previous_significant_token(function);
    if (return_type == nullptr || !textparser_token_in_id_list(decl->return_type_tokens, return_type->token_id)) return;

    for (textparser_token_item *candidate = *root; candidate != nullptr; candidate = candidate->next) {
        if (candidate->token_id != decl->identifier_token_id) continue;

        textparser_token_item *declarator = next_significant_token(candidate);
        while (declarator != nullptr && textparser_token_in_id_list(decl->declarator_tokens, declarator->token_id)) {
            declarator = next_significant_token(declarator);
        }
        if (declarator == nullptr || declarator->token_id != decl->identifier_token_id) continue;

        candidate->token_id = decl->type_name_token_id;
        candidate->text_color = language->tokens[decl->type_name_token_id].text_color;
        candidate = declarator;
    }
}

/**
 * Disambiguate template/generic bracket pairs (<...>) from comparison operators in CST.
 *
 * @param root Root CST node of the document or subtree.
 * @param language Pointer to the active language definition structure.
 */
static void textparser_disambiguate_templates(textparser_token_item **root, const textparser_language_definition *language)
{
    if (root == nullptr || *root == nullptr || language == nullptr || language->template_disambiguation == nullptr) return;
    const textparser_template_disambiguation *tpl = language->template_disambiguation;
    if (tpl->template_group_token_id < 0 && tpl->template_open_tokens == nullptr) return;

    textparser_token_item *curr = *root;
    while (curr) {
        textparser_token_item *next_item = curr->next;

        if (curr->token_id >= 0 && textparser_token_in_id_list(tpl->template_open_tokens, curr->token_id)) {
            textparser_token_item *prev = curr->prev;
            while (prev && is_trivia_token_id(prev->token_id)) {
                prev = prev->prev;
            }

            bool prev_is_qualifying = false;
            if (prev != nullptr && prev->token_id >= 0) {
                const char *p_name = language->tokens[prev->token_id].name;
                if (p_name && (strcmp(p_name, "Variable") == 0 || strcmp(p_name, "variable") == 0 ||
                               strcmp(p_name, "Keyword") == 0 || strcmp(p_name, "keyword") == 0 ||
                               strcmp(p_name, "TemplateGroup") == 0)) {
                    prev_is_qualifying = true;
                }
            }

            if (prev_is_qualifying) {
                int depth = 1;
                textparser_token_item *scan = curr->next;
                textparser_token_item *end_bracket = nullptr;
                bool is_valid_template = true;

                while (scan && depth > 0) {
                    if (is_trivia_token_id(scan->token_id)) {
                        scan = scan->next;
                        continue;
                    }
                    if (scan->token_id < 0) {
                        is_valid_template = false;
                        break;
                    }

                    const char *s_name = language->tokens[scan->token_id].name;
                    if (s_name && (strcmp(s_name, "CodeBlock") == 0 || strcmp(s_name, "ArrayIndex") == 0)) {
                        is_valid_template = false;
                        break;
                    }

                    if (textparser_token_in_id_list(tpl->template_open_tokens, scan->token_id)) {
                        depth++;
                    } else if (textparser_token_in_id_list(tpl->template_close_tokens, scan->token_id)) {
                        depth--;
                        if (depth == 0) {
                            end_bracket = scan;
                            break;
                        }
                    } else if (tpl->valid_inner_tokens && !textparser_token_in_id_list(tpl->valid_inner_tokens, scan->token_id)) {
                        is_valid_template = false;
                        break;
                    }

                    scan = scan->next;
                }

                if (depth == 0 && end_bracket != nullptr && is_valid_template && tpl->template_group_token_id >= 0) {
                    textparser_token_item *grp = calloc(1, sizeof(textparser_token_item));
                    if (grp != nullptr) {
                        grp->token_id = tpl->template_group_token_id;
                        grp->text_color = language->tokens[grp->token_id].text_color;
                        grp->text_flags = language->tokens[grp->token_id].text_flags | 0x80000000;
                        grp->parent = curr->parent;
                        grp->prev = curr->prev;
                        grp->next = end_bracket->next;

                        if (curr->prev) {
                            curr->prev->next = grp;
                        } else if (curr->parent) {
                            curr->parent->child = grp;
                        } else if (root) {
                            *root = grp;
                        }

                        if (end_bracket->next) {
                            end_bracket->next->prev = grp;
                        }

                        grp->child = curr;
                        curr->prev = nullptr;
                        end_bracket->next = nullptr;

                        size_t total_len = 0;
                        for (textparser_token_item *c = grp->child; c != nullptr; c = c->next) {
                            c->parent = grp;
                            total_len += c->len;
                        }
                        grp->len = total_len;

                        next_item = grp->next;
                        curr = grp;
                    }
                }
            }
        }
        curr = next_item;
    }
}

void textparser_post_process(textparser_token_item **root, const textparser_language_definition *language)
{
    if (root == nullptr || *root == nullptr || language == nullptr) return;

    /* Mark the subtree as being in AST mode. The final head is re-marked at the
       end because delete_if_only_one_child may replace *root. */
    (*root)->node_flags |= TEXTPARSER_NODE_POST_PROCESSED;

    /* First recursively process child subtrees */
    for (textparser_token_item *c = *root; c != nullptr; c = c->next) {
        if (c->child) {
            textparser_post_process(&c->child, language);
        }
    }

    /* Apply Type Cast disambiguation if configured */
    if (language->cast_disambiguation != nullptr) {
        textparser_disambiguate_casts(root, language);
    }

    /* Reclassify declaration-position identifiers in function parameter lists. */
    if (language->declaration_disambiguation != nullptr) {
        textparser_disambiguate_declarations(root, language);
    }

    /* Apply Template / Generics disambiguation if configured */
    if (language->template_disambiguation != nullptr) {
        textparser_disambiguate_templates(root, language);
    }

    /* Unwrap operator groups before Pratt expression parsing so all operators are direct siblings */
    textparser_token_item *it = *root;
    while (it) {
        textparser_token_item *next_sibling = it->next;
        if (is_operator_group(it, language) && it->child) {
            textparser_token_item *last_c = it->child;
            while (last_c->next) last_c = last_c->next;
            unwrap_node(root, it);
            it = last_c;
        }
        it = next_sibling;
    }

    /* Apply operator precedence Pratt parsing to current sibling list if applicable */
    if (language->operator_precedence != nullptr && language->operator_precedence->count > 0) {
        textparser_post_process_expressions(root, language);
    }

    textparser_token_item *curr = *root;

    while (curr) {
        textparser_token_item *next_sibling = curr->next;

        /* Check if this node has delete_if_only_one_child condition */
        if (curr->token_id >= 0 && language->tokens[curr->token_id].delete_if_only_one_child &&
            curr->child && textparser_get_semantic_children_count(curr) == 1) {
            textparser_token_item *semantic_child = curr->child;
            while (semantic_child && is_trivia_token_id(semantic_child->token_id)) {
                semantic_child = semantic_child->next;
            }
            if (semantic_child != nullptr) {
                semantic_child->parent = curr->parent;
                semantic_child->prev = curr->prev;
                semantic_child->next = curr->next;

                if (curr->prev) {
                    curr->prev->next = semantic_child;
                } else if (curr->parent) {
                    curr->parent->child = semantic_child;
                } else {
                    *root = semantic_child;
                }

                if (next_sibling) {
                    next_sibling->prev = semantic_child;
                }

                curr = semantic_child;
            }
        }

        curr = next_sibling;
    }

    if (*root != nullptr) {
        (*root)->node_flags |= TEXTPARSER_NODE_POST_PROCESSED;
    }
}

/**
 * Return the first error-severity diagnostic recorded by the grammar engine,
 * or NULL when none exists. Legacy parse errors are stored directly on the
 * handle; V2 (grammar) errors live in the diagnostic list, so the public error
 * accessors fall back to it.
 *
 * @param handle Parser handle.
 * @return Pointer to the first error diagnostic, or NULL.
 */
static const textparser_diagnostic *textparser_first_error_diagnostic(const struct textparser_handle *handle)
{
    if (handle == nullptr || handle->diagnostics == nullptr)
        return nullptr;
    for (size_t i = 0; i < handle->diagnostic_count; i++) {
        if (handle->diagnostics[i].severity == TEXTPARSER_SEVERITY_ERROR)
            return &handle->diagnostics[i];
    }
    return nullptr;
}

EXPORT_TEXTPARSER const char *textparser_parse_error(textparser_t handle)
{
    if (handle == nullptr)
        return nullptr;
    if (handle->error != nullptr)
        return handle->error;
    const textparser_diagnostic *diag = textparser_first_error_diagnostic(handle);
    return diag ? diag->message : nullptr;
}

EXPORT_TEXTPARSER size_t textparser_parse_error_position(textparser_t handle)
{
    if (handle == nullptr)
        return 0;
    if (handle->error != nullptr)
        return handle->error_offset;
    const textparser_diagnostic *diag = textparser_first_error_diagnostic(handle);
    return diag ? diag->start_pos : 0;
}

EXPORT_TEXTPARSER size_t textparser_parse_error_length(textparser_t handle)
{
    if (handle == nullptr)
        return 0;
    if (handle->error != nullptr)
        return handle->error_length;
    const textparser_diagnostic *diag = textparser_first_error_diagnostic(handle);
    return diag ? diag->length : 0;
}

const char *textparser_strerror(int error_code)
{
    switch (error_code) {
    case TEXTPARSER_OK:
        return "Success";
    case TEXTPARSER_ERROR_FILE_OPEN:
        return "Failed to open or map file";
    case TEXTPARSER_ERROR_INVALID_ARGUMENT:
        return "Invalid argument";
    case TEXTPARSER_ERROR_OUT_OF_MEMORY:
        return "Out of memory";
    case TEXTPARSER_ERROR_PARSE_FAILED:
        return "Parsing failed";
    case TEXTPARSER_ERROR_UNSUPPORTED_BOM:
        return "Unsupported byte order mark (BOM)";
    case TEXTPARSER_ERROR_BYTE_ORDER_CONVERSION:
        return "Byte order conversion failed";
    case TEXTPARSER_ERROR_INVALID_ENCODING:
        return "Invalid text encoding";
    case TEXTPARSER_ERROR_FILE_TOO_LARGE:
        return "File exceeds maximum parse size (16 MB)";
    case TEXTPARSER_ERROR_INVALID_UTF16_SIZE:
        return "Invalid UTF-16 size (not a multiple of 2 bytes)";
    case TEXTPARSER_ERROR_INVALID_UTF32_SIZE:
        return "Invalid UTF-32 size (not a multiple of 4 bytes)";
    default:
        return "Unknown error";
    }
}

void textparser_set_callback(textparser_t handle, void (*callback)(textparser_t, textparser_token_item *, enum textparser_callback_type callback_type, void *user_data), void *user_data)
{
    if (handle) {
        handle->callback = callback;
        handle->user_data = user_data;
    }
}

const char *textparser_get_text(textparser_t handle)
{
    if (handle == nullptr)
        return nullptr;

    return handle->text_addr;
}

size_t textparser_get_text_size(textparser_t handle)
{
    if (handle == nullptr)
        return 0;

    return handle->text_size;
}

textparser_token_item *textparser_get_first_token(const textparser_t handle)
{
    if (handle == nullptr)
        return nullptr;

    return handle->first_item;
}

/**
 * Encode a 32-bit Unicode code point into a UTF-8 byte array.
 *
 * @param cp Arena checkpoint watermark pointer.
 * @param out Output pointer receiving parsed result.
 * @return Number of bytes written (1 to 4), or 0 on invalid code point.
 */
static inline size_t encode_utf8_codepoint(uint32_t cp, char *out)
{
    if (cp <= 0x7F) {
        if (out) out[0] = (char)cp;
        return 1;
    } else if (cp <= 0x7FF) {
        if (out) {
            out[0] = (char)(0xC0 | (cp >> 6));
            out[1] = (char)(0x80 | (cp & 0x3F));
        }
        return 2;
    } else if (cp <= 0xFFFF) {
        if (out) {
            out[0] = (char)(0xE0 | (cp >> 12));
            out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[2] = (char)(0x80 | (cp & 0x3F));
        }
        return 3;
    } else if (cp <= 0x10FFFF) {
        if (out) {
            out[0] = (char)(0xF0 | (cp >> 18));
            out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
            out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[3] = (char)(0x80 | (cp & 0x3F));
        }
        return 4;
    }
    // Replacement character U+FFFD
    if (out) {
        out[0] = (char)0xEF;
        out[1] = (char)0xBF;
        out[2] = (char)0xBD;
    }
    return 3;
}

char *textparser_get_token_text(const textparser_t handle, const textparser_token_item *item)
{
    if ((handle == nullptr)||(item == nullptr)||(item->len <= 0))
        return nullptr;

    char *ret = nullptr;

    size_t total_units = textparser_get_total_units(handle);
    size_t item_pos = textparser_get_token_position(item);
    if (item->len > total_units || item_pos > total_units - item->len)
        return nullptr;

    if (handle->text_format == TEXTPARSER_ENCODING_UNICODE || handle->text_format == TEXTPARSER_ENCODING_UTF_16) {
        const uint16_t *src = (const uint16_t *)(handle->text_addr + textparser_get_byte_offset(handle, item_pos));
        size_t utf8_len = 0;
        for (size_t i = 0; i < item->len; i++) {
            uint32_t cp = src[i];
            if (cp >= 0xD800 && cp <= 0xDBFF && (i + 1 < item->len) && (src[i + 1] >= 0xDC00 && src[i + 1] <= 0xDFFF)) {
                cp = 0x10000 + (((src[i] - 0xD800) << 10) | (src[i + 1] - 0xDC00));
                i++;
            } else if (cp >= 0xD800 && cp <= 0xDFFF) {
                cp = 0xFFFD;
            }
            utf8_len += encode_utf8_codepoint(cp, nullptr);
        }

        ret = malloc(utf8_len + 1);
        if (ret) {
            size_t out_idx = 0;
            for (size_t i = 0; i < item->len; i++) {
                uint32_t cp = src[i];
                if (cp >= 0xD800 && cp <= 0xDBFF && (i + 1 < item->len) && (src[i + 1] >= 0xDC00 && src[i + 1] <= 0xDFFF)) {
                    cp = 0x10000 + (((src[i] - 0xD800) << 10) | (src[i + 1] - 0xDC00));
                    i++;
                } else if (cp >= 0xD800 && cp <= 0xDFFF) {
                    cp = 0xFFFD;
                }
                out_idx += encode_utf8_codepoint(cp, ret + out_idx);
            }
            ret[out_idx] = '\0';
        }
    } else if (handle->text_format == TEXTPARSER_ENCODING_UTF_32) {
        const uint32_t *src = (const uint32_t *)(handle->text_addr + textparser_get_byte_offset(handle, item_pos));
        size_t utf8_len = 0;
        for (size_t i = 0; i < item->len; i++) {
            uint32_t cp = src[i];
            if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
                cp = 0xFFFD;
            }
            utf8_len += encode_utf8_codepoint(cp, nullptr);
        }

        ret = malloc(utf8_len + 1);
        if (ret) {
            size_t out_idx = 0;
            for (size_t i = 0; i < item->len; i++) {
                uint32_t cp = src[i];
                if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
                    cp = 0xFFFD;
                }
                out_idx += encode_utf8_codepoint(cp, ret + out_idx);
            }
            ret[out_idx] = '\0';
        }
    } else {
        size_t byte_len = textparser_get_byte_len(handle, item->len);
        if (byte_len > SIZE_MAX - 1)
            return nullptr;
        ret = malloc(byte_len + 1);
        if (ret) {
            memcpy(ret, handle->text_addr + textparser_get_byte_offset(handle, item_pos), byte_len);
            ret[byte_len] = '\0';
        }
    }

    return ret;
}

uint16_t *textparser_get_token_text16(const textparser_t handle, const textparser_token_item *item)
{
    if ((handle == nullptr)||(item == nullptr)||(item->len <= 0))
        return nullptr;

    if (handle->text_format != TEXTPARSER_ENCODING_UNICODE && handle->text_format != TEXTPARSER_ENCODING_UTF_16)
        return nullptr;

    size_t total_units = textparser_get_total_units(handle);
    size_t item_pos = textparser_get_token_position(item);
    if (item->len > total_units || item_pos > total_units - item->len)
        return nullptr;

    if (item->len > (SIZE_MAX / sizeof(uint16_t)) - 1)
        return nullptr;

    uint16_t *ret = malloc((item->len + 1) * sizeof(uint16_t));
    if (ret) {
        const uint16_t *src = (const uint16_t *)(handle->text_addr + textparser_get_byte_offset(handle, item_pos));
        memcpy(ret, src, item->len * sizeof(uint16_t));
        ret[item->len] = 0;
    }
    return ret;
}

uint32_t *textparser_get_token_text32(const textparser_t handle, const textparser_token_item *item)
{
    if ((handle == nullptr)||(item == nullptr)||(item->len <= 0))
        return nullptr;

    if (handle->text_format != TEXTPARSER_ENCODING_UTF_32)
        return nullptr;

    size_t total_units = textparser_get_total_units(handle);
    size_t item_pos = textparser_get_token_position(item);
    if (item->len > total_units || item_pos > total_units - item->len)
        return nullptr;

    if (item->len > (SIZE_MAX / sizeof(uint32_t)) - 1)
        return nullptr;

    uint32_t *ret = malloc((item->len + 1) * sizeof(uint32_t));
    if (ret) {
        const uint32_t *src = (const uint32_t *)(handle->text_addr + textparser_get_byte_offset(handle, item_pos));
        memcpy(ret, src, item->len * sizeof(uint32_t));
        ret[item->len] = 0;
    }
    return ret;
}

void textparser_free_token_text(void *text)
{
    if (text) {
        free(text);
    }
}

const textparser_language_definition *textparser_get_language(const textparser_t handle)
{
    if (handle == nullptr)
        return nullptr;

    return handle->language;
}

size_t textparser_get_token_children_count(const textparser_token_item *token)
{
    if (token == nullptr)
        return 0;

    size_t ret = 0;

    const struct textparser_token_item * child = token->child;

    while (child)
    {
        ret++;
        child = child->next;
    }

    return ret;
}

const textparser_token_item *textparser_get_token_child(const textparser_token_item *token)
{
    if (token == nullptr)
        return nullptr;

    return token->child;
}

const textparser_token_item *textparser_get_token_next(const textparser_token_item *token)
{
    if (token == nullptr)
        return nullptr;

    return token->next;
}

const textparser_token_item *textparser_get_token_prev(const textparser_token_item *token)
{
    if (token == nullptr)
        return nullptr;

    return token->prev;
}

const char *textparser_get_token_type_str(const textparser_language_definition *language, const textparser_token_item *token)
{
    int token_id = 0;

    if (token == nullptr)
        return nullptr;

    if (token->token_id == TEXTPARSER_TOKEN_ID_START_DELIMITER)
        return "StartDelimiter";

    if (token->token_id == TEXTPARSER_TOKEN_ID_END_DELIMITER)
        return "EndDelimiter";

    if (token->token_id == TEXTPARSER_TOKEN_ID_WHITESPACE)
        return "Whitespace";

    if (token->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED)
        return "Unprocessed";

    if (token->token_id == TEXTPARSER_TOKEN_ID_ERROR)
        return "Error";

    if (language == nullptr)
        return nullptr;

    token_id = token->token_id;
    if (token_id < 0)
        return nullptr;

    for (int c = 0; c <= token_id; c++)
    {
        if (language->tokens[c].name == nullptr)
        {
            return nullptr;
        }

        if (c == token_id)
        {
            return language->tokens[c].name;
        }
    }

    return nullptr;
}

int textparser_get_token_type(const textparser_token_item *token)
{
    if (token == nullptr)
        return -1;

    return token->token_id;
}

size_t textparser_get_token_position(const textparser_token_item *token)
{
    if (token == nullptr)
        return 0;

    size_t offset = 0;
    const textparser_token_item *curr = token;

    while (curr != nullptr)
    {
        const textparser_token_item *sibling = curr->prev;
        while (sibling != nullptr)
        {
            offset += sibling->len;
            sibling = sibling->prev;
        }
        curr = curr->parent;
    }

    return offset;
}

size_t textparser_get_token_length(const textparser_token_item *token)
{
    if (token == nullptr)
        return 0;

    return token->len;
}

EXPORT_TEXTPARSER int textparser_get_cst_node_view(
    const textparser_t handle,
    const textparser_node *node,
    textparser_cst_node_view *out_view)
{
    if (node == nullptr || out_view == nullptr) return -1;
    const char *kind = node->cst_kind;
    if (kind == nullptr && handle != nullptr && handle->language != nullptr)
        kind = textparser_get_token_type_str(handle->language, node);
    out_view->kind = kind;
    if ((node->node_flags & TEXTPARSER_NODE_EXPLICIT_SPAN) != 0) {
        out_view->start = node->source_start;
        out_view->end = node->source_end;
    } else {
        out_view->start = textparser_get_token_position(node);
        out_view->end = out_view->start + node->len;
    }
    out_view->flags = node->node_flags;
    out_view->terminal = node->child == nullptr &&
        (node->node_flags & TEXTPARSER_NODE_MISSING) == 0;
    return kind == nullptr ? -1 : 0;
}

EXPORT_TEXTPARSER textparser_cst_category textparser_node_get_category(
    const textparser_node *node)
{
    if (node == nullptr) return TEXTPARSER_CST_UNKNOWN;
    if (node->category != TEXTPARSER_CST_UNKNOWN) return node->category;
    if (node->child == nullptr && (node->node_flags & TEXTPARSER_NODE_MISSING) == 0)
        return TEXTPARSER_CST_TOKEN;
    if (node->child != nullptr && (node->node_flags & TEXTPARSER_NODE_SYNTHETIC) == 0)
        return TEXTPARSER_CST_EXPRESSION;
    return TEXTPARSER_CST_OTHER;
}

uint32_t textparser_get_token_text_color(const textparser_token_item *token)
{
    if (token == nullptr)
        return 0;

    return token->text_color;
}

uint32_t textparser_get_token_text_background(const textparser_token_item *token)
{
    if (token == nullptr)
        return 0;

    return token->text_background;
}

uint32_t textparser_get_token_text_flags(const textparser_token_item *token)
{
    if (token == nullptr)
        return 0;

    return token->text_flags;
}

const char *textparser_get_token_error(const textparser_token_item *token)
{
    if (token == nullptr)
        return nullptr;

    return token->error;
}

/**
 * Recursively record the active node hierarchy enclosing a position into a parser state snapshot.
 *
 * @param token Pointer to token item node or lexer token snapshot.
 * @param state Parser state snapshot structure being populated.
 * @param max_units Maximum allowable unit count.
 * @param depth Current recursion or speculation nesting depth.
 */
static void textparser_parse_state_recursively_fill_internal(const textparser_token_item *token, const textparser_token_item **state, size_t max_units, int depth)
{
    if (depth >= MAX_RECURSION_DEPTH) {
        return;
    }
    while (token != nullptr)
    {
        size_t pos = textparser_get_token_position(token);
        size_t len = token->len;

        for (size_t c = 0; c < len; c++)
        {
            if (pos + c < max_units) {
                state[pos + c] = token;
            }
        }

        if (token->child != nullptr)
        {
            textparser_parse_state_recursively_fill_internal(token->child, state, max_units, depth + 1);
        }
        token = token->next;
    }
}

/**
 * Populate a parser state snapshot with the chain of open nodes at the given unit position.
 *
 * @param token Pointer to token item node or lexer token snapshot.
 * @param state Parser state snapshot structure being populated.
 * @param max_units Maximum allowable unit count.
 */
static void textparser_parse_state_recursively_fill(const textparser_token_item *token, const textparser_token_item **state, size_t max_units)
{
    textparser_parse_state_recursively_fill_internal(token, state, max_units, 0);
}

textparser_parser_state *textparser_state_new(const textparser_t handle)
{
    textparser_parser_state *ret = nullptr;
    size_t to_allocate = 0;
    size_t allocated = 0;
    size_t size = 0;

    size = textparser_get_total_units(handle);

    if (size >= MAX_PARSE_SIZE)
        return nullptr;

    allocated = (size * sizeof(const textparser_token_item *));
    to_allocate = offsetof(textparser_parser_state, state) + allocated;

    ret = malloc(to_allocate);
    if (ret)
    {
        ret->len = size;
        memset(ret->state, 0, allocated);

        textparser_parse_state_recursively_fill(handle->first_item, ret->state, size);
    }

    return ret;
}

EXPORT_TEXTPARSER textparser_parser_state *textparser_state_generate(const textparser_t handle, size_t position)
{
    if (handle == nullptr)
        return nullptr;

    size_t total = textparser_get_total_units(handle);
    if (total >= MAX_PARSE_SIZE)
        return nullptr;

    if (position > total)
        position = total;

    size_t len = position;
    size_t allocated = len * sizeof(const textparser_token_item *);
    size_t to_allocate = offsetof(textparser_parser_state, state) + allocated;

    textparser_parser_state *ret = malloc(to_allocate);
    if (ret) {
        ret->len = len;
        if (allocated > 0) {
            memset(ret->state, 0, allocated);
        }
        if (position > 0) {
            const textparser_token_item *active = find_token_at_position_internal(handle->first_item, position - 1, 0);
            if (active) {
                ret->state[position - 1] = active;
            }
        }
    }
    return ret;
}

void textparser_state_free(textparser_parser_state *state)
{
    if (state)
    {
        free(state);
    }
}

void textparser_state_cleanup(textparser_parser_state **state)
{
    if (state)
    {
        textparser_state_free(*state);
        *state = nullptr;
    }
}

int textparser_build_line_map(textparser_t handle)
{
    if (handle == nullptr)
        return -1;

    if (handle->lines) {
        free(handle->lines);
        handle->lines = nullptr;
    }
    handle->no_lines = 0;

    if (handle->text_addr == nullptr || handle->text_size == 0) {
        return 0;
    }

    size_t total_units = textparser_get_total_units(handle);
    size_t count = 0;

    for (size_t ch = 0; ch < total_units; ch++) {
        if (textparser_get_unit_at(handle, ch) == '\n') {
            count++;
        }
    }

    if (count == 0) {
        return 0;
    }

    handle->lines = malloc(sizeof(size_t) * count);
    if (handle->lines == nullptr) {
        handle->no_lines = 0;
        return -1;
    }
    handle->no_lines = count;

    size_t cur_line_pos = 0;

    for (size_t ch = 0; ch < total_units; ch++) {
        if (textparser_get_unit_at(handle, ch) == '\n') {
            handle->lines[cur_line_pos++] = ch;
        }
    }

    return 0;
}

size_t textparser_get_line_count(const textparser_t handle)
{
    if (handle == nullptr)
        return 0;

    if (handle->text_addr == nullptr || handle->text_size == 0)
        return 0;

    return handle->no_lines + 1;
}

size_t textparser_get_line_start_position(const textparser_t handle, size_t line_index)
{
    if (handle == nullptr)
        return 0;

    if (line_index == 0)
        return 0;

    if (line_index > handle->no_lines) {
        return textparser_get_total_units(handle);
    }

    return handle->lines[line_index - 1] + 1;
}

size_t textparser_get_line_number_at_position(const textparser_t handle, size_t position)
{
    if (handle == nullptr || handle->no_lines == 0 || handle->lines == nullptr)
        return 0;

    size_t low = 0;
    size_t high = handle->no_lines;

    while (low < high) {
        size_t mid = low + (high - low) / 2;
        if (handle->lines[mid] >= position) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }

    return low;
}

typedef enum {
    QUERY_COMB_NONE = 0,
    QUERY_COMB_CHILD,      // '>'
    QUERY_COMB_DESCENDANT  // ' '
} query_combinator_t;

typedef struct {
    int token_id;           // Target token_id (-1 if unknown type name, -2 if '*')
    query_combinator_t comb; // Combinator connecting this step to step on its right in AST
} query_step_t;

typedef struct {
    query_step_t *steps;
    size_t step_count;
} query_sequence_t;

typedef struct {
    query_sequence_t *sequences;
    size_t sequence_count;
    size_t sequence_capacity;
} query_selector_t;

/**
 * Resolve a token name string to its integer token ID for query selector evaluation.
 *
 * @param language Active language definition.
 * @param name Token name string.
 * @return Token ID integer if found, or -1 if unknown.
 */
static int query_get_token_id_by_name(const textparser_language_definition *language, const char *name)
{
    if (!language || !language->tokens || !name)
        return -1;

    if (strcmp(name, "*") == 0)
        return -2;

    for (int c = 0; language->tokens[c].name != nullptr; c++)
    {
        if (strcmp(language->tokens[c].name, name) == 0)
            return c;
        if (!language->case_sensitivity && strcasecmp(language->tokens[c].name, name) == 0)
            return c;
    }

    return -1;
}

/**
 * Release memory allocated for a parsed AST query selector expression.
 *
 * @param sel Pointer to query selector structure to free.
 */
static void query_free_selector(query_selector_t *sel)
{
    if (!sel) return;
    if (sel->sequences) {
        for (size_t i = 0; i < sel->sequence_count; i++) {
            if (sel->sequences[i].steps) {
                free(sel->sequences[i].steps);
            }
        }
        free(sel->sequences);
    }
    sel->sequences = nullptr;
    sel->sequence_count = 0;
    sel->sequence_capacity = 0;
}

typedef struct {
    int token_id;
    query_combinator_t comb_after;
} temp_element_t;

/**
 * Parse a single selector sequence step (token name, predicates, or combinator) from query string.
 *
 * @param language Active language definition.
 * @param seq_str Query sequence string slice.
 * @param seq_len Length of query sequence string.
 * @param out_seq Output pointer receiving parsed query sequence.
 * @return Pointer to parsed query sequence step, or NULL on syntax error.
 */
static bool query_parse_sequence(const textparser_language_definition *language, const char *seq_str, size_t seq_len, query_sequence_t *out_seq)
{
    temp_element_t elements[128];
    size_t elem_count = 0;

    const char *p = seq_str;
    const char *end = seq_str + seq_len;

    while (p < end) {
        while (p < end && isspace((unsigned char)*p)) p++;
        if (p >= end) break;

        if (*p == '>') {
            if (elem_count == 0) return false;
            elements[elem_count - 1].comb_after = QUERY_COMB_CHILD;
            p++;
            continue;
        }

        const char *name_start = p;
        while (p < end && !isspace((unsigned char)*p) && *p != '>' && *p != ',') {
            p++;
        }
        size_t name_len = p - name_start;
        if (name_len == 0) break;

        if (elem_count > 0 && elements[elem_count - 1].comb_after == QUERY_COMB_NONE) {
            elements[elem_count - 1].comb_after = QUERY_COMB_DESCENDANT;
        }

        char name_buf[256];
        if (name_len >= sizeof(name_buf)) name_len = sizeof(name_buf) - 1;
        memcpy(name_buf, name_start, name_len);
        name_buf[name_len] = '\0';

        if (elem_count >= 128) return false;

        elements[elem_count].token_id = query_get_token_id_by_name(language, name_buf);
        elements[elem_count].comb_after = QUERY_COMB_NONE;
        elem_count++;
    }

    if (elem_count == 0) return false;

    out_seq->steps = malloc(elem_count * sizeof(query_step_t));
    if (!out_seq->steps) return false;
    out_seq->step_count = elem_count;

    for (size_t k = 0; k < elem_count; k++) {
        size_t step_idx = (elem_count - 1) - k;
        out_seq->steps[step_idx].token_id = elements[k].token_id;
        if (k == elem_count - 1) {
            out_seq->steps[step_idx].comb = QUERY_COMB_NONE;
        } else {
            out_seq->steps[step_idx].comb = elements[k].comb_after;
        }
    }

    return true;
}

/**
 * Parse an entire AST query selector string into a structured query selector graph.
 *
 * @param language Active language definition.
 * @param selector Query selector expression string.
 * @param out_sel Output pointer receiving parsed query selector.
 * @return Pointer to parsed selector structure, or NULL on parse failure.
 */
static bool query_parse_selector(const textparser_language_definition *language, const char *selector, query_selector_t *out_sel)
{
    out_sel->sequences = nullptr;
    out_sel->sequence_count = 0;
    out_sel->sequence_capacity = 0;

    const char *p = selector;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        const char *seq_start = p;
        while (*p && *p != ',') p++;
        size_t seq_len = p - seq_start;

        query_sequence_t seq = {0};
        if (query_parse_sequence(language, seq_start, seq_len, &seq)) {
            if (out_sel->sequence_count >= out_sel->sequence_capacity) {
                size_t new_cap = out_sel->sequence_capacity == 0 ? 4 : out_sel->sequence_capacity * 2;
                query_sequence_t *new_seqs = realloc(out_sel->sequences, new_cap * sizeof(query_sequence_t));
                if (!new_seqs) {
                    if (seq.steps) free(seq.steps);
                    query_free_selector(out_sel);
                    return false;
                }
                out_sel->sequences = new_seqs;
                out_sel->sequence_capacity = new_cap;
            }
            out_sel->sequences[out_sel->sequence_count++] = seq;
        }

        if (*p == ',') p++;
    }

    return out_sel->sequence_count > 0;
}

/**
 * Evaluate whether a candidate AST node matches a query sequence step.
 *
 * @param candidate Candidate token item or CST node to test.
 * @param seq Query sequence step.
 * @param scope_root Root CST node of the candidate scope.
 * @return true if node matches selector sequence; false otherwise.
 */
static bool query_match_sequence(const textparser_token_item *candidate, const query_sequence_t *seq, const textparser_token_item *scope_root)
{
    if (seq->step_count == 0) return false;

    const textparser_token_item *curr = candidate;

    for (size_t i = 0; i < seq->step_count; i++) {
        const query_step_t *step = &seq->steps[i];

        if (i == 0) {
            if (step->token_id == -1) return false;
            if (step->token_id != -2 && curr->token_id != step->token_id) return false;
        } else {
            if (step->comb == QUERY_COMB_CHILD) {
                curr = curr->parent;
                if (!curr) return false;
                if (scope_root && curr == scope_root->parent) return false;
                if (step->token_id == -1) return false;
                if (step->token_id != -2 && curr->token_id != step->token_id) return false;
            } else if (step->comb == QUERY_COMB_DESCENDANT) {
                curr = curr->parent;
                bool found = false;
                while (curr) {
                    if (scope_root && curr == scope_root->parent) break;
                    if (step->token_id != -1 && (step->token_id == -2 || curr->token_id == step->token_id)) {
                        found = true;
                        break;
                    }
                    curr = curr->parent;
                }
                if (!found) return false;
            }
        }
    }
    return true;
}

/**
 * Evaluate whether an AST node candidate satisfies all steps of a query selector expression.
 *
 * @param candidate Candidate token item or CST node to test.
 * @param sel Parsed query selector.
 * @param scope_root Root CST node of the candidate scope.
 * @return true if node satisfies the query; false otherwise.
 */
static bool query_match_candidate(const textparser_token_item *candidate, const query_selector_t *sel, const textparser_token_item *scope_root)
{
    for (size_t s = 0; s < sel->sequence_count; s++) {
        if (query_match_sequence(candidate, &sel->sequences[s], scope_root)) {
            return true;
        }
    }
    return false;
}

EXPORT_TEXTPARSER const textparser_token_item **textparser_query(
    const textparser_t handle,
    const textparser_token_item *root,
    const char *selector,
    size_t *out_count
) {
    if (!out_count) return nullptr;
    *out_count = 0;

    if (!handle || !selector || strlen(selector) == 0) {
        return nullptr;
    }

    const textparser_language_definition *language = textparser_get_language(handle);
    if (!language) {
        return nullptr;
    }

    const textparser_token_item *start_node = root;
    if (!start_node) {
        start_node = textparser_get_first_token(handle);
    }
    if (!start_node) {
        return nullptr;
    }

    query_selector_t sel = {0};
    if (!query_parse_selector(language, selector, &sel) || sel.sequence_count == 0) {
        query_free_selector(&sel);
        return nullptr;
    }

    size_t capacity = 16;
    size_t count = 0;
    const textparser_token_item **results = malloc(capacity * sizeof(const textparser_token_item *));
    if (!results) {
        query_free_selector(&sel);
        return nullptr;
    }

    const textparser_token_item *curr = start_node;
    while (curr != nullptr) {
        if (query_match_candidate(curr, &sel, root)) {
            if (count >= capacity) {
                size_t new_cap = capacity * 2;
                const textparser_token_item **new_res = realloc((void *)results, new_cap * sizeof(const textparser_token_item *));
                if (!new_res) {
                    free((void *)results);
                    query_free_selector(&sel);
                    return nullptr;
                }
                results = new_res;
                capacity = new_cap;
            }
            results[count++] = curr;
        }

        if (curr->child != nullptr) {
            curr = curr->child;
        } else if (curr->next != nullptr) {
            curr = curr->next;
        } else {
            while (curr != nullptr && curr->next == nullptr) {
                curr = curr->parent;
                if (curr == start_node) {
                    curr = nullptr;
                    break;
                }
            }
            if (curr != nullptr) {
                if (curr == start_node) {
                    curr = nullptr;
                } else {
                    curr = curr->next;
                }
            }
        }
    }

    query_free_selector(&sel);

    *out_count = count;
    if (count == 0) {
        free((void *)results);
        return nullptr;
    }

    return results;
}

EXPORT_TEXTPARSER void textparser_free_query_result(const textparser_token_item **results)
{
    if (results) {
        free((void *)results);
    }
}

/**
 * Sequentially compute 1-based line and column coordinates for a byte offset in text buffer.
 *
 * @param handle Pointer to the textparser handle.
 * @param pos Starting character unit offset in text buffer.
 * @param inout_line_idx In/out pointer tracking 0-based line index in line map.
 * @param out_line Output pointer receiving 1-based line number.
 * @param out_col Output pointer receiving 1-based column number.
 */
static inline void textparser_calculate_line_col_sequential(const textparser_t handle, size_t pos, size_t *inout_line_idx, uint32_t *out_line, uint32_t *out_col)
{
    if (handle == nullptr || handle->lines == nullptr || handle->no_lines == 0) {
        *out_line = 0;
        *out_col = (uint32_t)pos;
        return;
    }

    size_t line_idx = (inout_line_idx != nullptr) ? *inout_line_idx : 0;
    if (line_idx > handle->no_lines) {
        line_idx = handle->no_lines;
    }

    // Advance if position is past current line's newline
    while (line_idx < handle->no_lines && pos > handle->lines[line_idx]) {
        line_idx++;
    }

    // Rewind if position is before previous line's newline (e.g. non-monotonic queries)
    while (line_idx > 0 && pos <= handle->lines[line_idx - 1]) {
        line_idx--;
    }

    if (inout_line_idx != nullptr) {
        *inout_line_idx = line_idx;
    }

    *out_line = (uint32_t)line_idx;
    size_t line_start = (line_idx == 0) ? 0 : (handle->lines[line_idx - 1] + 1);
    *out_col = (pos >= line_start) ? (uint32_t)(pos - line_start) : 0;
}

/**
 * Recursively traverse CST to export flat token range descriptors intersecting filter criteria.
 *
 * @param handle Pointer to the textparser handle.
 * @param node Current token item node being traversed.
 * @param node_start_pos Accumulated unit start offset of node.
 * @param filter_start_pos Inclusive start offset filter.
 * @param filter_end_pos Exclusive end offset filter.
 * @param buffer Output array of textparser_token_range entries.
 * @param max_tokens Maximum capacity of buffer.
 * @param inout_count In/out pointer tracking count of accumulated matches.
 * @param inout_line_idx In/out pointer tracking 0-based line index in line map.
 */
static void textparser_export_tokens_internal(const textparser_t handle, const textparser_token_item *node, size_t node_start_pos, size_t filter_start_pos, size_t filter_end_pos, textparser_token_range *buffer, size_t max_tokens, size_t *inout_count, size_t *inout_line_idx)
{
    if (node == nullptr || handle == nullptr || handle->language == nullptr)
        return;

    const textparser_language_definition *language = handle->language;

    const textparser_token_item *curr = node;
    size_t curr_pos = node_start_pos;

    while (curr != nullptr)
    {
        size_t curr_end = curr_pos + curr->len;

        // Skip token if entirely before filter range
        if (curr_end <= filter_start_pos) {
            curr_pos = curr_end;
            curr = curr->next;
            continue;
        }

        // Stop sibling traversal if past filter range
        if (curr_pos >= filter_end_pos) {
            break;
        }

        // If node has children, recurse into children first
        if (curr->child != nullptr) {
            textparser_export_tokens_internal(handle, curr->child, curr_pos, filter_start_pos, filter_end_pos, buffer, max_tokens, inout_count, inout_line_idx);
        } else {
            // Leaf token - export if intersects filter range
            if (curr_end > filter_start_pos && curr_pos < filter_end_pos) {
                size_t idx = *inout_count;
                if (buffer != nullptr && idx < max_tokens) {
                    textparser_token_range *range = &buffer[idx];
                    range->start_pos = curr_pos;
                    range->length = curr->len;
                    range->token_id = curr->token_id;

                    if (curr->token_id >= 0) {
                        range->text_color = language->tokens[curr->token_id].text_color;
                        range->text_background = language->tokens[curr->token_id].text_background;
                        range->text_flags = language->tokens[curr->token_id].text_flags;
                    } else {
                        // Inherit color/background/flags from parent token if available (e.g. string literals)
                        uint32_t eff_color = curr->text_color;
                        uint32_t eff_bg = curr->text_background;
                        uint32_t eff_flags = curr->text_flags;
                        const textparser_token_item *p = curr->parent;
                        while (p != nullptr && (eff_color == TEXTPARSER_NOCOLOR || eff_bg == TEXTPARSER_NOCOLOR || eff_flags == 0)) {
                            if (p->token_id >= 0) {
                                const textparser_token *pdef = &language->tokens[p->token_id];
                                if (curr->token_id == TEXTPARSER_TOKEN_ID_START_DELIMITER ||
                                    curr->token_id == TEXTPARSER_TOKEN_ID_END_DELIMITER) {
                                    if (eff_color == TEXTPARSER_NOCOLOR && pdef->delimiter_text_color != TEXTPARSER_NOCOLOR) {
                                        eff_color = pdef->delimiter_text_color;
                                    }
                                    if (eff_bg == TEXTPARSER_NOCOLOR && pdef->delimiter_text_background != TEXTPARSER_NOCOLOR) {
                                        eff_bg = pdef->delimiter_text_background;
                                    }
                                    if (eff_flags == 0 && pdef->delimiter_text_flags != 0) {
                                        eff_flags = pdef->delimiter_text_flags;
                                    }
                                }
                                if (eff_color == TEXTPARSER_NOCOLOR && pdef->text_color != TEXTPARSER_NOCOLOR) {
                                    eff_color = pdef->text_color;
                                }
                                if (eff_bg == TEXTPARSER_NOCOLOR && pdef->text_background != TEXTPARSER_NOCOLOR) {
                                    eff_bg = pdef->text_background;
                                }
                                if (eff_flags == 0 && pdef->text_flags != 0) {
                                    eff_flags = pdef->text_flags;
                                }
                                if (eff_color != TEXTPARSER_NOCOLOR) break;
                            }
                            p = p->parent;
                        }
                        range->text_color = eff_color;
                        range->text_background = eff_bg;
                        range->text_flags = eff_flags;
                    }

                    textparser_calculate_line_col_sequential(handle, curr_pos, inout_line_idx, &range->start_line, &range->start_col);
                    textparser_calculate_line_col_sequential(handle, curr_end, inout_line_idx, &range->end_line, &range->end_col);
                }
                (*inout_count)++;
            }
        }

        curr_pos = curr_end;
        curr = curr->next;
    }
}

EXPORT_TEXTPARSER int textparser_export_tokens(const textparser_t handle, textparser_token_range *buffer, size_t max_tokens, size_t *out_count)
{
    if (handle == nullptr || out_count == nullptr)
        return -1;

    if (handle->lines == nullptr && handle->text_addr != nullptr && handle->text_size > 0) {
        textparser_build_line_map(handle);
    }

    size_t count = 0;
    size_t line_idx = 0;
    size_t total_units = textparser_get_total_units(handle);

    textparser_export_tokens_internal(handle, handle->first_item, 0, 0, total_units, buffer, max_tokens, &count, &line_idx);

    *out_count = count;
    if (buffer != nullptr && count > max_tokens) {
        return -2; // Buffer too small to fit all tokens
    }

    return 0;
}

EXPORT_TEXTPARSER const textparser_lex_token *textparser_get_lexer_tokens(
    const textparser_t handle,
    size_t *out_count)
{
    if (out_count == nullptr) return nullptr;
    if (handle != nullptr) {
        textparser_snapshot_materialize_bias((struct textparser_handle *)handle);
    }
    *out_count = handle ? handle->lexer_token_count : 0;
    return handle ? handle->lexer_tokens : nullptr;
}

EXPORT_TEXTPARSER const textparser_lex_trivia *textparser_get_lexer_trivia(
    const textparser_t handle,
    size_t *out_count)
{
    if (out_count == nullptr) return nullptr;
    if (handle != nullptr) {
        textparser_snapshot_materialize_bias((struct textparser_handle *)handle);
    }
    *out_count = handle ? handle->lexer_trivia_count : 0;
    return handle ? handle->lexer_trivia : nullptr;
}

EXPORT_TEXTPARSER int textparser_export_tokens_range(const textparser_t handle, size_t start_pos, size_t end_pos, textparser_token_range *buffer, size_t max_tokens, size_t *out_count)
{
    if (handle == nullptr || out_count == nullptr)
        return -1;

    if (start_pos > end_pos)
        return -1;

    if (handle->lines == nullptr && handle->text_addr != nullptr && handle->text_size > 0) {
        textparser_build_line_map(handle);
    }

    size_t count = 0;
    size_t line_idx = 0;

    textparser_export_tokens_internal(handle, handle->first_item, 0, start_pos, end_pos, buffer, max_tokens, &count, &line_idx);

    *out_count = count;
    if (buffer != nullptr && count > max_tokens) {
        return -2;
    }

    return 0;
}

EXPORT_TEXTPARSER int textparser_export_tokens_lines(const textparser_t handle, size_t start_line, size_t end_line, textparser_token_range *buffer, size_t max_tokens, size_t *out_count)
{
    if (handle == nullptr || out_count == nullptr)
        return -1;

    if (start_line > end_line)
        return -1;

    if (handle->lines == nullptr && handle->text_addr != nullptr && handle->text_size > 0) {
        textparser_build_line_map(handle);
    }

    size_t start_pos = textparser_get_line_start_position(handle, start_line);
    size_t end_pos = (end_line + 1 < textparser_get_line_count(handle))
                         ? textparser_get_line_start_position(handle, end_line + 1)
                         : textparser_get_total_units(handle);

    return textparser_export_tokens_range(handle, start_pos, end_pos, buffer, max_tokens, out_count);
}

EXPORT_TEXTPARSER int textparser_register_handler(
    textparser_t handle,
    const char *name,
    textparser_semantic_handler handler,
    void *user_data)
{
    if (handle == nullptr || name == nullptr || handler == nullptr) {
        return -1;
    }

    textparser_handler_entry *entry = handle->handlers;
    while (entry != nullptr) {
        if (entry->name && strcmp(entry->name, name) == 0) {
            entry->handler = handler;
            entry->user_data = user_data;
            return 0;
        }
        entry = entry->next;
    }

    entry = malloc(sizeof(textparser_handler_entry));
    if (entry == nullptr) {
        return -1;
    }
    entry->name = strdup(name);
    if (entry->name == nullptr) {
        free(entry);
        return -1;
    }
    entry->handler = handler;
    entry->user_data = user_data;
    entry->next = handle->handlers;
    handle->handlers = entry;

    return 0;
}

EXPORT_TEXTPARSER textparser_action textparser_dispatch_event(
    textparser_t handle,
    const char *handler_name,
    const textparser_event *event)
{
    if (handle == nullptr || event == nullptr) {
        return TEXTPARSER_ACTION_ABORT;
    }

    if (handler_name != nullptr) {
        textparser_handler_entry *entry = handle->handlers;
        while (entry != nullptr) {
            if (entry->name && strcmp(entry->name, handler_name) == 0) {
                return entry->handler(handle, event, entry->user_data);
            }
            entry = entry->next;
        }
        return TEXTPARSER_ACTION_ACCEPT;
    }

    return TEXTPARSER_ACTION_ACCEPT;
}

/**
 * Enqueue a semantic lifecycle event for deferred dispatch upon speculative branch commit.
 *
 * @param handle Pointer to the textparser handle.
 * @param handler_name Name of registered semantic handler callback.
 * @param event Pointer to event descriptor payload.
 * @return 0 on success, or non-zero error code on failure.
 */
static int textparser_queue_event(
    textparser_t handle,
    const char *handler_name,
    const textparser_event *event)
{
    if (handle == nullptr || handler_name == nullptr || event == nullptr) return -1;
    if (handle->parser.pending_event_count == handle->pending_event_capacity) {
        size_t capacity = handle->pending_event_capacity == 0
            ? 16 : handle->pending_event_capacity * 2;
        textparser_pending_event *events = realloc(
            handle->pending_events, capacity * sizeof(*events));
        if (events == nullptr) return -1;
        handle->pending_events = events;
        handle->pending_event_capacity = capacity;
    }
    textparser_pending_event *pending =
        &handle->pending_events[handle->parser.pending_event_count++];
    pending->handler_name = handler_name;
    pending->event = *event;
    return 0;
}

/**
 * Dispatch queued semantic lifecycle events starting from a watermark after branch commit.
 *
 * @param handle Pointer to the textparser handle.
 * @param first In/out pointer to head of sibling node list.
 * @return TEXTPARSER_ACTION_ACCEPT, TEXTPARSER_ACTION_REJECT, or TEXTPARSER_ACTION_ABORT.
 */
static textparser_action textparser_publish_pending_events(
    textparser_t handle,
    size_t first)
{
    if (handle == nullptr || first > handle->parser.pending_event_count)
        return TEXTPARSER_ACTION_ABORT;
    size_t end = handle->parser.pending_event_count;
    for (size_t i = first; i < end; i++) {
        if (handle->pending_events[i].event.node != nullptr)
            handle->pending_events[i].event.parent =
                handle->pending_events[i].event.node->parent;
        textparser_action action = textparser_dispatch_event(
            handle, handle->pending_events[i].handler_name,
            &handle->pending_events[i].event);
        if (action != TEXTPARSER_ACTION_ACCEPT) {
            handle->parser.pending_event_count = first;
            return action;
        }
    }
    handle->parser.pending_event_count = first;
    return TEXTPARSER_ACTION_ACCEPT;
}

EXPORT_TEXTPARSER uint64_t textparser_node_get_id(const textparser_node *node)
{
    return node ? node->id : 0;
}

EXPORT_TEXTPARSER uint32_t textparser_node_get_flags(const textparser_node *node)
{
    return node ? node->node_flags : 0;
}

EXPORT_TEXTPARSER void textparser_node_set_flags(textparser_node *node, uint32_t flags)
{
    if (node) {
        node->node_flags = flags;
    }
}

EXPORT_TEXTPARSER void *textparser_node_get_user_data(const textparser_node *node)
{
    return node ? node->user_data : nullptr;
}

EXPORT_TEXTPARSER void textparser_node_set_user_data(
    textparser_node *node,
    void *user_data,
    void (*free_fn)(void *))
{
    if (node) {
        if (node->user_data && node->free_user_data && node->user_data != user_data) {
            node->free_user_data(node->user_data);
        }
        node->user_data = user_data;
        node->free_user_data = free_fn;
    }
}

EXPORT_TEXTPARSER const char *textparser_node_get_decoded_value(const textparser_node *node)
{
    return node ? node->decoded_value : nullptr;
}

EXPORT_TEXTPARSER void textparser_node_set_decoded_value(textparser_node *node, const char *value)
{
    if (node) {
        node->decoded_value = value;
    }
}

/* -------------------------------------------------------------------------
 * Phase 3: Lexer Modes, Goals, Decoders & Validators Implementations
 * ------------------------------------------------------------------------- */

EXPORT_TEXTPARSER int textparser_register_decoder(
    textparser_t handle,
    const char *name,
    textparser_decoder_fn decoder,
    void *user_data)
{
    if (handle == nullptr || name == nullptr || decoder == nullptr) {
        return -1;
    }

    textparser_decoder_entry *entry = handle->decoders;
    while (entry != nullptr) {
        if (entry->name && strcmp(entry->name, name) == 0) {
            entry->decoder = decoder;
            entry->user_data = user_data;
            return 0;
        }
        entry = entry->next;
    }

    entry = malloc(sizeof(textparser_decoder_entry));
    if (entry == nullptr) {
        return -1;
    }
    entry->name = strdup(name);
    if (entry->name == nullptr) {
        free(entry);
        return -1;
    }
    entry->decoder = decoder;
    entry->user_data = user_data;
    entry->next = handle->decoders;
    handle->decoders = entry;

    return 0;
}

EXPORT_TEXTPARSER int textparser_register_validator(
    textparser_t handle,
    const char *name,
    textparser_validator_fn validator,
    void *user_data)
{
    if (handle == nullptr || name == nullptr || validator == nullptr) {
        return -1;
    }

    textparser_validator_entry *entry = handle->validators;
    while (entry != nullptr) {
        if (entry->name && strcmp(entry->name, name) == 0) {
            entry->validator = validator;
            entry->user_data = user_data;
            return 0;
        }
        entry = entry->next;
    }

    entry = malloc(sizeof(textparser_validator_entry));
    if (entry == nullptr) {
        return -1;
    }
    entry->name = strdup(name);
    if (entry->name == nullptr) {
        free(entry);
        return -1;
    }
    entry->validator = validator;
    entry->user_data = user_data;
    entry->next = handle->validators;
    handle->validators = entry;

    return 0;
}

EXPORT_TEXTPARSER char *textparser_decode_token(
    textparser_t handle,
    const char *decoder_name,
    const char *raw_text,
    size_t length)
{
    if (handle == nullptr || decoder_name == nullptr || raw_text == nullptr) {
        return nullptr;
    }

    textparser_decoder_entry *entry = handle->decoders;
    while (entry != nullptr) {
        if (entry->name && strcmp(entry->name, decoder_name) == 0) {
            return entry->decoder(handle, raw_text, length, entry->user_data);
        }
        entry = entry->next;
    }

    return nullptr;
}

EXPORT_TEXTPARSER bool textparser_validate_token(
    textparser_t handle,
    const char *validator_name,
    const char *raw_text,
    size_t length,
    const char **out_error)
{
    if (handle == nullptr || validator_name == nullptr || raw_text == nullptr) {
        if (out_error) *out_error = "Invalid arguments to validator";
        return false;
    }

    textparser_validator_entry *entry = handle->validators;
    while (entry != nullptr) {
        if (entry->name && strcmp(entry->name, validator_name) == 0) {
            return entry->validator(handle, raw_text, length, out_error, entry->user_data);
        }
        entry = entry->next;
    }

    return true; // Unknown validator defaults to accept
}

EXPORT_TEXTPARSER bool textparser_regex_match_pattern(
    textparser_t handle,
    const char *pattern,
    const char *text,
    size_t length,
    bool only_at_start,
    size_t *out_offset,
    size_t *out_length)
{
    if (handle == nullptr || pattern == nullptr || text == nullptr) return false;
    if (handle->regex_ctx == nullptr) {
        handle->regex_ctx = adv_regex_context_create();
        if (handle->regex_ctx == nullptr) return false;
    }
    void *regex = nullptr;
    size_t found_at = 0, found_len = 0;
    bool matched = adv_regex_find_pattern_ctx(
        handle->regex_ctx, pattern, &regex, TEXTPARSER_ENCODING_UTF_8,
        text, length, &found_at, &found_len, false, only_at_start);
    adv_regex_free(handle->regex_ctx, &regex, TEXTPARSER_ENCODING_UTF_8);
    if (!matched) return false;
    if (out_offset != nullptr) *out_offset = found_at;
    if (out_length != nullptr) *out_length = found_len;
    return true;
}

EXPORT_TEXTPARSER int textparser_register_operand_validator(
    textparser_t handle,
    const char *name,
    textparser_operand_validator_fn validator,
    void *user_data)
{
    if (handle == nullptr || name == nullptr || validator == nullptr) {
        return -1;
    }

    textparser_operand_validator_entry *entry = handle->operand_validators;
    while (entry != nullptr) {
        if (entry->name && strcmp(entry->name, name) == 0) {
            entry->validator = validator;
            entry->user_data = user_data;
            return 0;
        }
        entry = entry->next;
    }

    entry = malloc(sizeof(textparser_operand_validator_entry));
    if (entry == nullptr) {
        return -1;
    }
    entry->name = strdup(name);
    if (entry->name == nullptr) {
        free(entry);
        return -1;
    }
    entry->validator = validator;
    entry->user_data = user_data;
    entry->next = handle->operand_validators;
    handle->operand_validators = entry;

    return 0;
}

EXPORT_TEXTPARSER int textparser_push_mode(textparser_t handle, const char *mode_name)
{
    if (handle == nullptr || mode_name == nullptr) {
        return -1;
    }
    if (handle->mode_stack_depth >= TEXTPARSER_MAX_MODE_STACK) {
        return -1;
    }

    char *mode_copy = strdup(mode_name);
    if (mode_copy == nullptr) {
        return -1;
    }

    handle->mode_stack[handle->mode_stack_depth++] = mode_copy;
    return 0;
}

EXPORT_TEXTPARSER int textparser_pop_mode(textparser_t handle)
{
    if (handle == nullptr || handle->mode_stack_depth == 0) {
        return -1;
    }

    handle->mode_stack_depth--;
    if (handle->mode_stack[handle->mode_stack_depth]) {
        free(handle->mode_stack[handle->mode_stack_depth]);
        handle->mode_stack[handle->mode_stack_depth] = nullptr;
    }
    return 0;
}

EXPORT_TEXTPARSER const char *textparser_get_current_mode(textparser_t handle)
{
    if (handle == nullptr) {
        return "default";
    }
    if (handle->mode_stack_depth > 0 && handle->mode_stack[handle->mode_stack_depth - 1] != nullptr) {
        return handle->mode_stack[handle->mode_stack_depth - 1];
    }
    /* With an empty stack the active mode is the definition's initial mode, so
     * consume and peek agree. */
    if (handle->language != nullptr && handle->language->initial_lexer_mode != nullptr) {
        return handle->language->initial_lexer_mode;
    }
    return "default";
}

EXPORT_TEXTPARSER void textparser_set_lexical_goal(textparser_t handle, const char *goal_name)
{
    if (handle == nullptr) return;

    if (handle->lexical_goal) {
        free(handle->lexical_goal);
        handle->lexical_goal = nullptr;
    }
    if (goal_name) {
        handle->lexical_goal = strdup(goal_name);
    }
}

EXPORT_TEXTPARSER const char *textparser_get_lexical_goal(textparser_t handle)
{
    return handle ? handle->lexical_goal : nullptr;
}

/**
 * Look up a named lexer mode definition in the active language definition.
 *
 * @param language Active language definition.
 * @param name Name string of token, production, or context.
 * @return Pointer to textparser_lexer_mode if found, or NULL if not defined.
 */
static const textparser_lexer_mode *textparser_find_lexer_mode(
    const textparser_language_definition *language,
    const char *name)
{
    if (language == nullptr || name == nullptr) return nullptr;
    for (size_t i = 0; i < language->lexer_mode_count; i++) {
        if (strcmp(language->lexer_modes[i].name, name) == 0) return &language->lexer_modes[i];
    }
    return nullptr;
}

/**
 * Remap a source token ID according to the active contextual lexical goal mappings.
 *
 * @param language Active language definition.
 * @param goal Active lexical goal name string.
 * @param token_id Integer ID of the token.
 * @param goal_id Integer ID of the active lexical goal.
 * @return Remapped token ID if a mapping matches, or original source_token.
 */
static int textparser_goal_token(
    const textparser_language_definition *language,
    const char *goal,
    int token_id,
    int *goal_id)
{
    if (goal_id != nullptr) *goal_id = 0;
    if (language == nullptr || goal == nullptr) return token_id;
    for (size_t i = 0; i < language->lexer_goal_count; i++) {
        const textparser_lexer_goal *item = &language->lexer_goals[i];
        if (strcmp(item->name, goal) != 0) continue;
        if (goal_id != nullptr) *goal_id = (int)i + 1;
        for (size_t m = 0; m < item->mapping_count; m++) {
            if (item->mappings[m].source_token == token_id) return item->mappings[m].target_token;
        }
        return token_id;
    }
    return token_id;
}

/**
 * Check whether an integer ID is present in a null-terminated or TextParser_END-terminated array.
 *
 * @param ids Array of integer IDs.
 * @param id Identifier integer.
 * @return true if found in list; false otherwise.
 */
static bool textparser_id_in_list(const int *ids, int id)
{
    if (ids == nullptr) return false;
    for (size_t i = 0; ids[i] >= 0; i++) if (ids[i] == id) return true;
    return false;
}

/**
 * Match a single candidate contextual lexer rule against the input stream at source_offset.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Token ID of candidate lexer rule.
 * @param offset Output pointer receiving matched offset relative to pos.
 * @param length Length in character code units or bytes.
 * @return true if candidate rule matched; false otherwise.
 */
static bool textparser_contextual_match(
    struct textparser_handle *handle,
    int token_id,
    size_t offset,
    size_t *length)
{
    const textparser_contextual_lexer_rule *lexer_rule = handle->language->lexer_rules != nullptr
        ? &handle->language->lexer_rules[token_id] : nullptr;
    if (lexer_rule != nullptr && lexer_rule->dynamic_trigger > 0) {
        int slot = lexer_rule->dynamic_trigger;
        if (slot >= TEXTPARSER_MAX_LEXER_CAPTURES ||
            handle->lexer_captures[slot].count == 0)
            return false;
    }
    if (lexer_rule != nullptr && lexer_rule->dynamic > 0) {
        int slot = lexer_rule->dynamic;
        if (slot >= TEXTPARSER_MAX_LEXER_CAPTURES) return false;
        const textparser_lexer_capture_queue *queue = &handle->lexer_captures[slot];
        if (queue->count == 0) return false;
        const textparser_lexer_capture *capture = &queue->items[0];
        if (capture->bytes == nullptr || capture->unit_length == 0) return false;
        size_t total = textparser_get_total_units(handle);
        if (offset >= total) return false;
        size_t start = offset;
        if (capture->strip_tabs) {
            while (start < total && textparser_get_unit_at(handle, start) == '\t') start++;
        }
        if (capture->unit_length > total - start) return false;
        size_t byte_offset = textparser_get_byte_offset(handle, start);
        if (byte_offset + capture->byte_length > handle->text_size) return false;
        if (memcmp(handle->text_addr + byte_offset, capture->bytes, capture->byte_length) != 0)
            return false;
        /* The dynamic token must occupy a whole line (here-doc delimiter) and
         * consumes the line terminator so it ties the line-based body token. */
        size_t after = start + capture->unit_length;
        size_t match_length = after - offset;
        if (after < total) {
            uint32_t ch = textparser_get_unit_at(handle, after);
            if (ch != '\n' && ch != '\r' && ch != 0x2028 && ch != 0x2029) return false;
            match_length++;
            after++;
            if (ch == '\r' && after < total && textparser_get_unit_at(handle, after) == '\n')
                match_length++;
        }
        *length = match_length;
        return true;
    }
    const textparser_token *rule = &handle->language->tokens[token_id];
    if (rule->name != nullptr && strcmp(rule->name, "Hashbang") == 0 && offset != 0)
        return false;
    if (rule->start_regex == nullptr && rule->startRegexFunction == nullptr) return false;
    size_t found_at = 0;
    size_t found_len = 0;
    size_t total = textparser_get_total_units(handle);
    if (offset >= total || !textparser_match_start_token(
            handle, token_id,
            handle->text_addr + textparser_get_byte_offset(handle, offset),
            total - offset, &found_at, &found_len, true, true) || found_at != 0 || found_len == 0) {
        return false;
    }
    *length = found_len;
    if (handle->language->lexer_rules != nullptr &&
        handle->language->lexer_rules[token_id].validator != nullptr) {
        const char *validator_name = handle->language->lexer_rules[token_id].validator;
        const char *raw_text = handle->text_addr + textparser_get_byte_offset(handle, offset);
        const char *err_msg = nullptr;
        if (!textparser_validate_token(handle, validator_name, raw_text, found_len, &err_msg))
            return false;
    }
    return true;
}

/**
 * Record a dynamic lexer capture (e.g. a here-doc delimiter) for a matched token.
 *
 * @param handle Pointer to the textparser handle.
 * @param token_id Matched token id whose lexer rule declares `capture`.
 * @param offset Unit offset where the token matched.
 */
static void textparser_contextual_capture(
    struct textparser_handle *handle,
    int token_id,
    size_t offset)
{
    if (handle->language->lexer_rules == nullptr) return;
    const textparser_contextual_lexer_rule *lexer_rule = &handle->language->lexer_rules[token_id];
    int slot = lexer_rule->capture;
    if (slot <= 0 || slot >= TEXTPARSER_MAX_LEXER_CAPTURES) return;
    const textparser_token *token_def = &handle->language->tokens[token_id];
    if (token_def->start_regex == nullptr) return;

    size_t found_at = 0, found_len = 0, capture_at = 0, capture_len = 0;
    size_t total = textparser_get_total_units(handle);
    if (!adv_regex_find_pattern_capture_ctx(
            handle->regex_ctx, token_def->start_regex,
            (void **)handle->start_regex + token_id, handle->text_format,
            handle->text_addr + textparser_get_byte_offset(handle, offset), total - offset,
            &found_at, &found_len, !handle->language->case_sensitivity, true, true,
            slot, &capture_at, &capture_len) ||
        capture_len == 0) {
        return;
    }
    bool strip_tabs = false;
    if (lexer_rule->capture_flag > 0) {
        size_t flag_at = 0, flag_len = 0;
        if (adv_regex_find_pattern_capture_ctx(
                handle->regex_ctx, token_def->start_regex,
                (void **)handle->start_regex + token_id, handle->text_format,
                handle->text_addr + textparser_get_byte_offset(handle, offset), total - offset,
                &found_at, &found_len, !handle->language->case_sensitivity, true, true,
                lexer_rule->capture_flag, &flag_at, &flag_len) && flag_len > 0) {
            strip_tabs = true;
        }
    }
    size_t byte_start = textparser_get_byte_offset(handle, offset + capture_at);
    size_t byte_end = textparser_get_byte_offset(handle, offset + capture_at + capture_len);
    size_t byte_length = byte_end - byte_start;
    char *bytes = malloc(byte_length + 1);
    if (bytes == nullptr) return;
    memcpy(bytes, handle->text_addr + byte_start, byte_length);
    bytes[byte_length] = '\0';

    textparser_lexer_capture_queue *queue = &handle->lexer_captures[slot];
    if (queue->count == queue->capacity) {
        size_t capacity = queue->capacity ? queue->capacity * 2 : 4;
        textparser_lexer_capture *items = realloc(queue->items, capacity * sizeof(*items));
        if (items == nullptr) { free(bytes); return; }
        queue->items = items;
        queue->capacity = capacity;
    }
    queue->items[queue->count++] = (textparser_lexer_capture){
        bytes, byte_length, capture_len, strip_tabs};
}

/* Consume the front of a dynamic capture queue once its token is selected. */
static void textparser_contextual_consume_capture(struct textparser_handle *handle, int token_id) {
    if (handle->language->lexer_rules == nullptr) return;
    int slot = handle->language->lexer_rules[token_id].dynamic;
    if (slot <= 0 || slot >= TEXTPARSER_MAX_LEXER_CAPTURES) return;
    textparser_lexer_capture_queue *queue = &handle->lexer_captures[slot];
    if (queue->count == 0) return;
    free(queue->items[0].bytes);
    memmove(&queue->items[0], &queue->items[1], (queue->count - 1) * sizeof(*queue->items));
    queue->count--;
}

/* A dynamic token keeps its mode while more queued captures remain (bash reads
 * queued here-doc bodies in order). */
static bool textparser_rule_should_pop(struct textparser_handle *handle,
                                       const textparser_contextual_lexer_rule *rule) {
    if (!rule->pop_mode) return false;
    if (rule->dynamic > 0 && rule->dynamic < TEXTPARSER_MAX_LEXER_CAPTURES &&
        handle->lexer_captures[rule->dynamic].count > 0)
        return false;
    return true;
}

/**
 * Scan the next syntax token from source_offset using active mode, goal, and priority rules.
 *
 * @param handle Pointer to the textparser handle.
 * @param source_offset Unit offset in input stream.
 * @param mode_name Active lexer mode name.
 * @param goal_name Active contextual lexical goal name.
 * @param out_token Output pointer receiving populated lexer token snapshot.
 * @param out_source_rule Output pointer receiving matched rule index.
 * @return 0 on success, 1 on EOF, or negative error code on scanning failure.
 */
static int textparser_contextual_scan_one(
    struct textparser_handle *handle,
    size_t source_offset,
    const char *mode_name,
    const char *goal_name,
    const textparser_lex_token **out_token,
    int *out_source_rule,
    textparser_cached_trivia **out_trivia,
    size_t *out_trivia_count)
{
    *out_token = nullptr;
    if (out_source_rule != nullptr) *out_source_rule = -1;
    if (out_trivia != nullptr) *out_trivia = nullptr;
    if (out_trivia_count != nullptr) *out_trivia_count = 0;
    const char *mode = mode_name ? mode_name : "default";
    const char *goal = goal_name ? goal_name : "";
    for (textparser_lexer_cache_entry *cached = handle->lexer_cache;
         cached != nullptr; cached = cached->next) {
        if (cached->source_offset == source_offset && strcmp(cached->mode, mode) == 0 &&
            strcmp(cached->goal, goal) == 0) {
            *out_token = &cached->token;
            if (out_source_rule != nullptr) *out_source_rule = cached->source_rule;
            return 0;
        }
    }

    const textparser_lexer_mode *active = textparser_find_lexer_mode(handle->language, mode);
    if (handle->language->lexer_mode_count != 0 && active == nullptr) return -1;
    size_t total = textparser_get_total_units(handle);
    size_t offset = source_offset;
    size_t trivia_start = offset;
    uint32_t flags = 0;
    textparser_cached_trivia *trivia = nullptr;
    size_t trivia_count = 0;
    size_t trivia_capacity = 0;
    for (;;) {
        int best = -1;
        size_t best_len = 0;
        for (int id = 0; id < (int)handle->token_count; id++) {
            if (!handle->language->lexer_rules[id].is_trivia ||
                (active != nullptr && !textparser_id_in_list(active->trivia, id))) continue;
            size_t length = 0;
            if (textparser_contextual_match(handle, id, offset, &length) &&
                (length > best_len || (length == best_len && best >= 0 &&
                 handle->language->lexer_rules[id].priority > handle->language->lexer_rules[best].priority))) {
                best = id;
                best_len = length;
            }
        }
        if (best < 0) break;
        if (trivia_count == trivia_capacity) {
            size_t capacity = trivia_capacity ? trivia_capacity * 2 : 8;
            textparser_cached_trivia *grown = realloc(trivia, capacity * sizeof(*grown));
            if (grown == nullptr) { free(trivia); return -1; }
            trivia = grown;
            trivia_capacity = capacity;
        }
        trivia[trivia_count++] = (textparser_cached_trivia){best, offset, offset + best_len};
        flags |= textparser_lexer_span_flags(handle, offset, offset + best_len);
        offset += best_len;
    }

    /* Hand the collected trivia to the caller (ownership transfers), or drop it
     * when the caller only needs the next token. */
    if (out_trivia != nullptr) { *out_trivia = trivia; *out_trivia_count = trivia_count; }
    else free(trivia);

    if (offset >= total) return 1;

    int best = -1;
    int best_source = -1;
    size_t best_len = 0;
    int goal_id = 0;
    for (int source_id = 0; source_id < (int)handle->token_count; source_id++) {
        if (handle->language->lexer_rules[source_id].is_trivia ||
            (active != nullptr && !textparser_id_in_list(active->tokens, source_id))) continue;
        int id = textparser_goal_token(handle->language, goal_name, source_id, &goal_id);
        size_t length = 0;
        if (textparser_contextual_match(handle, id, offset, &length) &&
            (length > best_len || (length == best_len && best >= 0 &&
             handle->language->lexer_rules[id].priority > handle->language->lexer_rules[best].priority))) {
            best = id;
            best_source = source_id;
            best_len = length;
        }
    }
    if (best < 0) return 1;

    /* Store any dynamic capture (e.g. a here-doc delimiter) declared by the rule.
     * Capture from the token start (after leading trivia). */
    textparser_contextual_capture(handle, best_source, offset);
    textparser_contextual_consume_capture(handle, best_source);

    textparser_lexer_cache_entry *entry = calloc(1, sizeof(*entry));
    if (entry == nullptr) return -1;
    entry->mode = strdup(mode);
    entry->goal = strdup(goal);
    if (entry->mode == nullptr || entry->goal == nullptr) {
        free(entry->mode); free(entry->goal); free(entry); return -1;
    }
    entry->source_offset = source_offset;
    entry->source_rule = best_source;
    entry->token.kind = best;
    entry->token.start = offset;
    entry->token.end = offset + best_len;
    entry->token.leading_trivia_start = trivia_start;
    entry->token.leading_trivia_count = offset - trivia_start;
    entry->token.mode = active == nullptr ? 0 : (int)(active - handle->language->lexer_modes) + 1;
    entry->token.lexical_goal = goal_id;
    entry->token.flags = flags;
    entry->next = handle->lexer_cache;
    handle->lexer_cache = entry;
    *out_token = &entry->token;
    if (out_source_rule != nullptr) *out_source_rule = best_source;
    return 0;
}

EXPORT_TEXTPARSER int textparser_lexer_peek(
    textparser_t handle, size_t lookahead, const char *goal_name,
    const textparser_lex_token **out_token)
{
    if (handle == nullptr || handle->language == nullptr || out_token == nullptr) return -1;
    const char *modes[TEXTPARSER_MAX_MODE_STACK] = {0};
    size_t depth = handle->mode_stack_depth;
    for (size_t i = 0; i < depth; i++) modes[i] = handle->mode_stack[i];
    size_t offset = handle->parser.source_offset;
    const textparser_lex_token *token = nullptr;
    int source_rule = -1;
    for (size_t i = 0; i <= lookahead; i++) {
        const char *mode = depth ? modes[depth - 1] :
            (handle->language->initial_lexer_mode ? handle->language->initial_lexer_mode : "default");
        int ret = textparser_contextual_scan_one(
            handle, offset, mode, goal_name, &token, &source_rule, nullptr, nullptr);
        if (ret != 0) { *out_token = nullptr; return ret; }
        if (i == lookahead) break;
        const textparser_contextual_lexer_rule *source = source_rule >= 0
            ? &handle->language->lexer_rules[source_rule] : nullptr;
        const textparser_contextual_lexer_rule *rule = source != nullptr &&
            (source->pop_mode || source->push_mode != nullptr)
            ? source : &handle->language->lexer_rules[token->kind];
        if (rule->pop_mode && depth > 0) depth--;
        if (rule->push_mode != nullptr && depth < TEXTPARSER_MAX_MODE_STACK) modes[depth++] = rule->push_mode;
        offset = token->end;
    }
    *out_token = token;
    return 0;
}

EXPORT_TEXTPARSER int textparser_lexer_consume(
    textparser_t handle, const char *goal_name, const textparser_lex_token **out_token)
{
    int scan = textparser_lexer_peek(handle, 0, goal_name, out_token);
    if (scan != 0) return scan;
    int source_rule = -1;
    const char *mode = textparser_get_current_mode(handle);
    if (textparser_contextual_scan_one(
            handle, handle->parser.source_offset, mode, goal_name, out_token, &source_rule, nullptr, nullptr) != 0)
        return -1;
    const textparser_contextual_lexer_rule *source = source_rule >= 0
        ? &handle->language->lexer_rules[source_rule] : nullptr;
    const textparser_contextual_lexer_rule *rule = source != nullptr &&
        (source->pop_mode || source->push_mode != nullptr)
        ? source : &handle->language->lexer_rules[(*out_token)->kind];
    if (textparser_rule_should_pop(handle, rule) && textparser_pop_mode(handle) != 0) return -1;
    if (rule->push_mode != nullptr && textparser_push_mode(handle, rule->push_mode) != 0) return -1;
    handle->parser.previous_token = **out_token;
    handle->parser.has_previous_token = true;
    handle->parser.source_offset = (*out_token)->end;
    handle->parser.token_index++;
    const char *token_name = handle->language->tokens[(*out_token)->kind].name;
    if (token_name != nullptr &&
        (strncmp(token_name, "Invalid", 7) == 0 ||
         strncmp(token_name, "Unterminated", 12) == 0)) {
        const char *message = strncmp(token_name, "Unterminated", 12) == 0
            ? "Unterminated literal." : "Invalid lexical token.";
        if (textparser_report_diagnostic(
                handle, TEXTPARSER_SEVERITY_ERROR, "TS_LEXICAL", message,
                (*out_token)->start, (*out_token)->end - (*out_token)->start) != 0)
            return -1;
    }
    return 0;
}

/* Append a flat sibling node to the contextual parse tree. */
static void textparser_contextual_append(textparser_token_item **first,
                                         textparser_token_item **last,
                                         textparser_token_item *node) {
    if (*first == nullptr) *first = node;
    else (*last)->next = node;
    node->prev = *last;
    *last = node;
}

/*
 * Tokenize a v2 (contextual lexer) definition with mode/priority awareness and
 * build a flat top-level token tree plus the immutable lexer snapshots. The
 * legacy scanner cannot be used here because it ignores lexer modes and token
 * priorities; see BUGS.md.
 */
static int textparser_parse_contextual(struct textparser_handle *handle,
                                       const textparser_language_definition *definition) {
    if (handle->first_item != nullptr) {
        unwrap_post_processed_tokens(&handle->first_item);
        handle->first_item = nullptr;
        free_arena(handle);
    }
    textparser_clear_lexer_cache(handle);
    textparser_reset_lexical_state(handle);
    handle->parser.source_offset = 0;
    handle->parser.token_index = 0;
    handle->parser.has_previous_token = false;

    textparser_token_item *first = nullptr;
    textparser_token_item *last = nullptr;
    size_t total = textparser_get_total_units(handle);
    while (handle->parser.source_offset < total) {
        size_t pos = handle->parser.source_offset;
        const char *mode = textparser_get_current_mode(handle);
        const textparser_lex_token *token = nullptr;
        int source_rule = -1;
        textparser_cached_trivia *trivia = nullptr;
        size_t trivia_count = 0;
        int rc = textparser_contextual_scan_one(
            handle, pos, mode, nullptr, &token, &source_rule, &trivia, &trivia_count);

        for (size_t i = 0; i < trivia_count; ++i) {
            const textparser_cached_trivia *item = &trivia[i];
            const char *name = definition->tokens[item->kind].name;
            int kind = (name != nullptr && strstr(name, "Whitespace") != nullptr)
                ? TEXTPARSER_TOKEN_ID_WHITESPACE : item->kind;
            textparser_token_item *node =
                textparser_alloc_token(handle, kind, item->end - item->start);
            if (node == nullptr) { free(trivia); return TEXTPARSER_ERROR_OUT_OF_MEMORY; }
            node->node_flags |= TEXTPARSER_NODE_TRIVIA;
            textparser_contextual_append(&first, &last, node);
        }
        size_t trivia_end = trivia_count > 0 ? trivia[trivia_count - 1].end : pos;
        free(trivia);

        if (token == nullptr || rc != 0) {
            /* EOF, or unlexable input after any leading trivia. */
            if (trivia_end >= total) break;
            size_t char_len = textparser_char_len(handle, trivia_end);
            textparser_token_item *node =
                textparser_alloc_token(handle, TEXTPARSER_TOKEN_ID_UNPROCESSED, char_len);
            if (node == nullptr) return TEXTPARSER_ERROR_OUT_OF_MEMORY;
            textparser_contextual_append(&first, &last, node);
            handle->parser.source_offset = trivia_end + char_len;
            continue;
        }

        textparser_token_item *node =
            textparser_alloc_token(handle, token->kind, token->end - token->start);
        if (node == nullptr) return TEXTPARSER_ERROR_OUT_OF_MEMORY;
        textparser_contextual_append(&first, &last, node);

        const textparser_contextual_lexer_rule *source = source_rule >= 0
            ? &definition->lexer_rules[source_rule] : nullptr;
        const textparser_contextual_lexer_rule *rule = source != nullptr &&
            (source->pop_mode || source->push_mode != nullptr)
            ? source : &definition->lexer_rules[token->kind];
        if (textparser_rule_should_pop(handle, rule)) textparser_pop_mode(handle);
        if (rule->push_mode != nullptr) textparser_push_mode(handle, rule->push_mode);
        handle->parser.source_offset = token->end;
        handle->parser.token_index++;
    }

    handle->first_item = first;
    /* Leave the handle in the language's initial lexical state: the legacy
     * scanner never touched the mode stack, but this path does, and grammar
     * execution expects an empty stack. */
    textparser_reset_lexical_state(handle);
    return textparser_rebuild_lexer_streams(handle);
}

EXPORT_TEXTPARSER bool textparser_has_line_terminator_between(textparser_t handle, size_t start_pos, size_t end_pos)
{
    if (handle == nullptr || start_pos >= end_pos) {
        return false;
    }

    size_t total = textparser_get_total_units(handle);
    size_t limit = (end_pos < total) ? end_pos : total;

    for (size_t p = start_pos; p < limit; p++) {
        uint32_t ch = textparser_get_unit_at(handle, p);
        if (ch == '\n' || ch == '\r' || ch == 0x2028 || ch == 0x2029) {
            return true;
        }
    }

    return false;
}

/* -------------------------------------------------------------------------
 * Phase 4: Declarative Grammar Engine & Speculative Parsing Implementations
 * ------------------------------------------------------------------------- */

EXPORT_TEXTPARSER int textparser_register_predicate(
    textparser_t handle,
    const char *name,
    textparser_predicate_fn predicate,
    void *user_data)
{
    if (handle == nullptr || name == nullptr || predicate == nullptr) {
        return -1;
    }

    textparser_predicate_entry *entry = handle->predicates;
    while (entry != nullptr) {
        if (entry->name && strcmp(entry->name, name) == 0) {
            entry->predicate = predicate;
            entry->parser_predicate = nullptr;
            entry->user_data = user_data;
            return 0;
        }
        entry = entry->next;
    }

    entry = malloc(sizeof(textparser_predicate_entry));
    if (entry == nullptr) {
        return -1;
    }
    entry->name = strdup(name);
    if (entry->name == nullptr) {
        free(entry);
        return -1;
    }
    entry->predicate = predicate;
    entry->parser_predicate = nullptr;
    entry->user_data = user_data;
    entry->next = handle->predicates;
    handle->predicates = entry;

    return 0;
}

EXPORT_TEXTPARSER int textparser_register_parser_predicate(
    textparser_t handle,
    const char *name,
    textparser_parser_predicate_fn predicate,
    void *user_data)
{
    if (handle == nullptr || name == nullptr || predicate == nullptr) return -1;
    textparser_predicate_entry *entry = handle->predicates;
    while (entry != nullptr) {
        if (entry->name && strcmp(entry->name, name) == 0) {
            entry->predicate = nullptr;
            entry->parser_predicate = predicate;
            entry->user_data = user_data;
            return 0;
        }
        entry = entry->next;
    }
    entry = calloc(1, sizeof(*entry));
    if (entry == nullptr) return -1;
    entry->name = strdup(name);
    if (entry->name == nullptr) {
        free(entry);
        return -1;
    }
    entry->parser_predicate = predicate;
    entry->user_data = user_data;
    entry->next = handle->predicates;
    handle->predicates = entry;
    return 0;
}

EXPORT_TEXTPARSER bool textparser_eval_predicate(
    textparser_t handle,
    const char *name)
{
    if (handle == nullptr || name == nullptr) {
        return false;
    }

    textparser_predicate_entry *entry = handle->predicates;
    while (entry != nullptr) {
        if (entry->name && strcmp(entry->name, name) == 0) {
            if (entry->predicate != nullptr) return entry->predicate(handle, name, entry->user_data);
            return false;
        }
        entry = entry->next;
    }

    return false;
}

EXPORT_TEXTPARSER int textparser_context_set(
    textparser_t handle,
    const char *context_name,
    int64_t value)
{
    if (handle == nullptr || context_name == nullptr) {
        return -1;
    }

    textparser_context_entry *entry = handle->contexts;
    while (entry != nullptr) {
        if (entry->name && strcmp(entry->name, context_name) == 0) {
            entry->value = value;
            return 0;
        }
        entry = entry->next;
    }

    entry = malloc(sizeof(textparser_context_entry));
    if (entry == nullptr) {
        return -1;
    }
    entry->name = strdup(context_name);
    if (entry->name == nullptr) {
        free(entry);
        return -1;
    }
    entry->value = value;
    entry->next = handle->contexts;
    handle->contexts = entry;

    return 0;
}

EXPORT_TEXTPARSER int textparser_context_get(
    textparser_t handle,
    const char *context_name,
    int64_t *out_value)
{
    if (handle == nullptr || context_name == nullptr || out_value == nullptr) {
        return -1;
    }

    textparser_context_entry *entry = handle->contexts;
    while (entry != nullptr) {
        if (entry->name && strcmp(entry->name, context_name) == 0) {
            *out_value = entry->value;
            return 0;
        }
        entry = entry->next;
    }

    return -1;
}

EXPORT_TEXTPARSER bool textparser_context_is(
    textparser_t handle,
    const char *context_name)
{
    if (handle == nullptr || context_name == nullptr) {
        return false;
    }

    int64_t val = 0;
    if (textparser_context_get(handle, context_name, &val) == 0) {
        return val != 0;
    }
    return false;
}

/**
 * Count the number of active scoped context entries stored in the handle.
 *
 * @param context Context frame or state structure.
 * @return Number of scoped context variables currently defined.
 */
static size_t textparser_context_count(const textparser_context_entry *context)
{
    size_t count = 0;
    while (context != nullptr) {
        count++;
        context = context->next;
    }
    return count;
}

EXPORT_TEXTPARSER int textparser_get_parser_state(
    textparser_t handle,
    textparser_parser_state_view *out_state)
{
    if (handle == nullptr || out_state == nullptr) return -1;
    out_state->source_offset = handle->parser.source_offset;
    out_state->token_index = handle->parser.token_index;
    out_state->mode_depth = handle->mode_stack_depth;
    out_state->context_depth = textparser_context_count(handle->contexts);
    out_state->diagnostic_count = handle->diagnostic_count;
    out_state->pending_event_count = handle->parser.pending_event_count;
    out_state->speculation_depth = handle->parser.speculation_depth;
    out_state->recovery_depth = handle->parser.recovery_depth;
    return 0;
}

#define TEXTPARSER_CHECKPOINT_MAGIC UINT64_C(0x545043484b505431)

typedef struct {
    uint64_t magic;
    textparser_t owner;
    textparser_arena_checkpoint arena;
    size_t source_offset;
    size_t token_index;
    size_t pending_event_count;
    unsigned speculation_depth;
    unsigned recovery_depth;
    bool has_previous_token;
    textparser_lex_token previous_token;
    size_t mode_depth;
    char *modes[TEXTPARSER_MAX_MODE_STACK];
    char *lexical_goal;
    textparser_context_entry *contexts;
    textparser_diagnostic *diagnostics;
    size_t diagnostic_count;
    uint64_t memo_seq;
} textparser_parser_checkpoint;

/**
 * Free all nodes in a linked list of scoped context entries.
 *
 * @param context Context frame or state structure.
 */
static void textparser_free_context_list(textparser_context_entry *context)
{
    while (context != nullptr) {
        textparser_context_entry *next = context->next;
        free(context->name);
        free(context);
        context = next;
    }
}

/**
 * Create a deep copy of a linked list of scoped context entries.
 *
 * @param source Source text slice in text buffer.
 * @return Pointer to head of newly allocated context list copy, or NULL if empty.
 */
static textparser_context_entry *textparser_clone_context_list(
    const textparser_context_entry *source)
{
    textparser_context_entry *head = nullptr;
    textparser_context_entry **tail = &head;
    while (source != nullptr) {
        textparser_context_entry *copy = calloc(1, sizeof(*copy));
        if (copy == nullptr) goto fail;
        copy->name = source->name ? strdup(source->name) : nullptr;
        if (source->name != nullptr && copy->name == nullptr) {
            free(copy);
            goto fail;
        }
        copy->value = source->value;
        *tail = copy;
        tail = &copy->next;
        source = source->next;
    }
    return head;

fail:
    textparser_free_context_list(head);
    return nullptr;
}

/**
 * Release heap-allocated diagnostic entries in a diagnostic snapshot array.
 *
 * @param diagnostics Diagnostics vector or snapshot array.
 * @param count Number of items in array.
 */
static void textparser_free_diagnostic_snapshot(
    textparser_diagnostic *diagnostics,
    size_t count)
{
    if (diagnostics == nullptr) return;
    for (size_t i = 0; i < count; i++) {
        free((void *)diagnostics[i].code);
        free((void *)diagnostics[i].message);
    }
    free(diagnostics);
}

/**
 * Create a deep snapshot copy of the current handle diagnostics vector.
 *
 * @param source Source text slice in text buffer.
 * @param count Total count of items in array.
 * @return 0 on success, or non-zero on allocation failure.
 */
static textparser_diagnostic *textparser_clone_diagnostics(
    const textparser_diagnostic *source,
    size_t count)
{
    if (count == 0) return nullptr;
    textparser_diagnostic *copy = calloc(count, sizeof(*copy));
    if (copy == nullptr) return nullptr;
    for (size_t i = 0; i < count; i++) {
        copy[i] = source[i];
        copy[i].code = source[i].code ? strdup(source[i].code) : nullptr;
        copy[i].message = source[i].message ? strdup(source[i].message) : nullptr;
        if ((source[i].code && !copy[i].code) || (source[i].message && !copy[i].message)) {
            textparser_free_diagnostic_snapshot(copy, count);
            return nullptr;
        }
    }
    return copy;
}

/**
 * Release heap allocations (contexts, diagnostics) associated with a speculation checkpoint.
 *
 * @param checkpoint Pointer to speculation checkpoint structure to free.
 */
static void textparser_checkpoint_free(textparser_parser_checkpoint *checkpoint)
{
    if (checkpoint == nullptr) return;
    for (size_t i = 0; i < checkpoint->mode_depth; i++) free(checkpoint->modes[i]);
    free(checkpoint->lexical_goal);
    textparser_free_context_list(checkpoint->contexts);
    textparser_free_diagnostic_snapshot(checkpoint->diagnostics, checkpoint->diagnostic_count);
    checkpoint->magic = 0;
    free(checkpoint);
}

typedef struct textparser_memo_entry {
    int production_id;
    size_t token_index;
    uint32_t context_hash;
    char *lexical_goal;
    textparser_match_result result;
    size_t source_length;
    uint64_t seq;
    struct textparser_memo_entry *next;
} textparser_memo_entry;

/**
 * Compute a 64-bit hash over active scoped contexts for packrat memoization table keying.
 *
 * @param head In/out pointer to head of node or token list.
 * @return 64-bit hash value summarizing names and values of all active contexts.
 */
static uint32_t textparser_hash_contexts(const textparser_context_entry *head)
{
    uint32_t h = 2166136261u;
    for (const textparser_context_entry *c = head; c != nullptr; c = c->next) {
        if (c->name != nullptr) {
            for (const char *p = c->name; *p; p++) {
                h ^= (uint8_t)*p;
                h *= 16777619u;
            }
        }
        uint64_t val = (uint64_t)c->value;
        for (int i = 0; i < 8; i++) {
            h ^= (uint8_t)(val >> (i * 8));
            h *= 16777619u;
        }
    }
    return h;
}

/**
 * Clear all cached production results in the packrat grammar memoization table.
 *
 * @param handle Pointer to the textparser handle.
 */
static void textparser_memo_clear(textparser_t handle)
{
    if (handle == nullptr) return;
    textparser_memo_entry *curr = handle->grammar_memo;
    while (curr != nullptr) {
        textparser_memo_entry *next = curr->next;
        free(curr->lexical_goal);
        free(curr);
        curr = next;
    }
    handle->grammar_memo = nullptr;
}

/**
 * Shift token indexes and invalidate dirty regions in grammar memoization table after edits.
 *
 * @param handle Pointer to the textparser handle.
 * @param dirty_start_token Starting token index affected by edit.
 * @param dirty_end_token Ending token index affected by edit.
 * @param delta_tokens Net difference in token count after edit.
 */
static void textparser_memo_shift_and_invalidate(
    textparser_t handle,
    size_t dirty_start_token,
    size_t dirty_end_token,
    ssize_t delta_tokens)
{
    if (handle == nullptr) return;
    textparser_memo_entry **head_ptr = &handle->grammar_memo;
    while (*head_ptr != nullptr) {
        textparser_memo_entry *entry = *head_ptr;
        size_t entry_start = entry->token_index;
        size_t entry_end = entry->token_index + entry->result.consumed_tokens;

        // Invalidate if overlapping the dirty token range
        if (entry_end > dirty_start_token && entry_start < dirty_end_token) {
            *head_ptr = entry->next;
            free(entry->lexical_goal);
            free(entry);
        } else {
            // If strictly after the dirty token range, shift by delta_tokens
            if (entry_start >= dirty_end_token && delta_tokens != 0) {
                entry->token_index = (size_t)((ssize_t)entry->token_index + delta_tokens);
            }
            head_ptr = &entry->next;
        }
    }
}

/**
 * Invalidate memoization entries added after a checkpoint sequence counter.
 *
 * @param handle Pointer to the textparser handle.
 * @param seq Query sequence structure.
 */
static void textparser_memo_rollback(textparser_t handle, uint64_t seq)
{
    if (handle == nullptr) return;
    textparser_memo_entry **head_ptr = &handle->grammar_memo;
    while (*head_ptr != nullptr) {
        textparser_memo_entry *entry = *head_ptr;
        if (entry->seq > seq) {
            *head_ptr = entry->next;
            free(entry->lexical_goal);
            free(entry);
        } else {
            head_ptr = &entry->next;
        }
    }
}

/**
 * Look up a cached production parse result in the packrat memoization table.
 *
 * @param handle Pointer to the textparser handle.
 * @param production_id Production ID.
 * @param token_index Token index where production began.
 * @param context_hash Active scoped context hash value.
 * @param lexical_goal Active lexical goal name string.
 * @return true if a valid cached result was retrieved; false on memo miss.
 */
static const textparser_memo_entry *textparser_memo_lookup(
    textparser_t handle,
    int production_id,
    size_t token_index,
    uint32_t context_hash,
    const char *lexical_goal)
{
    if (handle == nullptr) return nullptr;
    for (const textparser_memo_entry *e = handle->grammar_memo; e != nullptr; e = e->next) {
        if (e->production_id == production_id &&
            e->token_index == token_index &&
            e->context_hash == context_hash) {
            if ((e->lexical_goal == nullptr && lexical_goal == nullptr) ||
                (e->lexical_goal != nullptr && lexical_goal != nullptr &&
                 strcmp(e->lexical_goal, lexical_goal) == 0)) {
                return e;
            }
        }
    }
    return nullptr;
}

/**
 * Store a production parse result into the packrat memoization table.
 *
 * @param handle Pointer to the textparser handle.
 * @param production_id Production ID.
 * @param token_index Token index where production began.
 * @param context_hash Active scoped context hash value.
 * @param lexical_goal Active lexical goal name string.
 * @param result Pointer to match result to cache.
 * @param source_length Length of source span in units.
 */
static void textparser_memo_store(
    textparser_t handle,
    int production_id,
    size_t token_index,
    uint32_t context_hash,
    const char *lexical_goal,
    const textparser_match_result *result,
    size_t source_length)
{
    if (handle == nullptr || result == nullptr) return;
    textparser_memo_entry *entry = malloc(sizeof(*entry));
    if (entry == nullptr) return;
    entry->production_id = production_id;
    entry->token_index = token_index;
    entry->context_hash = context_hash;
    entry->lexical_goal = lexical_goal ? strdup(lexical_goal) : nullptr;
    entry->result = *result;
    entry->source_length = source_length;
    entry->seq = ++handle->next_memo_seq;
    entry->next = handle->grammar_memo;
    handle->grammar_memo = entry;
}

/**
 * Determine whether a grammar production kind is eligible for packrat memoization.
 *
 * @param production Pointer to the declarative production structure.
 * @return true if production kind should be memoized; false otherwise.
 */
static bool textparser_is_memoizable_production(const textparser_production *production)
{
    if (production == nullptr || production->name == nullptr) return false;
    const char *n = production->name;
    return (strncmp(n, "Statement", 9) == 0 ||
            strncmp(n, "Declaration", 11) == 0 ||
            strcmp(n, "ClassElement") == 0 ||
            strcmp(n, "TypeMember") == 0 ||
            strcmp(n, "FunctionDeclaration") == 0 ||
            strcmp(n, "VariableStatement") == 0 ||
            strcmp(n, "ExportDeclaration") == 0 ||
            strcmp(n, "ImportDeclaration") == 0);
}

EXPORT_TEXTPARSER void textparser_speculate_begin(
    textparser_t handle,
    void **out_checkpoint)
{
    if (out_checkpoint == nullptr) return;
    *out_checkpoint = nullptr;
    if (handle == nullptr) return;

    textparser_parser_checkpoint *cp = calloc(1, sizeof(*cp));
    if (cp == nullptr) return;
    cp->magic = TEXTPARSER_CHECKPOINT_MAGIC;
    cp->owner = handle;
    cp->arena = textparser_arena_checkpoint_save(handle);
    cp->source_offset = handle->parser.source_offset;
    cp->token_index = handle->parser.token_index;
    cp->pending_event_count = handle->parser.pending_event_count;
    cp->speculation_depth = handle->parser.speculation_depth;
    cp->recovery_depth = handle->parser.recovery_depth;
    cp->has_previous_token = handle->parser.has_previous_token;
    cp->previous_token = handle->parser.previous_token;
    cp->mode_depth = handle->mode_stack_depth;

    for (size_t i = 0; i < cp->mode_depth; i++) {
        cp->modes[i] = handle->mode_stack[i] ? strdup(handle->mode_stack[i]) : nullptr;
        if (handle->mode_stack[i] != nullptr && cp->modes[i] == nullptr) goto fail;
    }
    cp->lexical_goal = handle->lexical_goal ? strdup(handle->lexical_goal) : nullptr;
    if (handle->lexical_goal != nullptr && cp->lexical_goal == nullptr) goto fail;
    cp->contexts = textparser_clone_context_list(handle->contexts);
    if (handle->contexts != nullptr && cp->contexts == nullptr) goto fail;
    cp->diagnostic_count = handle->diagnostic_count;
    cp->diagnostics = textparser_clone_diagnostics(handle->diagnostics, cp->diagnostic_count);
    if (cp->diagnostic_count != 0 && cp->diagnostics == nullptr) goto fail;
    cp->memo_seq = handle->next_memo_seq;

    handle->parser.speculation_depth++;
    *out_checkpoint = cp;
    return;

fail:
    textparser_checkpoint_free(cp);
}

EXPORT_TEXTPARSER void textparser_speculate_commit(
    textparser_t handle,
    void *checkpoint)
{
    textparser_parser_checkpoint *cp = checkpoint;
    if (cp == nullptr || cp->magic != TEXTPARSER_CHECKPOINT_MAGIC || cp->owner != handle) return;
    handle->parser.speculation_depth = cp->speculation_depth;
    textparser_checkpoint_free(cp);
}

EXPORT_TEXTPARSER void textparser_speculate_rollback(
    textparser_t handle,
    void *checkpoint)
{
    textparser_parser_checkpoint *cp = checkpoint;
    if (handle == nullptr || cp == nullptr || cp->magic != TEXTPARSER_CHECKPOINT_MAGIC || cp->owner != handle) return;

    for (size_t i = cp->arena.chunk_count; i < handle->arena.chunk_count; i++) {
        free(handle->arena.chunks[i]);
        handle->arena.chunks[i] = nullptr;
    }
    handle->arena.chunk_count = cp->arena.chunk_count;
    textparser_arena_checkpoint_restore(handle, &cp->arena);

    for (size_t i = 0; i < handle->mode_stack_depth; i++) {
        free(handle->mode_stack[i]);
        handle->mode_stack[i] = nullptr;
    }
    handle->mode_stack_depth = cp->mode_depth;
    for (size_t i = 0; i < cp->mode_depth; i++) {
        handle->mode_stack[i] = cp->modes[i];
        cp->modes[i] = nullptr;
    }
    free(handle->lexical_goal);
    handle->lexical_goal = cp->lexical_goal;
    cp->lexical_goal = nullptr;

    textparser_free_context_list(handle->contexts);
    handle->contexts = cp->contexts;
    cp->contexts = nullptr;

    textparser_free_diagnostic_snapshot(handle->diagnostics, handle->diagnostic_count);
    handle->diagnostics = cp->diagnostics;
    handle->diagnostic_count = cp->diagnostic_count;
    handle->diagnostic_capacity = cp->diagnostic_count;
    cp->diagnostics = nullptr;
    cp->diagnostic_count = 0;

    handle->parser.source_offset = cp->source_offset;
    handle->parser.token_index = cp->token_index;
    handle->parser.pending_event_count = cp->pending_event_count;
    handle->parser.speculation_depth = cp->speculation_depth;
    handle->parser.recovery_depth = cp->recovery_depth;
    handle->parser.has_previous_token = cp->has_previous_token;
    handle->parser.previous_token = cp->previous_token;
    textparser_memo_rollback(handle, cp->memo_seq);
    textparser_checkpoint_free(cp);
}

typedef struct {
    textparser_t handle;
    const textparser_production *productions;
    size_t production_count;
    unsigned recursion_depth;
    size_t furthest_failure_offset;
    size_t furthest_failure_length;
    int furthest_unexpected_token;
    const textparser_production *furthest_failure;
    const char *validator_diagnostic_code;
    const char *validator_diagnostic_message;
    size_t validator_diagnostic_start;
    size_t validator_diagnostic_length;
    size_t initial_diagnostic_count;
    textparser_capture_entry *captures;
} textparser_grammar_executor;

/**
 * Helper function to construct a textparser_match_result structure.
 *
 * @param status Match status enum (MATCH_OK, MATCH_NO, MATCH_ERROR, MATCH_ABORT).
 * @param node Constructed syntax node pointer (or NULL).
 * @param consumed Output pointer receiving number of consumed tokens.
 * @return Populated textparser_match_result structure.
 */
static textparser_match_result textparser_match_result_make(
    textparser_match_status status,
    textparser_node *node,
    size_t consumed)
{
    textparser_match_result result = {0};
    result.status = status;
    result.node = node;
    result.consumed_tokens = consumed;
    result.committed = false;
    return result;
}

/**
 * Helper function to construct a committed textparser_match_result structure.
 *
 * @param status Match status enum.
 * @param node Constructed syntax node pointer (or NULL).
 * @param consumed Output pointer receiving number of consumed tokens.
 * @param committed Output pointer receiving true if branch committed.
 * @return Populated textparser_match_result with committed flag set to true.
 */
static textparser_match_result textparser_match_result_committed(
    textparser_match_status status,
    textparser_node *node,
    size_t consumed,
    bool committed)
{
    textparser_match_result result = textparser_match_result_make(status, node, consumed);
    result.committed = committed;
    return result;
}

/**
 * Look up a production definition by its integer production ID in the executor table.
 *
 * @param executor Pointer to grammar executor containing production table.
 * @param production_id Unique integer ID of production.
 * @return Pointer to textparser_production if found, or NULL if ID is out of bounds.
 */
static const textparser_production *textparser_find_production(
    const textparser_grammar_executor *executor,
    int production_id)
{
    const textparser_production *found = nullptr;
    for (size_t i = 0; i < executor->production_count; i++) {
        if (executor->productions[i].id == production_id) {
            if (found != nullptr) return nullptr;
            found = &executor->productions[i];
        }
    }
    return found;
}

/**
 * Create an AST terminal node for a matched grammar token.
 *
 * @param handle Pointer to the active textparser handle.
 * @param token Pointer to lexer token snapshot.
 * @return Allocated CST token node, or NULL on allocation error.
 */
static textparser_node *textparser_grammar_token_node(
    textparser_t handle,
    const textparser_lex_token *token)
{
    textparser_node *node = textparser_alloc_token(handle, token->kind, token->end - token->start);
    if (node != nullptr) {
        node->decoded_value = token->decoded_value;
        node->source_start = token->start;
        node->source_end = token->end;
        node->node_flags |= TEXTPARSER_NODE_EXPLICIT_SPAN;
        if (handle->language != nullptr && handle->language->tokens != nullptr && token->kind >= 0)
            node->cst_kind = handle->language->tokens[token->kind].name;
    }
    return node;
}

/**
 * Create an AST container node grouping child productions under a CST kind name.
 *
 * @param handle Pointer to the active textparser handle.
 * @param production Pointer to the declarative production structure.
 * @param first_child Pointer to first child node in children linked list.
 * @param source_start Starting unit offset in input buffer.
 * @param source_length Length of source span in units.
 * @return Allocated container CST node.
 */
static textparser_node *textparser_grammar_group_node(
    textparser_t handle,
    const textparser_production *production,
    textparser_node *first_child,
    size_t source_start,
    size_t source_length)
{
    if (first_child == nullptr) return nullptr;
    textparser_node *node = textparser_alloc_token(handle, production->id, source_length);
    if (node == nullptr) return nullptr;
    node->node_flags |= TEXTPARSER_NODE_SYNTHETIC;
    node->node_flags |= TEXTPARSER_NODE_EXPLICIT_SPAN;
    static const char *production_kinds[] = {
        "Token", "Reference", "Sequence", "Choice", "Optional", "Repeat",
        "Lookahead", "NegativeLookahead", "Predicate", "Context", "Commit",
        "PrattExpression", "LexicalGoal", "Capture", "MatchCapture"
    };
    node->cst_kind = production->ast_kind != nullptr ? production->ast_kind :
        (production->name != nullptr ? production->name :
        (production->kind >= TEXTPARSER_PROD_TOKEN &&
         production->kind <= TEXTPARSER_PROD_MATCH_CAPTURE
            ? production_kinds[production->kind] : "Production"));
    node->category = production->category;
    node->source_start = source_start;
    node->source_end = source_start + source_length;
    node->child = first_child;
    for (textparser_node *child = first_child; child != nullptr; child = child->next) {
        child->parent = node;
    }
    return node;
}

/**
 * Append a newly matched production node to an accumulating sibling linked list.
 *
 * @param first In/out pointer to head of sibling node list.
 * @param last In/out pointer to tail of sibling node list.
 * @param node New node to append.
 */
static void textparser_grammar_append_node(
    textparser_node **first,
    textparser_node **last,
    textparser_node *node)
{
    if (node == nullptr) return;
    node->prev = *last;
    node->next = nullptr;
    if (*last != nullptr) (*last)->next = node;
    else *first = node;
    *last = node;
}

static textparser_match_result textparser_parse_production(
    textparser_grammar_executor *executor,
    int production_id);

/**
 * Peek at the next token in the lexer stream without consuming it.
 *
 * @param executor Pointer to grammar executor.
 * @param out Output pointer receiving parsed result.
 * @return 0 on success, 1 on EOF, or negative error code.
 */
static int textparser_grammar_peek_token(
    textparser_grammar_executor *executor,
    const textparser_lex_token **out)
{
    textparser_t handle = executor->handle;
    if (handle->language != nullptr && handle->language->initial_lexer_mode != nullptr)
        return textparser_lexer_peek(handle, 0, handle->lexical_goal, out);
    if (handle->parser.token_index >= handle->lexer_token_count) {
        *out = nullptr;
        return 1;
    }
    *out = &handle->lexer_tokens[handle->parser.token_index];
    return 0;
}

/**
 * Consume the next token in the lexer stream and advance the parser cursor.
 *
 * @param executor Pointer to grammar executor.
 * @param expected_kind Expected CST kind name string.
 * @return 0 on success, 1 on EOF, or negative error code.
 */
static textparser_match_result textparser_grammar_consume_token(
    textparser_grammar_executor *executor,
    int expected_kind)
{
    textparser_t handle = executor->handle;
    const textparser_lex_token *token = nullptr;
    int scan = textparser_grammar_peek_token(executor, &token);
    if (scan < 0) return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    if (scan > 0 || token == nullptr || token->kind != expected_kind)
        return textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
    textparser_node *node = textparser_grammar_token_node(handle, token);
    if (node == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    if (handle->language != nullptr && handle->language->initial_lexer_mode != nullptr) {
        const textparser_lex_token *consumed = nullptr;
        if (textparser_lexer_consume(handle, handle->lexical_goal, &consumed) != 0)
            return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    } else {
        handle->parser.token_index++;
        handle->parser.source_offset = token->end;
    }
    return textparser_match_result_make(TEXTPARSER_MATCH_OK, node, 1);
}

/**
 * Record failure location and expected token info for furthest-failure diagnostic reporting.
 *
 * @param executor Pointer to grammar executor.
 * @param production Production that failed to match.
 */
static void textparser_grammar_note_failure(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    const textparser_lex_token *unexpected = nullptr;
    int peek = textparser_grammar_peek_token(executor, &unexpected);
    bool has_contextual_lexer = executor->handle->language != nullptr &&
        executor->handle->language->initial_lexer_mode != nullptr;
    size_t offset = has_contextual_lexer
        ? (unexpected != nullptr ? unexpected->start : textparser_get_total_units(executor->handle))
        : executor->handle->parser.source_offset;
    size_t length = has_contextual_lexer && unexpected != nullptr
        ? unexpected->end - unexpected->start : 0;
    int description_priority = production != nullptr && production->expected_description != nullptr
        ? (strcmp(production->expected_description, "expression") == 0 ? 1 : 2) : 0;
    int previous_priority = executor->furthest_failure != nullptr &&
        executor->furthest_failure->expected_description != nullptr
        ? (strcmp(executor->furthest_failure->expected_description, "expression") == 0 ? 1 : 2) : 0;
    bool prefer_description = offset == executor->furthest_failure_offset &&
        description_priority > previous_priority;
    if (executor->furthest_failure == nullptr || offset > executor->furthest_failure_offset ||
        prefer_description) {
        executor->furthest_failure_offset = offset;
        executor->furthest_failure_length = length;
        executor->furthest_unexpected_token = peek == 0 && unexpected != nullptr
            ? unexpected->kind : -1;
        executor->furthest_failure = production;
    }
}

/* Expand only %s and %%; never pass definition text as a printf format string. */
static void textparser_format_diagnostic(char *buffer, size_t capacity,
    const char *format, const char *value)
{
    size_t used = 0;
    if (capacity == 0) return;
    for (size_t i = 0; format[i] != '\0' && used + 1 < capacity; i++) {
        if (format[i] == '%' && format[i + 1] == 's') {
            for (size_t j = 0; value[j] != '\0' && used + 1 < capacity; j++) buffer[used++] = value[j];
            i++;
        } else {
            buffer[used++] = format[i];
            if (format[i] == '%' && format[i + 1] == '%') i++;
        }
    }
    buffer[used] = '\0';
}

/**
 * Emit a syntax diagnostic describing expected tokens at the furthest failure offset.
 *
 * @param executor Pointer to grammar executor.
 * @param production Production where failure occurred.
 * @param start Starting character unit offset.
 * @param length Token span length.
 * @param recovered Output pointer receiving true if recovery was triggered.
 */
static int textparser_grammar_report_expected(
    textparser_grammar_executor *executor,
    const textparser_production *production,
    size_t start,
    size_t length,
    bool recovered)
{
    size_t limit = executor->handle->language && executor->handle->language->maximum_diagnostics
        ? executor->handle->language->maximum_diagnostics : 100;
    if (executor->handle->diagnostic_count >= limit) return 0;
    char message[256];
    const char *name = production && production->expected_description
        ? production->expected_description
        : (production && production->name ? production->name : "syntax element");
    const textparser_language_definition *language = executor->handle->language;
    const textparser_token *token = nullptr;
    if (production != nullptr && production->kind == TEXTPARSER_PROD_TOKEN && language != nullptr &&
        language->tokens != nullptr && production->token_id >= 0 &&
        (size_t)production->token_id < executor->handle->token_count)
        token = &language->tokens[production->token_id];
    const textparser_diagnostic_template *selected = nullptr;
    if (recovered) {
        if (production != nullptr && production->diagnostics.recovered.message != nullptr)
            selected = &production->diagnostics.recovered;
        else if (language != nullptr && language->diagnostics.recovered.message != nullptr)
            selected = &language->diagnostics.recovered;
    } else {
        if (production != nullptr && production->diagnostics.expected.message != nullptr)
            selected = &production->diagnostics.expected;
        else if (token != nullptr && token->diagnostics.expected.message != nullptr)
            selected = &token->diagnostics.expected;
        else if (token != nullptr && token->spelling != nullptr && language != nullptr &&
                 language->diagnostics.token_expected.message != nullptr)
            selected = &language->diagnostics.token_expected;
        else if (language != nullptr && language->diagnostics.expected.message != nullptr)
            selected = &language->diagnostics.expected;
    }
    const char *code = recovered ? "TEXTPARSER_RECOVERED" : "TEXTPARSER_EXPECTED";
    const char *format = recovered ? "Recovered while parsing %s." : "Expected %s.";
    const char *value = !recovered && token != nullptr && token->spelling != nullptr ? token->spelling : name;
    if (selected != nullptr) {
        if (selected->code != nullptr) code = selected->code;
        format = selected->message;
    } else if (!recovered && token != nullptr && token->spelling != nullptr) {
        format = "'%s' expected.";
    }
    textparser_format_diagnostic(message, sizeof(message), format, value);
    return textparser_report_diagnostic(executor->handle, TEXTPARSER_SEVERITY_ERROR,
        code, message, start, length);
}

/**
 * Check whether a token ID belongs to the synchronization token set of a production.
 *
 * @param executor Pointer to grammar executor.
 * @param production Production defining synchronization tokens.
 * @param token_kind Expected integer token kind ID.
 * @return true if token_id is in production or global synchronization sets; false otherwise.
 */
static bool textparser_grammar_is_sync(
    const textparser_grammar_executor *executor,
    const textparser_production *production,
    int token_kind)
{
    const int *tokens = production->recovery_sync_tokens;
    size_t count = production->recovery_sync_token_count;
    if (count == 0 && executor->handle->language != nullptr) {
        tokens = executor->handle->language->recovery_sync_tokens;
        count = executor->handle->language->recovery_sync_token_count;
    }
    for (size_t i = 0; i < count; i++)
        if (tokens[i] == token_kind) return true;
    return false;
}

/**
 * Synthesize and insert a zero-width missing token node for error recovery.
 *
 * @param executor Pointer to grammar executor.
 * @param production Production requiring the missing token.
 * @param token_kind Expected integer token kind ID.
 * @param report_error true to report syntax error diagnostic on failure.
 * @return Constructed missing CST node with TEXTPARSER_NODE_MISSING flag set.
 */
static textparser_match_result textparser_grammar_missing_token(
    textparser_grammar_executor *executor,
    const textparser_production *production,
    int token_kind,
    bool report_error)
{
    textparser_node *node = textparser_alloc_token(executor->handle, token_kind, 0);
    if (node == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    node->node_flags |= TEXTPARSER_NODE_SYNTHETIC | TEXTPARSER_NODE_MISSING;
    node->node_flags |= TEXTPARSER_NODE_EXPLICIT_SPAN;
    node->source_start = executor->handle->parser.source_offset;
    node->source_end = executor->handle->parser.source_offset;
    if (executor->handle->language != nullptr && token_kind >= 0)
        node->cst_kind = executor->handle->language->tokens[token_kind].name;
    if (report_error)
        textparser_grammar_report_expected(executor, production,
            executor->handle->parser.source_offset, 0, false);
    executor->handle->parser.recovery_depth++;
    return textparser_match_result_make(TEXTPARSER_MATCH_OK, node, 0);
}

/**
 * Evaluate JavaScript/TypeScript Automatic Semicolon Insertion (ASI) legality rules.
 *
 * @param executor Pointer to grammar executor.
 * @param production Pointer to the declarative production structure.
 * @return true if semicolon can be automatically inserted; false otherwise.
 */
static bool textparser_grammar_can_insert_semicolon(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    const textparser_lex_token *current = nullptr;
    int peek = textparser_grammar_peek_token(executor, &current);
    if (peek > 0 || current == nullptr) return true;
    if (peek < 0) return false;
    return (current->flags & TEXTPARSER_LEX_FLAG_CONTAINS_LINE_TERMINATOR) != 0 ||
        textparser_grammar_is_sync(executor, production, current->kind);
}

/**
 * Perform panic-mode error recovery by skipping unexpected tokens to a synchronization barrier.
 *
 * @param executor Pointer to grammar executor.
 * @param production Production encountering syntax mismatch.
 * @return TEXTPARSER_MATCH_OK on successful resync, or TEXTPARSER_MATCH_ERROR if recovery failed.
 */
static textparser_match_result textparser_grammar_synchronize(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    textparser_t handle = executor->handle;
    void *checkpoint = nullptr;
    textparser_speculate_begin(handle, &checkpoint);
    if (checkpoint == nullptr)
        return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    size_t maximum = handle->language && handle->language->maximum_skipped_tokens
        ? handle->language->maximum_skipped_tokens : 256;
    size_t start_offset = handle->parser.source_offset;
    size_t start_index = handle->parser.token_index;
    textparser_node *first = nullptr;
    textparser_node *last = nullptr;
    while (handle->parser.token_index - start_index < maximum) {
        const textparser_lex_token *token = nullptr;
        int peek = textparser_grammar_peek_token(executor, &token);
        if (peek != 0 || token == nullptr || textparser_grammar_is_sync(executor, production, token->kind)) break;
        textparser_match_result skipped = textparser_grammar_consume_token(executor, token->kind);
        if (skipped.status != TEXTPARSER_MATCH_OK) {
            textparser_speculate_rollback(handle, checkpoint);
            return skipped;
        }
        textparser_grammar_append_node(&first, &last, skipped.node);
    }
    size_t consumed = handle->parser.token_index - start_index;
    if (consumed == 0) {
        textparser_speculate_rollback(handle, checkpoint);
        return textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
    }

    /* Recovery must reach a real boundary. At EOF there is no enclosing
       list iteration to resume, so preserving the original failure gives
       callers the precise terminal diagnostic instead of accepting an
       incomplete file. A final semicolon/terminator is likewise not useful
       by itself without following input to resume. */
    if (handle->language != nullptr && handle->language->grammar != nullptr) {
        const textparser_lex_token *boundary = nullptr;
        int boundary_status = textparser_grammar_peek_token(executor, &boundary);
        bool can_resume = boundary_status == 0 && boundary != nullptr &&
            textparser_grammar_is_sync(executor, production, boundary->kind);
        if (can_resume && handle->language != nullptr && boundary->kind >= 0 &&
            (size_t)boundary->kind < handle->token_count) {
            const textparser_token *tok = &handle->language->tokens[boundary->kind];
            bool is_terminator = (tok->spelling != nullptr && strcmp(tok->spelling, ";") == 0) ||
                (tok->name != nullptr && strcmp(tok->name, "Semicolon") == 0);
            if (is_terminator) {
                const textparser_lex_token *following = nullptr;
                int following_status = handle->language->initial_lexer_mode != nullptr
                    ? textparser_lexer_peek(handle, 1, handle->lexical_goal, &following)
                    : (handle->parser.token_index + 1 < handle->lexer_token_count
                        ? (following = &handle->lexer_tokens[handle->parser.token_index + 1], 0) : 1);
                can_resume = following_status == 0 && following != nullptr;
            }
        }
        if (!can_resume) {
            textparser_speculate_rollback(handle, checkpoint);
            return textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
        }
    }
    textparser_node *node = textparser_grammar_group_node(
        handle, production, first, start_offset,
        handle->parser.source_offset - start_offset);
    if (node == nullptr) {
        textparser_speculate_rollback(handle, checkpoint);
        return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    }
    node->node_flags |= TEXTPARSER_NODE_RECOVERED;
    textparser_grammar_report_expected(executor, production, start_offset,
        handle->parser.source_offset - start_offset, true);
    handle->parser.recovery_depth++;
    textparser_speculate_commit(handle, checkpoint);
    return textparser_match_result_make(TEXTPARSER_MATCH_OK, node, consumed);
}

/**
 * Construct an AST operator node linking operands during Pratt expression parsing.
 *
 * @param op Operator definition specifying role, precedence, and symbol.
 * @param first In/out pointer to head of sibling node list.
 * @param second Second operand CST node.
 * @param third Third operand CST node.
 * @param source_length Length of source span in units.
 * @return Constructed operator expression CST node.
 */
static textparser_node *textparser_pratt_operator_node(
    textparser_node *op,
    textparser_node *first,
    textparser_node *second,
    textparser_node *third,
    size_t source_length)
{
    if (op == nullptr || first == nullptr) return nullptr;
    op->len = source_length;
    if ((first->node_flags & TEXTPARSER_NODE_EXPLICIT_SPAN) != 0) {
        op->node_flags |= TEXTPARSER_NODE_EXPLICIT_SPAN;
        op->source_start = first->source_start;
        op->source_end = first->source_start + source_length;
    }
    op->child = first;
    first->parent = op;
    first->prev = nullptr;
    first->next = second;
    if (second != nullptr) {
        second->parent = op;
        second->prev = first;
        second->next = third;
    }
    if (third != nullptr) {
        third->parent = op;
        third->prev = second;
        third->next = nullptr;
    }
    return op;
}


/**
 * Find the first terminal leaf token node within a CST subtree.
 *
 * @param node Root of subtree to search.
 * @return Pointer to first terminal leaf node, or node itself if already a terminal.
 */
EXPORT_TEXTPARSER const textparser_node *textparser_node_first_terminal(const textparser_node *node)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        if ((item->node_flags & TEXTPARSER_NODE_SYNTHETIC) == 0) return item;
        const textparser_node *nested = textparser_node_first_terminal(item->child);
        if (nested != nullptr) return nested;
    }
    return nullptr;
}

/**
 * Retrieve the last direct child node under a parent CST node.
 *
 * @param node Parent CST node.
 * @return Pointer to last child node, or NULL if node has no children.
 */
EXPORT_TEXTPARSER const textparser_node *textparser_node_last_child(const textparser_node *node)
{
    const textparser_node *last = node == nullptr ? nullptr : node->child;
    if (last == nullptr) return nullptr;
    while (last->next != nullptr) last = last->next;
    return last;
}

EXPORT_TEXTPARSER const char *textparser_grammar_node_name(const textparser_t handle, const textparser_node *node)
{
    if (node == nullptr) return nullptr;
    if ((node->node_flags & TEXTPARSER_NODE_SYNTHETIC) != 0) {
        if (node->cst_kind != nullptr) return node->cst_kind;
        if (handle != nullptr && handle->language != nullptr && handle->language->grammar != nullptr) {
            for (size_t i = 0; i < handle->language->grammar->production_count; i++) {
                if (handle->language->grammar->productions[i].id == node->token_id) {
                    const textparser_production *prod = &handle->language->grammar->productions[i];
                    return prod->ast_kind != nullptr ? prod->ast_kind : prod->name;
                }
            }
        }
        return nullptr;
    }
    if (handle != nullptr && handle->language != nullptr && handle->language->tokens != nullptr && node->token_id >= 0) {
        return handle->language->tokens[node->token_id].name;
    }
    return node->cst_kind;
}

/**
 * Invoke registered validator callback on completed left or prefix Pratt operand.
 *
 * @param executor Pointer to grammar executor.
 * @param validator Registered validator callback name string.
 * @param node Completed operand CST node.
 * @param allow_pattern true if destructuring patterns are permitted as valid targets.
 * @return true if operand passes validator; false otherwise.
 */
static bool textparser_pratt_validate_operand(
    textparser_grammar_executor *executor,
    const char *validator,
    const textparser_node *node,
    bool allow_pattern)
{
    if (validator == nullptr) return true;
    if (executor == nullptr || executor->handle == nullptr) return true;

    textparser_operand_validator_entry *entry = executor->handle->operand_validators;
    while (entry != nullptr) {
        if (entry->name != nullptr && strcmp(entry->name, validator) == 0) {
            const char *code = nullptr;
            const char *msg = nullptr;
            size_t start = 0;
            size_t length = 0;
            bool ok = entry->validator(executor->handle, validator, node, allow_pattern,
                                       &code, &msg, &start, &length, entry->user_data);
            if (!ok) {
                executor->validator_diagnostic_code = code;
                executor->validator_diagnostic_message = msg;
                executor->validator_diagnostic_start = start;
                executor->validator_diagnostic_length = length;
                return false;
            }
            return true;
        }
        entry = entry->next;
    }

    return true; // Unregistered validator accepted by default
}

/**
 * Recursive Pratt expression parsing loop handling prefix, infix, postfix, and ternary binding powers.
 *
 * @param executor Pointer to grammar executor.
 * @param primary_production Primary production ID parsing atomic expressions.
 * @param postfix_production Optional postfix production ID, or -1.
 * @param minimum_precedence Minimum binding power required to consume next operator.
 * @param depth Current expression recursion nesting depth.
 * @return textparser_match_result containing the parsed expression subtree root.
 */
static textparser_match_result textparser_parse_pratt_internal(
    textparser_grammar_executor *executor,
    int primary_production,
    int postfix_production,
    int minimum_precedence,
    unsigned depth)
{
    if (depth >= MAX_RECURSION_DEPTH)
        return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    textparser_t handle = executor->handle;
    size_t start_index = handle->parser.token_index;
    size_t start_offset = handle->parser.source_offset;
    const textparser_lex_token *next = nullptr;
    textparser_operator_def prefix = {0};
    textparser_match_result left;
    bool has_expression_goals = false;
    if (handle->language != nullptr) {
        for (size_t i = 0; i < handle->language->lexer_goal_count; i++) {
            if (strcmp(handle->language->lexer_goals[i].name, "ExpressionStart") == 0)
                has_expression_goals = true;
        }
    }
    if (has_expression_goals) textparser_set_lexical_goal(handle, "ExpressionStart");
    if (textparser_grammar_peek_token(executor, &next) == 0 && next != nullptr &&
        textparser_get_operator(handle, next->kind, TEXTPARSER_OP_PREFIX, &prefix) == 0) {
        textparser_match_result op = textparser_grammar_consume_token(executor, next->kind);
        if (op.status != TEXTPARSER_MATCH_OK) return op;
        textparser_match_result operand = textparser_parse_pratt_internal(
            executor, primary_production, postfix_production, prefix.precedence, depth + 1);
        if (operand.status != TEXTPARSER_MATCH_OK)
            return textparser_match_result_committed(TEXTPARSER_MATCH_ERROR, nullptr,
                handle->parser.token_index - start_index, operand.committed);
        if (!textparser_pratt_validate_operand(
                executor, prefix.operand_validator, operand.node, false))
            return textparser_match_result_committed(TEXTPARSER_MATCH_ERROR, nullptr,
                handle->parser.token_index - start_index, true);
        left = textparser_match_result_committed(TEXTPARSER_MATCH_OK,
            textparser_pratt_operator_node(op.node, operand.node, nullptr, nullptr,
                handle->parser.source_offset - start_offset),
            handle->parser.token_index - start_index, operand.committed);
    } else {
        left = textparser_parse_production(executor, primary_production);
        if (left.status != TEXTPARSER_MATCH_OK) return left;
    }

    if (has_expression_goals) textparser_set_lexical_goal(handle, "ExpressionContinuation");
    for (;;) {
        if (postfix_production >= 0) {
            void *suffix_checkpoint = nullptr;
            textparser_speculate_begin(handle, &suffix_checkpoint);
            if (suffix_checkpoint == nullptr)
                return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
            textparser_match_result suffix = textparser_parse_production(executor, postfix_production);
            if (suffix.status == TEXTPARSER_MATCH_OK) {
                if (suffix.consumed_tokens == 0) {
                    textparser_speculate_rollback(handle, suffix_checkpoint);
                    return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
                }
                textparser_speculate_commit(handle, suffix_checkpoint);
                textparser_node *suffix_children = suffix.node ? suffix.node->child : nullptr;
                if (suffix.node == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
                suffix.node->child = left.node;
                left.node->parent = suffix.node;
                left.node->prev = nullptr;
                left.node->next = suffix_children;
                if (suffix_children != nullptr) {
                    suffix_children->prev = left.node;
                    for (textparser_node *child = suffix_children; child != nullptr; child = child->next)
                        child->parent = suffix.node;
                }
                suffix.node->len = handle->parser.source_offset - start_offset;
                suffix.node->node_flags |= TEXTPARSER_NODE_GRAMMAR_POSTFIX;
                suffix.node->node_flags |= TEXTPARSER_NODE_EXPLICIT_SPAN;
                suffix.node->source_start = start_offset;
                suffix.node->source_end = handle->parser.source_offset;
                left.node = suffix.node;
                left.consumed_tokens = handle->parser.token_index - start_index;
                left.committed = left.committed || suffix.committed;
                continue;
            }
            if (suffix.committed) {
                textparser_speculate_commit(handle, suffix_checkpoint);
                return textparser_match_result_committed(
                    TEXTPARSER_MATCH_ERROR, nullptr,
                    handle->parser.token_index - start_index, true);
            }
            textparser_match_status suffix_status = suffix.status;
            textparser_speculate_rollback(handle, suffix_checkpoint);
            if (suffix_status != TEXTPARSER_MATCH_NO)
                return textparser_match_result_make(suffix_status, nullptr, 0);
        }
        if (textparser_grammar_peek_token(executor, &next) != 0 || next == nullptr) break;
        textparser_operator_def opdef = {0};
        bool postfix = textparser_get_operator(handle, next->kind, TEXTPARSER_OP_POSTFIX, &opdef) == 0;
        if (postfix && (next->flags & TEXTPARSER_LEX_FLAG_CONTAINS_LINE_TERMINATOR) != 0)
            break;
        bool ternary = false;
        if (!postfix) ternary = textparser_get_operator(handle, next->kind, TEXTPARSER_OP_TERNARY, &opdef) == 0;
        if (!postfix && !ternary &&
            textparser_get_operator(handle, next->kind, TEXTPARSER_OP_INFIX, &opdef) != 0) break;
        if (opdef.precedence < minimum_precedence) break;

        const char *operator_name = handle->language != nullptr && handle->language->tokens != nullptr
            ? handle->language->tokens[next->kind].name : nullptr;
        bool allow_assignment_pattern = operator_name != nullptr && strcmp(operator_name, "Assign") == 0;
        if (!textparser_pratt_validate_operand(
                executor, opdef.left_validator, left.node, allow_assignment_pattern))
            return textparser_match_result_committed(TEXTPARSER_MATCH_ERROR, nullptr,
                handle->parser.token_index - start_index, true);

        size_t expression_start = start_offset;
        textparser_match_result op = textparser_grammar_consume_token(executor, next->kind);
        if (op.status != TEXTPARSER_MATCH_OK) return op;
        if (postfix) {
            left.node = textparser_pratt_operator_node(op.node, left.node, nullptr, nullptr,
                handle->parser.source_offset - expression_start);
            left.consumed_tokens = handle->parser.token_index - start_index;
            continue;
        }
        int right_min = opdef.associativity == TEXTPARSER_ASSOC_LEFT
            ? opdef.precedence + 1 : opdef.precedence;
        if (ternary) {
            textparser_match_result middle = textparser_parse_pratt_internal(
                executor, primary_production, postfix_production, 0, depth + 1);
            if (middle.status != TEXTPARSER_MATCH_OK)
                return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
            if (has_expression_goals) textparser_set_lexical_goal(handle, "ExpressionContinuation");
            textparser_match_result separator = textparser_grammar_consume_token(
                executor, opdef.secondary_token_id);
            if (separator.status != TEXTPARSER_MATCH_OK)
                return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
            textparser_match_result right = textparser_parse_pratt_internal(
                executor, primary_production, postfix_production, right_min, depth + 1);
            if (right.status != TEXTPARSER_MATCH_OK)
                return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
            left.node = textparser_pratt_operator_node(op.node, left.node, middle.node, right.node,
                handle->parser.source_offset - expression_start);
            left.committed = left.committed || middle.committed || right.committed;
        } else {
            textparser_match_result right = textparser_parse_pratt_internal(
                executor, primary_production, postfix_production, right_min, depth + 1);
            if (right.status != TEXTPARSER_MATCH_OK)
                return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
            left.node = textparser_pratt_operator_node(op.node, left.node, right.node, nullptr,
                handle->parser.source_offset - expression_start);
            left.committed = left.committed || right.committed;
        }
        if (has_expression_goals) textparser_set_lexical_goal(handle, "ExpressionContinuation");
        left.consumed_tokens = handle->parser.token_index - start_index;
    }
    return left;
}

/**
 * Execute a PRATT declarative production construct using the registered operator table.
 *
 * @param executor Pointer to grammar executor.
 * @param production PRATT production definition.
 * @return textparser_match_result containing expression parse status and root node.
 */
static textparser_match_result textparser_parse_pratt(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    if ((production->child_count != 1 && production->child_count != 2) || production->children == nullptr)
        return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    char *saved_goal = executor->handle->lexical_goal
        ? strdup(executor->handle->lexical_goal) : nullptr;
    if (executor->handle->lexical_goal != nullptr && saved_goal == nullptr)
        return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    void *checkpoint = nullptr;
    textparser_speculate_begin(executor->handle, &checkpoint);
    if (checkpoint == nullptr) {
        free(saved_goal);
        return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    }
    textparser_match_result result = textparser_parse_pratt_internal(
        executor, production->children[0],
        production->child_count == 2 ? production->children[1] : -1,
        production->minimum_precedence, 0);
    if (result.status == TEXTPARSER_MATCH_OK) {
        /* A successfully completed expression is a cut boundary.  The commit
           flags raised while disambiguating inside the expression (arrow
           bodies, call arguments, member/index suffixes, array/object literal
           openers, ...) must not leak into the enclosing construct.  If they
           did, a later failure *after* the finished expression would be
           reported as a hard committed error and structural backtracking that
           legitimately re-classifies the enclosing production would be
           suppressed (e.g. choosing the paired JSX element form once a
           self-closing tag attempt has consumed attribute-value expressions
           such as arrow functions or call expressions). */
        result.committed = false;
        textparser_set_lexical_goal(executor->handle, saved_goal);
        textparser_speculate_commit(executor->handle, checkpoint);
    } else {
        textparser_speculate_rollback(executor->handle, checkpoint);
    }
    free(saved_goal);
    return result;
}

/**
 * Execute a SEQUENCE declarative production construct matching all children in order.
 *
 * @param executor Pointer to grammar executor.
 * @param production SEQUENCE production definition.
 * @return textparser_match_result on success, or mismatch with rollback on failure.
 */
static textparser_match_result textparser_parse_sequence(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    textparser_t handle = executor->handle;
    size_t start_index = handle->parser.token_index;
    size_t start_offset = handle->parser.source_offset;
    void *checkpoint = nullptr;
    textparser_speculate_begin(handle, &checkpoint);
    if (checkpoint == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);

    textparser_node *first = nullptr;
    textparser_node *last = nullptr;
    bool committed = false;
    for (size_t i = 0; i < production->child_count; i++) {
        textparser_match_result child = textparser_parse_production(executor, production->children[i]);
        if (child.status != TEXTPARSER_MATCH_OK) {
            if (committed || child.committed) {
                size_t consumed = handle->parser.token_index - start_index;
                textparser_speculate_commit(handle, checkpoint);
                return textparser_match_result_committed(TEXTPARSER_MATCH_ERROR, nullptr, consumed, true);
            }
            textparser_match_status status = child.status;
            textparser_speculate_rollback(handle, checkpoint);
            return textparser_match_result_make(status, nullptr, 0);
        }
        committed = committed || child.committed;
        textparser_grammar_append_node(&first, &last, child.node);
    }

    textparser_node *node = textparser_grammar_group_node(
        handle, production, first, start_offset,
        handle->parser.source_offset - start_offset);
    if (first != nullptr && node == nullptr) {
        textparser_speculate_rollback(handle, checkpoint);
        return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    }
    size_t consumed = handle->parser.token_index - start_index;
    textparser_speculate_commit(handle, checkpoint);
    return textparser_match_result_committed(TEXTPARSER_MATCH_OK, node, consumed, committed);
}

/**
 * Execute a CHOICE declarative production construct trying alternatives with speculative rollback.
 *
 * @param executor Pointer to grammar executor.
 * @param production CHOICE production definition.
 * @return textparser_match_result of first matching alternative, or failure result.
 */
static textparser_match_result textparser_parse_choice(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    textparser_t handle = executor->handle;
    for (size_t i = 0; i < production->child_count; i++) {
        void *checkpoint = nullptr;
        textparser_speculate_begin(handle, &checkpoint);
        if (checkpoint == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
        textparser_match_result result = textparser_parse_production(executor, production->children[i]);
        if (result.status == TEXTPARSER_MATCH_OK) {
            textparser_speculate_commit(handle, checkpoint);
            if (production->name != nullptr && result.node != nullptr &&
                result.node->cst_kind != nullptr &&
                strcmp(result.node->cst_kind, "Sequence") == 0 &&
                result.node->child != nullptr) {
                result.node->cst_kind = production->ast_kind != nullptr ?
                    production->ast_kind : production->name;
                result.node->category = production->category;
            }
            return result;
        }
        textparser_match_status status = result.status;
        if (result.committed) {
            textparser_speculate_commit(handle, checkpoint);
            return textparser_match_result_committed(TEXTPARSER_MATCH_ERROR, nullptr,
                result.consumed_tokens, true);
        }
        textparser_speculate_rollback(handle, checkpoint);
        if (status != TEXTPARSER_MATCH_NO) {
            return textparser_match_result_make(status, nullptr, 0);
        }
    }
    return textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
}

/**
 * Execute an OPTIONAL declarative production construct matching zero or one time.
 *
 * @param executor Pointer to grammar executor.
 * @param production OPTIONAL production definition.
 * @return textparser_match_result with matched node if present, or success with empty node.
 */
static textparser_match_result textparser_parse_optional(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    if (production->child_count != 1 || production->children == nullptr) {
        return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    }
    void *checkpoint = nullptr;
    textparser_speculate_begin(executor->handle, &checkpoint);
    if (checkpoint == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    textparser_match_result result = textparser_parse_production(executor, production->children[0]);
    if (result.status == TEXTPARSER_MATCH_OK) {
        textparser_speculate_commit(executor->handle, checkpoint);
        return result;
    }
    textparser_match_status status = result.status;
    if (result.committed) {
        textparser_speculate_commit(executor->handle, checkpoint);
        return textparser_match_result_committed(TEXTPARSER_MATCH_ERROR, nullptr,
            result.consumed_tokens, true);
    }
    textparser_speculate_rollback(executor->handle, checkpoint);
    if (status == TEXTPARSER_MATCH_NO) {
        return textparser_match_result_make(TEXTPARSER_MATCH_OK, nullptr, 0);
    }
    return textparser_match_result_make(status, nullptr, 0);
}

/**
 * Execute a REPEAT declarative production construct matching zero or more times with forward progress.
 *
 * @param executor Pointer to grammar executor.
 * @param production REPEAT production definition.
 * @return textparser_match_result containing grouped repeated child nodes.
 */
static textparser_match_result textparser_parse_repeat(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    if (production->child_count != 1 || production->children == nullptr) {
        return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    }
    textparser_t handle = executor->handle;
    size_t start_index = handle->parser.token_index;
    size_t start_offset = handle->parser.source_offset;
    void *outer = nullptr;
    textparser_speculate_begin(handle, &outer);
    if (outer == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    textparser_node *first = nullptr;
    textparser_node *last = nullptr;
    bool committed = false;

    for (;;) {
        size_t iteration_start = handle->parser.token_index;
        void *iteration = nullptr;
        textparser_speculate_begin(handle, &iteration);
        if (iteration == nullptr) {
            textparser_speculate_rollback(handle, outer);
            return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
        }
        textparser_match_result child = textparser_parse_production(executor, production->children[0]);
        if (child.status == TEXTPARSER_MATCH_NO) {
            textparser_speculate_rollback(handle, iteration);
            break;
        }
        if (child.status != TEXTPARSER_MATCH_OK) {
            if (committed || child.committed) {
                textparser_speculate_commit(handle, iteration);
                textparser_speculate_commit(handle, outer);
                return textparser_match_result_committed(TEXTPARSER_MATCH_ERROR, nullptr,
                    handle->parser.token_index - start_index, true);
            }
            textparser_match_status status = child.status;
            textparser_speculate_rollback(handle, iteration);
            textparser_speculate_rollback(handle, outer);
            return textparser_match_result_make(status, nullptr, 0);
        }
        if (handle->parser.token_index == iteration_start) {
            textparser_speculate_rollback(handle, iteration);
            textparser_speculate_rollback(handle, outer);
            return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
        }
        textparser_speculate_commit(handle, iteration);
        committed = committed || child.committed;
        textparser_grammar_append_node(&first, &last, child.node);
    }

    textparser_node *node = textparser_grammar_group_node(
        handle, production, first, start_offset,
        handle->parser.source_offset - start_offset);
    if (first != nullptr && node == nullptr) {
        textparser_speculate_rollback(handle, outer);
        return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    }
    size_t consumed = handle->parser.token_index - start_index;
    textparser_speculate_commit(handle, outer);
    return textparser_match_result_committed(TEXTPARSER_MATCH_OK, node, consumed, committed);
}

/**
 * Execute a positive or negative LOOKAHEAD construct speculatively without consuming input.
 *
 * @param executor Pointer to grammar executor.
 * @param production LOOKAHEAD production definition.
 * @param negative true for negative lookahead (!P); false for positive lookahead (&P).
 * @return textparser_match_result indicating lookahead assertion success or failure.
 */
static textparser_match_result textparser_parse_lookahead(
    textparser_grammar_executor *executor,
    const textparser_production *production,
    bool negative)
{
    if (production->child_count != 1 || production->children == nullptr) {
        return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    }
    void *checkpoint = nullptr;
    textparser_speculate_begin(executor->handle, &checkpoint);
    if (checkpoint == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    textparser_match_result child = textparser_parse_production(executor, production->children[0]);
    textparser_speculate_rollback(executor->handle, checkpoint);
    if (child.status == TEXTPARSER_MATCH_OK) {
        return textparser_match_result_make(negative ? TEXTPARSER_MATCH_NO : TEXTPARSER_MATCH_OK, nullptr, 0);
    }
    if (child.status == TEXTPARSER_MATCH_NO) {
        return textparser_match_result_make(negative ? TEXTPARSER_MATCH_OK : TEXTPARSER_MATCH_NO, nullptr, 0);
    }
    return textparser_match_result_make(child.status, nullptr, 0);
}

/* Filename suffix matching deliberately ignores ASCII case, independent of locale. */
static bool textparser_guard_suffix_matches(const char *filename, const char *suffix)
{
    if (filename == nullptr) return false;
    size_t length = strlen(filename), suffix_length = strlen(suffix);
    if (suffix_length > length) return false;
    filename += length - suffix_length;
    for (size_t i = 0; i < suffix_length; i++) {
        unsigned char a = (unsigned char)filename[i], b = (unsigned char)suffix[i];
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return false;
    }
    return true;
}

/* Compare exact raw source spelling in UTF-8, including UTF-16/32 input conversion. */
static bool textparser_guard_text_matches(textparser_t handle,
    const textparser_lex_token *token, const char *expected)
{
    if (token->end < token->start || token->end > textparser_get_total_units(handle)) return false;
    if (handle->text_format != TEXTPARSER_ENCODING_UNICODE &&
        handle->text_format != TEXTPARSER_ENCODING_UTF_16 && handle->text_format != TEXTPARSER_ENCODING_UTF_32) {
        size_t length = token->end - token->start;
        return length == strlen(expected) && memcmp(handle->text_addr + token->start, expected, length) == 0;
    }
    size_t remaining = strlen(expected);
    for (size_t i = token->start; i < token->end; i++) {
        uint32_t cp = textparser_get_unit_at(handle, i);
        if (handle->text_format != TEXTPARSER_ENCODING_UTF_32 && cp >= 0xD800 && cp <= 0xDBFF &&
            i + 1 < token->end) {
            uint32_t low = textparser_get_unit_at(handle, i + 1);
            if (low >= 0xDC00 && low <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + low - 0xDC00;
                i++;
            }
        }
        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return false;
        char bytes[4];
        size_t length = encode_utf8_codepoint(cp, bytes);
        if (length > remaining || memcmp(expected, bytes, length) != 0) return false;
        expected += length;
        remaining -= length;
    }
    return remaining == 0;
}

static textparser_match_result textparser_parse_guard(
    textparser_grammar_executor *executor, const textparser_guard *guard)
{
    if (guard->file_suffixes != nullptr) {
        bool matches = false;
        for (size_t i = 0; guard->file_suffixes[i] != nullptr; i++)
            if (textparser_guard_suffix_matches(executor->handle->filename, guard->file_suffixes[i])) {
                matches = true;
                break;
            }
        if (!matches) return textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
    }
    if (guard->line_terminator_before || guard->next_tokens != nullptr || guard->next_token_text != nullptr) {
        const textparser_lex_token *token = nullptr;
        int scan = textparser_grammar_peek_token(executor, &token);
        if (scan < 0) return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
        bool eof = scan > 0 || token == nullptr;
        bool newline = !eof && (token->flags & TEXTPARSER_LEX_FLAG_CONTAINS_LINE_TERMINATOR) != 0;
        if (guard->line_terminator_before && newline != (guard->line_terminator_before > 0))
            return textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
        if (guard->next_tokens != nullptr) {
            bool matches = eof && guard->allow_eof;
            for (size_t i = 0; !eof && i < guard->next_token_count; i++)
                if (token->kind == guard->next_tokens[i]) { matches = true; break; }
            if (!matches) return textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
        }
        if (guard->next_token_text != nullptr &&
            (eof || !textparser_guard_text_matches(executor->handle, token, guard->next_token_text)))
            return textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
    }
    return textparser_match_result_make(TEXTPARSER_MATCH_OK, nullptr, 0);
}

/**
 * Execute a generic guard or a registered native predicate callback.
 *
 * @param executor Pointer to grammar executor.
 * @param production PREDICATE production definition.
 * @return textparser_match_result indicating predicate success or mismatch.
 */
static textparser_match_result textparser_parse_predicate(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    textparser_t handle = executor->handle;
    if (production->guard != nullptr) return textparser_parse_guard(executor, production->guard);
    if (production->predicate_name == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    textparser_predicate_entry *entry = handle->predicates;
    while (entry != nullptr && strcmp(entry->name, production->predicate_name) != 0) entry = entry->next;
    if (entry == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    void *checkpoint = nullptr;
    textparser_speculate_begin(handle, &checkpoint);
    if (checkpoint == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    bool accepted = false;
    if (entry->parser_predicate != nullptr) {
        textparser_predicate_context context = {0};
        context.production_id = production->id;
        if (handle->language != nullptr && handle->language->initial_lexer_mode != nullptr) {
            const textparser_lex_token *current = nullptr;
            if (textparser_lexer_peek(handle, 0, handle->lexical_goal, &current) == 0) {
                context.current = current;
                context.has_preceding_line_terminator =
                    (current->flags & TEXTPARSER_LEX_FLAG_CONTAINS_LINE_TERMINATOR) != 0;
            }
            if (handle->parser.has_previous_token) context.previous = &handle->parser.previous_token;
        } else if (handle->parser.token_index < handle->lexer_token_count) {
            context.current = &handle->lexer_tokens[handle->parser.token_index];
            context.has_preceding_line_terminator =
                (context.current->flags & TEXTPARSER_LEX_FLAG_CONTAINS_LINE_TERMINATOR) != 0;
        }
        if (handle->parser.token_index > 0) context.previous = &handle->lexer_tokens[handle->parser.token_index - 1];
        accepted = entry->parser_predicate(handle, &context, entry->user_data);
    } else if (entry->predicate != nullptr) {
        accepted = entry->predicate(handle, production->predicate_name, entry->user_data);
    }
    textparser_speculate_rollback(handle, checkpoint);
    return textparser_match_result_make(accepted ? TEXTPARSER_MATCH_OK : TEXTPARSER_MATCH_NO, nullptr, 0);
}

/**
 * Execute a CONTEXT declarative production construct pushing scoped context values during child execution.
 *
 * @param executor Pointer to grammar executor.
 * @param production CONTEXT production definition.
 * @return textparser_match_result of child production execution.
 */
static textparser_match_result textparser_parse_context(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    if (production->context_name == nullptr || production->child_count != 1 || production->children == nullptr) {
        return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    }
    textparser_t handle = executor->handle;
    textparser_context_entry *saved = textparser_clone_context_list(handle->contexts);
    if (handle->contexts != nullptr && saved == nullptr) {
        return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    }
    if (textparser_context_set(handle, production->context_name, production->context_value) != 0) {
        textparser_free_context_list(saved);
        return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    }
    textparser_match_result result = textparser_parse_production(executor, production->children[0]);
    textparser_free_context_list(handle->contexts);
    handle->contexts = saved;
    return result;
}

/**
 * Execute a LEXICAL_GOAL construct activating a transient lexical goal during child parsing.
 *
 * @param executor Pointer to grammar executor.
 * @param production LEXICAL_GOAL production definition.
 * @return textparser_match_result of child production execution.
 */
static textparser_match_result textparser_parse_lexical_goal(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    if (production->lexical_goal == nullptr || production->child_count != 1 ||
        production->children == nullptr)
        return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    textparser_t handle = executor->handle;
    char *saved = handle->lexical_goal ? strdup(handle->lexical_goal) : nullptr;
    if (handle->lexical_goal != nullptr && saved == nullptr)
        return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    textparser_set_lexical_goal(handle, production->lexical_goal);
    textparser_match_result result = textparser_parse_production(executor, production->children[0]);
    textparser_set_lexical_goal(handle, saved);
    free(saved);
    return result;
}

/**
 * Execute a CAPTURE declarative production construct binding named sub-matches.
 *
 * @param executor Pointer to grammar executor.
 * @param production CAPTURE production definition.
 * @return textparser_match_result of captured production execution.
 */
static textparser_match_result textparser_parse_capture(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    if (production->capture_name == nullptr || production->children == nullptr ||
        production->child_count != 2)
        return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    textparser_t handle = executor->handle;
    const textparser_lex_token *first_token = nullptr;
    int scan = textparser_grammar_peek_token(executor, &first_token);
    if (scan < 0) return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    if (scan > 0 || first_token == nullptr)
        return textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
    size_t start_index = handle->parser.token_index;
    size_t start_offset = handle->parser.source_offset;
    void *checkpoint = nullptr;
    textparser_speculate_begin(handle, &checkpoint);
    if (checkpoint == nullptr)
        return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    textparser_match_result captured = textparser_parse_production(executor, production->children[0]);
    if (captured.status != TEXTPARSER_MATCH_OK) {
        if (captured.committed) {
            size_t consumed = handle->parser.token_index - start_index;
            textparser_speculate_commit(handle, checkpoint);
            return textparser_match_result_committed(TEXTPARSER_MATCH_ERROR, nullptr, consumed, true);
        }
        textparser_match_status status = captured.status;
        textparser_speculate_rollback(handle, checkpoint);
        return textparser_match_result_make(status, nullptr, 0);
    }
    textparser_capture_entry entry = {
        .name = production->capture_name,
        .start = first_token->start,
        .end = handle->parser.source_offset,
        .next = executor->captures,
    };
    executor->captures = &entry;
    textparser_match_result remainder = textparser_parse_production(executor, production->children[1]);
    executor->captures = entry.next;
    if (remainder.status != TEXTPARSER_MATCH_OK) {
        textparser_match_status status = remainder.status;
        bool committed = captured.committed || remainder.committed;
        if (committed) {
            size_t consumed = handle->parser.token_index - start_index;
            textparser_speculate_commit(handle, checkpoint);
            return textparser_match_result_committed(TEXTPARSER_MATCH_ERROR, nullptr, consumed, true);
        }
        textparser_speculate_rollback(handle, checkpoint);
        return textparser_match_result_make(status, nullptr, 0);
    }
    textparser_node *first = nullptr;
    textparser_node *last = nullptr;
    textparser_grammar_append_node(&first, &last, captured.node);
    textparser_grammar_append_node(&first, &last, remainder.node);
    textparser_node *node = textparser_grammar_group_node(
        handle, production, first, start_offset,
        handle->parser.source_offset - start_offset);
    if (first != nullptr && node == nullptr) {
        textparser_speculate_rollback(handle, checkpoint);
        return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    }
    size_t consumed = handle->parser.token_index - start_index;
    textparser_speculate_commit(handle, checkpoint);
    return textparser_match_result_committed(
        TEXTPARSER_MATCH_OK, node, consumed, captured.committed || remainder.committed);
}

/**
 * Execute a MATCH_CAPTURE construct asserting that current input matches previously captured span.
 *
 * @param executor Pointer to grammar executor.
 * @param production MATCH_CAPTURE production definition.
 * @return textparser_match_result indicating matched capture assertion success or failure.
 */
static textparser_match_result textparser_parse_match_capture(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    if (production->capture_name == nullptr || production->children == nullptr ||
        production->child_count != 1)
        return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    textparser_capture_entry *capture = executor->captures;
    while (capture != nullptr && strcmp(capture->name, production->capture_name) != 0)
        capture = capture->next;
    if (capture == nullptr)
        return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    const textparser_lex_token *first_token = nullptr;
    int scan = textparser_grammar_peek_token(executor, &first_token);
    if (scan < 0) return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    if (scan > 0 || first_token == nullptr)
        return textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
    void *checkpoint = nullptr;
    textparser_speculate_begin(executor->handle, &checkpoint);
    if (checkpoint == nullptr)
        return textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    textparser_match_result result = textparser_parse_production(executor, production->children[0]);
    if (result.status != TEXTPARSER_MATCH_OK) {
        if (result.committed) {
            textparser_speculate_commit(executor->handle, checkpoint);
            return textparser_match_result_committed(
                TEXTPARSER_MATCH_ERROR, nullptr, result.consumed_tokens, true);
        }
        textparser_match_status status = result.status;
        textparser_speculate_rollback(executor->handle, checkpoint);
        return textparser_match_result_make(status, nullptr, 0);
    }
    size_t match_start = first_token->start;
    size_t match_end = executor->handle->parser.source_offset;
    size_t captured_length = capture->end - capture->start;
    size_t match_length = match_end - match_start;
    bool equal = capture->end <= executor->handle->text_size &&
        match_end <= executor->handle->text_size && captured_length == match_length &&
        memcmp(executor->handle->text_addr + capture->start,
               executor->handle->text_addr + match_start, captured_length) == 0;
    if (!equal) {
        if (result.committed) {
            textparser_speculate_commit(executor->handle, checkpoint);
            return textparser_match_result_committed(
                TEXTPARSER_MATCH_ERROR, nullptr, result.consumed_tokens, true);
        }
        textparser_speculate_rollback(executor->handle, checkpoint);
        return textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
    }
    textparser_speculate_commit(executor->handle, checkpoint);
    return result;
}

/**
 * Central grammar execution dispatcher executing a production by ID with packrat memoization.
 *
 * @param executor Pointer to grammar executor.
 * @param production_id Integer production ID to execute.
 * @return textparser_match_result containing match status, syntax node, and consumed token count.
 */
static textparser_match_result textparser_parse_production(
    textparser_grammar_executor *executor,
    int production_id)
{
    if (executor->recursion_depth >= MAX_RECURSION_DEPTH) {
        return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    }
    const textparser_production *production = textparser_find_production(executor, production_id);
    if (production == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);

    bool can_memoize = textparser_is_memoizable_production(production);
    uint32_t ctx_hash = 0;
    size_t start_token_idx = executor->handle->parser.token_index;
    if (can_memoize) {
        ctx_hash = textparser_hash_contexts(executor->handle->contexts);
        const textparser_memo_entry *cached = textparser_memo_lookup(
            executor->handle, production_id, start_token_idx, ctx_hash, executor->handle->lexical_goal);
        if (cached != nullptr) {
            executor->handle->parser.token_index += cached->result.consumed_tokens;
            executor->handle->parser.source_offset += cached->source_length;
            return cached->result;
        }
    }

    size_t event_start = executor->handle->parser.source_offset;
    executor->recursion_depth++;
    textparser_match_result result;

    switch (production->kind) {
    case TEXTPARSER_PROD_TOKEN: {
        textparser_t handle = executor->handle;
        const textparser_lex_token *token = nullptr;
        bool contextual = handle->language != nullptr && handle->language->initial_lexer_mode != nullptr;
        if (contextual) {
            int scan = textparser_lexer_peek(handle, 0, handle->lexical_goal, &token);
            if (scan < 0) {
                result = textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
                break;
            }
            if (scan > 0 || token == nullptr || token->kind != production->token_id) {
                textparser_grammar_note_failure(executor, production);
                if (production->allow_automatic_semicolon &&
                    textparser_grammar_can_insert_semicolon(executor, production)) {
                    result = textparser_grammar_missing_token(
                        executor, production, production->token_id, false);
                    break;
                }
                result = textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
                break;
            }
        } else {
            if (handle->parser.token_index >= handle->lexer_token_count ||
                handle->lexer_tokens[handle->parser.token_index].kind != production->token_id) {
                textparser_grammar_note_failure(executor, production);
                if (production->allow_automatic_semicolon &&
                    textparser_grammar_can_insert_semicolon(executor, production)) {
                    result = textparser_grammar_missing_token(
                        executor, production, production->token_id, false);
                    break;
                }
                result = textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
                break;
            }
            token = &handle->lexer_tokens[handle->parser.token_index];
        }
        textparser_node *node = textparser_grammar_token_node(handle, token);
        if (node == nullptr) {
            result = textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
            break;
        }
        if (contextual) {
            const textparser_lex_token *consumed = nullptr;
            if (textparser_lexer_consume(handle, handle->lexical_goal, &consumed) != 0) {
                result = textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
                break;
            }
        } else {
            handle->parser.token_index++;
            handle->parser.source_offset = token->end;
        }
        result = textparser_match_result_make(TEXTPARSER_MATCH_OK, node, 1);
        break;
    }
    case TEXTPARSER_PROD_REF:
        result = textparser_parse_production(executor, production->referenced_production);
        break;
    case TEXTPARSER_PROD_SEQUENCE:
        if (production->child_count != 0 && production->children == nullptr) {
            result = textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
        } else {
            result = textparser_parse_sequence(executor, production);
        }
        break;
    case TEXTPARSER_PROD_CHOICE:
        if (production->child_count != 0 && production->children == nullptr) {
            result = textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
        } else {
            result = textparser_parse_choice(executor, production);
        }
        break;
    case TEXTPARSER_PROD_OPTIONAL:
        result = textparser_parse_optional(executor, production);
        break;
    case TEXTPARSER_PROD_REPEAT:
        result = textparser_parse_repeat(executor, production);
        break;
    case TEXTPARSER_PROD_LOOKAHEAD:
        result = textparser_parse_lookahead(executor, production, false);
        break;
    case TEXTPARSER_PROD_NOT:
        result = textparser_parse_lookahead(executor, production, true);
        break;
    case TEXTPARSER_PROD_PREDICATE:
        result = textparser_parse_predicate(executor, production);
        break;
    case TEXTPARSER_PROD_CONTEXT:
        result = textparser_parse_context(executor, production);
        break;
    case TEXTPARSER_PROD_LEXICAL_GOAL:
        result = textparser_parse_lexical_goal(executor, production);
        break;
    case TEXTPARSER_PROD_CAPTURE:
        result = textparser_parse_capture(executor, production);
        break;
    case TEXTPARSER_PROD_MATCH_CAPTURE:
        result = textparser_parse_match_capture(executor, production);
        break;
    case TEXTPARSER_PROD_COMMIT:
        result = textparser_match_result_committed(TEXTPARSER_MATCH_OK, nullptr, 0, true);
        break;
    case TEXTPARSER_PROD_PRATT:
        result = textparser_parse_pratt(executor, production);
        break;
    default:
        result = textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
        break;
    }
    size_t maximum_attempts = executor->handle->language &&
        executor->handle->language->maximum_recovery_attempts
        ? executor->handle->language->maximum_recovery_attempts : 100;
    if ((result.status == TEXTPARSER_MATCH_NO || result.status == TEXTPARSER_MATCH_ERROR) &&
        executor->handle->parser.recovery_depth < maximum_attempts) {
        if (production->recovery_insert_enabled) {
            result = textparser_grammar_missing_token(
                executor, production, production->recovery_insert_token, true);
        } else if (production->recovery_skip &&
            (result.status == TEXTPARSER_MATCH_NO || production->name == nullptr ||
             strcmp(production->name, "Statement") != 0) &&
            (production->recovery_sync_token_count != 0 ||
             (executor->handle->language && executor->handle->language->recovery_sync_token_count != 0))) {
            textparser_match_result recovered = textparser_grammar_synchronize(executor, production);
            if (recovered.status != TEXTPARSER_MATCH_NO) result = recovered;
        }
    }
    if ((result.status == TEXTPARSER_MATCH_NO || result.status == TEXTPARSER_MATCH_ERROR) &&
        production->expected_description != nullptr)
        textparser_grammar_note_failure(executor, production);
    if (result.status == TEXTPARSER_MATCH_OK && result.node != nullptr) {
        textparser_event event = {0};
        event.node = result.node;
        event.parent = result.node->parent;
        event.start = event_start;
        event.end = executor->handle->parser.source_offset;
        event.synthetic = (result.node->node_flags & TEXTPARSER_NODE_SYNTHETIC) != 0;
        event.recovered = (result.node->node_flags &
            (TEXTPARSER_NODE_RECOVERED | TEXTPARSER_NODE_MISSING)) != 0;
        if (production->validate_handler != nullptr) {
            event.type = TEXTPARSER_EVENT_VALIDATE;
            event.configuration = production->validate_configuration;
            textparser_action action = textparser_dispatch_event(
                executor->handle, production->validate_handler, &event);
            if (action == TEXTPARSER_ACTION_REJECT) {
                result = textparser_match_result_make(TEXTPARSER_MATCH_NO, nullptr, 0);
            } else if (action == TEXTPARSER_ACTION_ABORT) {
                result = textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
            }
        }
        if (result.status == TEXTPARSER_MATCH_OK && event.recovered &&
            production->recovery_handler != nullptr) {
            event.type = TEXTPARSER_EVENT_RECOVERY;
            event.configuration = production->recovery_configuration;
            if (textparser_queue_event(executor->handle,
                    production->recovery_handler, &event) != 0)
                result = textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
        }
        if (result.status == TEXTPARSER_MATCH_OK && production->commit_handler != nullptr) {
            event.type = TEXTPARSER_EVENT_COMMIT;
            event.configuration = production->commit_configuration;
            if (textparser_queue_event(executor->handle,
                    production->commit_handler, &event) != 0)
                result = textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
        }
    }
    if (can_memoize && result.status == TEXTPARSER_MATCH_OK) {
        size_t src_len = executor->handle->parser.source_offset - event_start;
        textparser_memo_store(executor->handle, production_id, start_token_idx,
                              ctx_hash, executor->handle->lexical_goal, &result, src_len);
    }
    executor->recursion_depth--;
    return result;
}

EXPORT_TEXTPARSER int textparser_execute_production(
    textparser_t handle,
    const textparser_production *productions,
    size_t production_count,
    int start_production,
    textparser_match_result *out_result)
{
    if (handle == nullptr || productions == nullptr || production_count == 0 || out_result == nullptr) {
        return -1;
    }
    handle->parser.owner = handle;
    handle->parser.language = handle->language;
    handle->parser.source_offset = 0;
    handle->parser.token_index = 0;
    handle->parser.has_previous_token = false;
    handle->parser.recovery_depth = 0;
    handle->parser.pending_event_count = 0;
    textparser_grammar_executor executor = {
        .handle = handle,
        .productions = productions,
        .production_count = production_count,
        .initial_diagnostic_count = handle->diagnostic_count,
    };
    void *checkpoint = nullptr;
    textparser_speculate_begin(handle, &checkpoint);
    if (checkpoint == nullptr) return -1;
    *out_result = textparser_parse_production(&executor, start_production);
    if (out_result->status == TEXTPARSER_MATCH_OK) {
        textparser_speculate_commit(handle, checkpoint);
        textparser_action action = textparser_publish_pending_events(handle, 0);
        if (action == TEXTPARSER_ACTION_REJECT)
            *out_result = textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
        else if (action == TEXTPARSER_ACTION_ABORT)
            *out_result = textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    } else if (out_result->committed) {
        textparser_speculate_commit(handle, checkpoint);
        handle->parser.pending_event_count = 0;
    } else {
        textparser_speculate_rollback(handle, checkpoint);
    }
    if (out_result->status != TEXTPARSER_MATCH_OK &&
        handle->diagnostic_count == executor.initial_diagnostic_count) {
        if (executor.validator_diagnostic_code != nullptr) {
            textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR,
                executor.validator_diagnostic_code, executor.validator_diagnostic_message,
                executor.validator_diagnostic_start, executor.validator_diagnostic_length);
        } else if (executor.furthest_failure != nullptr) {
            textparser_grammar_report_expected(&executor, executor.furthest_failure,
                executor.furthest_failure_offset, executor.furthest_failure_length, false);
        }
    } else if (out_result->status == TEXTPARSER_MATCH_OK &&
        handle->language != nullptr && handle->language->grammar != nullptr &&
        handle->language->diagnostics.recovered.message != nullptr &&
        handle->diagnostic_count == executor.initial_diagnostic_count) {
        const textparser_lex_token *remaining = nullptr;
        int peek = textparser_grammar_peek_token(&executor, &remaining);
        if (peek == 0 && remaining != nullptr) {
            if (executor.validator_diagnostic_code != nullptr) {
                textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR,
                    executor.validator_diagnostic_code, executor.validator_diagnostic_message,
                    executor.validator_diagnostic_start, executor.validator_diagnostic_length);
            } else if (executor.furthest_failure != nullptr &&
                executor.furthest_failure_offset > remaining->start) {
                textparser_grammar_report_expected(&executor, executor.furthest_failure,
                    executor.furthest_failure_offset, executor.furthest_failure_length, false);
            } else {
                const char *diag_code = handle->language->diagnostics.recovered.code != nullptr
                    ? handle->language->diagnostics.recovered.code : "TEXTPARSER_EXPECTED";
                const char *diag_message = handle->language->diagnostics.recovered.message != nullptr
                    ? handle->language->diagnostics.recovered.message : "Unexpected token.";
                textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR, diag_code,
                    diag_message, remaining->start,
                    remaining->end - remaining->start);
            }
        }
    }
    return 0;
}

EXPORT_TEXTPARSER int textparser_execute_language_grammar(
    textparser_t handle,
    const textparser_language_definition *language,
    textparser_match_result *out_result)
{
    if (handle == nullptr || language == nullptr || language->grammar == nullptr ||
        language->grammar->productions == nullptr || out_result == nullptr) {
        return -1;
    }
    if (language->operator_definition_count != 0) {
        free(handle->operators);
        handle->operators = nullptr;
        handle->operator_count = 0;
        handle->operator_capacity = 0;
    }
    for (size_t i = 0; i < language->operator_definition_count; i++) {
        if (textparser_register_operator(handle, &language->operator_definitions[i]) != 0) return -1;
    }
    int status = textparser_execute_production(
        handle,
        language->grammar->productions,
        language->grammar->production_count,
        language->grammar->start_production,
        out_result);
    if (status != 0 || out_result->status != TEXTPARSER_MATCH_OK) return status;
    textparser_grammar_executor executor = {
        .handle = handle,
        .productions = language->grammar->productions,
        .production_count = language->grammar->production_count,
    };
    const textparser_lex_token *remaining = nullptr;
    int peek = textparser_grammar_peek_token(&executor, &remaining);
    if (peek == 0 && remaining != nullptr) return status;
    if (peek < 0) {
        *out_result = textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
        return status;
    }
    textparser_event event = {0};
    event.type = TEXTPARSER_EVENT_SOURCE_COMPLETE;
    event.node = out_result->node;
    event.start = 0;
    event.end = handle->parser.source_offset;
    event.configuration = language->grammar->source_complete_configuration;
    if (language->grammar->source_complete_handler != nullptr) {
        textparser_action action = textparser_dispatch_event(
            handle, language->grammar->source_complete_handler, &event);
        if (action == TEXTPARSER_ACTION_REJECT)
            *out_result = textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
        else if (action == TEXTPARSER_ACTION_ABORT)
            *out_result = textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
    } else {
        textparser_dispatch_event(handle, "source.complete", &event);
    }
    return status;
}

/* -------------------------------------------------------------------------
 * Phase 5: Operator Precedence & Pratt / Precedence Engine Implementations
 * ------------------------------------------------------------------------- */

EXPORT_TEXTPARSER int textparser_register_operator(
    textparser_t handle,
    const textparser_operator_def *op)
{
    if (handle == nullptr || op == nullptr) {
        return -1;
    }

    /* Check if already registered */
    for (size_t i = 0; i < handle->operator_count; i++) {
        if (handle->operators[i].token_id == op->token_id && handle->operators[i].role == op->role) {
            handle->operators[i] = *op;
            return 0;
        }
    }

    if (handle->operator_count >= handle->operator_capacity) {
        size_t new_cap = handle->operator_capacity == 0 ? 16 : handle->operator_capacity * 2;
        textparser_operator_def *new_ops = realloc(handle->operators, new_cap * sizeof(textparser_operator_def));
        if (new_ops == nullptr) {
            return -1;
        }
        handle->operators = new_ops;
        handle->operator_capacity = new_cap;
    }

    handle->operators[handle->operator_count++] = *op;
    return 0;
}

EXPORT_TEXTPARSER int textparser_get_operator(
    textparser_t handle,
    int token_id,
    int role,
    textparser_operator_def *out_op)
{
    if (handle == nullptr || out_op == nullptr) {
        return -1;
    }

    for (size_t i = 0; i < handle->operator_count; i++) {
        if (handle->operators[i].token_id == token_id) {
            if (role < 0 || handle->operators[i].role == (textparser_operator_role)role) {
                *out_op = handle->operators[i];
                return 0;
            }
        }
    }

    return -1;
}

EXPORT_TEXTPARSER int textparser_parse_pratt_expression(
    textparser_t handle,
    int min_precedence,
    textparser_node **out_node)
{
    if (handle == nullptr || out_node == nullptr) {
        return -1;
    }

    *out_node = nullptr;
    if (handle->language == nullptr || handle->language->grammar == nullptr) return -1;
    const textparser_grammar_definition *grammar = handle->language->grammar;
    const textparser_production *start = nullptr;
    for (size_t i = 0; i < grammar->production_count; i++)
        if (grammar->productions[i].id == grammar->start_production) start = &grammar->productions[i];
    if (start == nullptr || start->kind != TEXTPARSER_PROD_PRATT) return -1;
    if (handle->language->operator_definition_count != 0) {
        free(handle->operators);
        handle->operators = nullptr;
        handle->operator_count = 0;
        handle->operator_capacity = 0;
    }
    for (size_t i = 0; i < handle->language->operator_definition_count; i++) {
        if (textparser_register_operator(handle, &handle->language->operator_definitions[i]) != 0) return -1;
    }
    textparser_production override = *start;
    override.minimum_precedence = min_precedence;
    textparser_grammar_executor executor = {
        .handle = handle,
        .productions = grammar->productions,
        .production_count = grammar->production_count,
    };
    handle->parser.source_offset = 0;
    handle->parser.token_index = 0;
    handle->parser.has_previous_token = false;
    textparser_match_result result = textparser_parse_pratt(&executor, &override);
    if (result.status != TEXTPARSER_MATCH_OK) return -1;
    *out_node = result.node;
    return 0;
}

/* -------------------------------------------------------------------------
 * Phase 6: Error Recovery & Diagnostic Engine Implementations
 * ------------------------------------------------------------------------- */

EXPORT_TEXTPARSER int textparser_report_diagnostic(
    textparser_t handle,
    textparser_diagnostic_severity severity,
    const char *code,
    const char *message,
    size_t start_pos,
    size_t length)
{
    if (handle == nullptr || message == nullptr) {
        return -1;
    }

    if (handle->diagnostic_count >= handle->diagnostic_capacity) {
        size_t new_cap = handle->diagnostic_capacity == 0 ? 8 : handle->diagnostic_capacity * 2;
        textparser_diagnostic *new_diag = realloc(handle->diagnostics, new_cap * sizeof(textparser_diagnostic));
        if (new_diag == nullptr) {
            return -1;
        }
        handle->diagnostics = new_diag;
        handle->diagnostic_capacity = new_cap;
    }

    textparser_diagnostic *diag = &handle->diagnostics[handle->diagnostic_count++];
    diag->severity = severity;
    diag->code = code ? strdup(code) : nullptr;
    diag->message = strdup(message);
    diag->start_pos = start_pos;
    diag->length = length;
    
    if (handle->lines == nullptr && handle->text_addr != nullptr && handle->text_size > 0) {
        textparser_build_line_map(handle);
    }
    size_t line_no = textparser_get_line_number_at_position(handle, start_pos);
    size_t line_start = textparser_get_line_start_position(handle, line_no);
    diag->line = (uint32_t)line_no;
    diag->column = (start_pos >= line_start) ? (uint32_t)(start_pos - line_start) : 0;

    return 0;
}

EXPORT_TEXTPARSER size_t textparser_get_diagnostic_count(textparser_t handle)
{
    return handle ? handle->diagnostic_count : 0;
}

EXPORT_TEXTPARSER int textparser_get_diagnostic(
    textparser_t handle,
    size_t index,
    textparser_diagnostic *out_diagnostic)
{
    if (handle == nullptr || out_diagnostic == nullptr || index >= handle->diagnostic_count) {
        return -1;
    }

    *out_diagnostic = handle->diagnostics[index];
    return 0;
}

EXPORT_TEXTPARSER void textparser_clear_diagnostics(textparser_t handle)
{
    if (handle == nullptr || handle->diagnostics == nullptr) return;

    for (size_t d = 0; d < handle->diagnostic_count; d++) {
        if (handle->diagnostics[d].code) free((void *)handle->diagnostics[d].code);
        if (handle->diagnostics[d].message) free((void *)handle->diagnostics[d].message);
    }
    handle->diagnostic_count = 0;
}

EXPORT_TEXTPARSER int textparser_recover_until_token(
    textparser_t handle,
    const int *sync_tokens,
    size_t current_offset,
    size_t *out_new_offset)
{
    if (handle == nullptr || sync_tokens == nullptr || out_new_offset == nullptr) {
        return -1;
    }

    size_t total_units = textparser_get_total_units(handle);
    size_t offset = current_offset;

    while (offset < total_units) {
        for (int i = 0; sync_tokens[i] != TextParser_END && sync_tokens[i] != -1; i++) {
            ssize_t found = textparser_find_token(handle, sync_tokens[i], offset, false, nullptr, nullptr);
            if (found == 0) {
                *out_new_offset = offset;
                return 0;
            }
        }
        size_t char_len = textparser_char_len(handle, offset);
        if (char_len == 0) char_len = 1;
        offset += char_len;
    }

    *out_new_offset = total_units;
    return -1; // Reached EOF without matching sync token
}
