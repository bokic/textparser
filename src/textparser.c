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

#define exit_with_error(handle, error_text, offset)   \
    LOGE("Error: %s at %zu", error_text, offset);     \
    if(handle) (handle)->error = error_text;          \
    if(handle) (handle)->error_offset = offset;       \
    goto exit;                                        \

#define check_and_exit_on_fatal_parsing_error(handle, child, offset)                          \
    if ((handle)->error) {                                                                    \
        LOGW("Fatal error detected(%s) at offset %zu. exiting..", (handle)->error, offset);   \
        goto exit;                                                                            \
    }                                                                                         \
    if ((child)->len == 0) {                                                                  \
        LOGW("child->len == 0 detected(%s) at offset %zu. exiting..", (handle)->error ? (handle)->error : "none", offset); \
        exit_with_error(handle, "infinite loop due to 0-length token", offset);               \
    }

#define check_and_exit_on_fatal_parsing_error_start_stop(handle, child, offset)                \
    if ((handle)->error) {                                                                    \
        LOGW("Fatal error detected(%s) at offset %zu. exiting..", (handle)->error, offset);   \
        goto exit;                                                                            \
    }

static size_t calculate_chunk_size(size_t text_size);
static size_t textparser_skip_whitespace(const struct textparser_handle *handle, size_t pos);

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

typedef struct textparser_lexer_cache_entry {
    size_t source_offset;
    int source_rule;
    char *mode;
    char *goal;
    textparser_lex_token token;
    struct textparser_lexer_cache_entry *next;
} textparser_lexer_cache_entry;

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
    const char *error;
    size_t token_count;
    const char *text_addr;
    size_t text_size;
    size_t no_lines;
    size_t *lines;
    // Arena allocator fields
    void **chunks;
    size_t chunk_count;
    size_t chunk_capacity;
    size_t chunk_size; // size of chunk (for new allocations)
    void *current_chunk;
    size_t current_chunk_index;
    size_t current_chunk_used;
    void (*callback)(textparser_t, textparser_token_item *, enum textparser_callback_type callback_type, void *user_data);
    void *user_data;
    int recursion_depth;
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

    /* Phase 6: Multi-Diagnostic Vector */
    textparser_diagnostic *diagnostics;
    size_t diagnostic_count;
    size_t diagnostic_capacity;

    /* Immutable lexer snapshot for the latest successful parse. */
    textparser_lex_token *lexer_tokens;
    size_t lexer_token_count;
    textparser_lex_trivia *lexer_trivia;
    size_t lexer_trivia_count;
    textparser_lexer_cache_entry *lexer_cache;

    /* Shared transactional state used by all grammar operations. */
    textparser_parser_runtime parser;
};

static void textparser_clear_lexer_streams(struct textparser_handle *handle)
{
    if (handle == nullptr) return;
    free(handle->lexer_tokens);
    free(handle->lexer_trivia);
    handle->lexer_tokens = nullptr;
    handle->lexer_token_count = 0;
    handle->lexer_trivia = nullptr;
    handle->lexer_trivia_count = 0;
    textparser_lexer_cache_entry *entry = handle->lexer_cache;
    while (entry != nullptr) {
        textparser_lexer_cache_entry *next = entry->next;
        free(entry->mode);
        free(entry->goal);
        free(entry);
        entry = next;
    }
    handle->lexer_cache = nullptr;
}

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

static size_t textparser_get_byte_len(const struct textparser_handle *handle, size_t len)
{
    return textparser_get_byte_offset(handle, len);
}

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

static size_t textparser_get_end_of_line_units(const struct textparser_handle *handle, size_t pos)
{
    size_t total = textparser_get_total_units(handle);
    size_t cur = pos;
    while (cur < total) {
        uint32_t ch = textparser_get_unit_at(handle, cur);
        if (ch == '\n' || ch == '\r')
            break;
        cur++;
    }
    return cur - pos;
}

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

static void free_arena(struct textparser_handle *handle)
{
    if (handle->chunks) {
        for (size_t i = 0; i < handle->chunk_count; i++) {
            free(handle->chunks[i]);
        }
        free(handle->chunks);
        handle->chunks = nullptr;
    }
    handle->chunk_count = 0;
    handle->chunk_capacity = 0;
    handle->current_chunk = nullptr;
    handle->current_chunk_index = 0;
    handle->current_chunk_used = 0;
}

typedef struct {
    size_t chunk_index;
    size_t chunk_used;
    size_t chunk_count;
    size_t token_count;
    uint64_t next_node_id;
} textparser_arena_checkpoint;

static inline textparser_arena_checkpoint textparser_arena_checkpoint_save(const struct textparser_handle *handle)
{
    textparser_arena_checkpoint cp;
    cp.chunk_index = handle->current_chunk_index;
    cp.chunk_used = handle->current_chunk_used;
    cp.chunk_count = handle->chunk_count;
    cp.token_count = handle->token_count;
    cp.next_node_id = handle->next_node_id;
    return cp;
}

static inline void textparser_arena_checkpoint_restore(struct textparser_handle *handle, const textparser_arena_checkpoint *cp)
{
    handle->current_chunk_index = cp->chunk_index;
    if (handle->chunks && cp->chunk_index < handle->chunk_count) {
        handle->current_chunk = handle->chunks[cp->chunk_index];
    } else {
        handle->current_chunk = nullptr;
    }
    handle->current_chunk_used = cp->chunk_used;
    handle->token_count = cp->token_count;
    handle->next_node_id = cp->next_node_id;
}

static inline bool textparser_match_start_token(
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
        ret = adv_regex_find_pattern_ctx(
            handle->regex_ctx,
            token_def->start_regex,
            (void **)handle->start_regex + token_id,
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

static textparser_token_item *textparser_alloc_token(struct textparser_handle *handle, int token_id, size_t len)
{
    size_t token_size = sizeof(textparser_token_item);
    if (handle->current_chunk == nullptr ||
        handle->current_chunk_used + token_size > handle->chunk_size)
    {
        if (handle->current_chunk != nullptr && handle->current_chunk_index + 1 < handle->chunk_count) {
            handle->current_chunk_index++;
            handle->current_chunk = handle->chunks[handle->current_chunk_index];
            handle->current_chunk_used = 0;
            memset(handle->current_chunk, 0, handle->chunk_size);
        } else {
            void *new_chunk = malloc(handle->chunk_size);
            if (new_chunk == nullptr) {
                handle->error = "Can't allocate memory!";
                return nullptr;
            }
            memset(new_chunk, 0, handle->chunk_size);

            if (handle->chunk_count >= handle->chunk_capacity) {
                size_t new_capacity = handle->chunk_capacity == 0 ? 4 : handle->chunk_capacity * 2;
                void **new_chunks = realloc(handle->chunks, new_capacity * sizeof(void *));
                if (new_chunks == nullptr) {
                    free(new_chunk);
                    handle->error = "Can't allocate memory!";
                    return nullptr;
                }
                handle->chunks = new_chunks;
                handle->chunk_capacity = new_capacity;
            }

            handle->chunks[handle->chunk_count] = new_chunk;
            handle->current_chunk = new_chunk;
            handle->current_chunk_index = handle->chunk_count;
            handle->chunk_count++;
            handle->current_chunk_used = 0;
        }
    }

    textparser_token_item *ret = (textparser_token_item *)((char *)handle->current_chunk + handle->current_chunk_used);
    handle->current_chunk_used += token_size;
    memset(ret, 0, token_size);

    ret->id = ++handle->next_node_id;
    ret->token_id = token_id;
    ret->len = len;
    ret->text_color = TEXTPARSER_NOCOLOR;
    ret->text_background = TEXTPARSER_NOCOLOR;
    return ret;
}

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


static bool textparser_token_in_id_list(const int *list, int token_id)
{
    if (list == nullptr) return false;
    for (int i = 0; list[i] != TextParser_END; i++) {
        if (list[i] == token_id) return true;
    }
    return false;
}

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
            if (textparser_match_start_token(handle, token_id, text, len, &found_at, nullptr, true)) {
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
            bool found_start = textparser_match_start_token(handle, parent_token_id, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, &token_start, &start_len, true);
            if (found_start && token_start == 0)
            {
                return true;
            }
        }
    }

    return false;
}

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

static void append_whitespace_if_needed(
    struct textparser_handle *handle,
    textparser_token_item *parent,
    textparser_token_item **head,
    textparser_token_item **tail,
    size_t len)
{
    if (len == 0) return;
    if (*tail != nullptr && (*tail)->token_id == TEXTPARSER_TOKEN_ID_WHITESPACE)
    {
        (*tail)->len += len;
        return;
    }
    textparser_token_item *item = textparser_alloc_token(handle, TEXTPARSER_TOKEN_ID_WHITESPACE, len);
    if (item == nullptr) return;
    append_child_to_ast(parent, head, tail, item);
}

static void append_unprocessed_if_needed(
    struct textparser_handle *handle,
    textparser_token_item *parent,
    textparser_token_item **head,
    textparser_token_item **tail,
    size_t len)
{
    if (len == 0) return;
    if (*tail != nullptr && (*tail)->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED)
    {
        (*tail)->len += len;
        return;
    }
    textparser_token_item *item = textparser_alloc_token(handle, TEXTPARSER_TOKEN_ID_UNPROCESSED, len);
    if (item == nullptr) return;
    append_child_to_ast(parent, head, tail, item);
}

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
        exit_with_error(handle, "group_one_child token type nested_tokens list is empty!", offset);
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
            exit_with_error(handle, "Search for group_one_child token type failed. Can't find one child.", offset);
        }

        if (closest > 0) {
            append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, closest);
            offset += closest;
        }

        child = textparser_parse_token(handle, current_token_id, parent_token_id, parent_start_stop, offset, ret, last_child);
        if (child == nullptr) {
            exit_with_error(handle, "Search for group_one_child token type failed. Child token parsing failed.", offset);
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
        exit_with_error(handle, "nested_tokens list is empty!", offset);
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
            exit_with_error(handle, "Search for group token type failed. Can't find any child.", offset);
        }

        const textparser_token_item *current_prev = child;

        if (check_parent_token_boundary(handle, parent_token_id, parent_start_stop, offset)) {
            ret->len = offset - start_offset;
            break;
        }

        const int *loop_effective_nested = get_effective_nested_tokens(handle, token_id, ret);
        textparser_token_item *new_child = nullptr;
        textparser_token_item *error_child = nullptr;
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
                        error_child = attempt;
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
        }

        if (new_child != nullptr)
        {
            if (handle->error) {
                if (token_def->other_text_inside && offset < textparser_get_total_units(handle)) {
                    handle->error = nullptr;
                    handle->error_offset = 0;
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
                exit_with_error(handle, "0-length child token match caused infinite loop", offset);
            }

            offset += child_advance;
            ret->len = offset - start_offset;
        }
        else
        {
            if (token_def->other_text_inside && offset < textparser_get_total_units(handle))
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
                exit_with_error(handle, "Unrecognized token inside group", offset);
            }
        }
    }

    if (handle->callback) {
        handle->callback(handle, ret, TEXTPARSER_CALLBACK_TYPE_END, handle->user_data);
    }

exit:
    return ret;
}

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
        exit_with_error(handle, "nested_tokens list is empty!", offset);
    }

    int nested_count = 0;
    while(token_def->nested_tokens[nested_count] != TextParser_END) nested_count++;

    if (nested_count != 3) {
         exit_with_error(handle, "GroupAllChildrenInSameOrder should have exactly 3 nested tokens", offset);
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
        exit_with_error(handle, "Expected start token!", offset);
    }

    child = textparser_parse_token(handle, start_token_id, parent_token_id, parent_start_stop, offset, ret, prev_sibling);
    if (child == nullptr) {
        exit_with_error(handle, "Parsing start token failed", offset);
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
            exit_with_error(handle, "Expected end token, reached end of text!", offset);
        }

        ssize_t end_pos   = textparser_find_token(handle, end_token_id,   offset, definition->other_text_inside, ret, last_child);
        if (end_pos == 0) {
            break;
        }

        ssize_t inner_pos = textparser_find_token(handle, inner_token_id, offset, definition->other_text_inside, ret, last_child);
        if (inner_pos == 0) {
            child = textparser_parse_token(handle, inner_token_id, end_token_id, TEXTPARSER_SEARCH_START_TOKEN, offset, ret, last_child);
            if (child == nullptr) {
                exit_with_error(handle, "Parsing inner token failed", offset);
            }
            size_t child_advance = child->len;
            append_child_to_ast(ret, &ret->child, &last_child, child);
            maybe_merge_sign(handle, child);
            check_and_exit_on_fatal_parsing_error(handle, child, offset);

            if (child->len == 0) {
                exit_with_error(handle, "0-length child token match caused infinite loop", offset);
            }

            offset += child_advance;
            continue;
        }

        if (definition->other_text_inside) {
            size_t char_l = textparser_char_len(handle, offset);
            append_unprocessed_if_needed(handle, ret, &ret->child, &last_child, char_l);
            offset += char_l;
        } else {
            exit_with_error(handle, "Expected inner or end token!", offset);
        }
    }

    size_t ws_skipped_end = textparser_skip_whitespace(handle, offset) - offset;
    if (ws_skipped_end > 0) {
        append_whitespace_if_needed(handle, ret, &ret->child, &last_child, ws_skipped_end);
        offset += ws_skipped_end;
    }

    child = textparser_parse_token(handle, end_token_id, parent_token_id, parent_start_stop, offset, ret, last_child);
    if (child == nullptr) {
        exit_with_error(handle, "Parsing end token failed", offset);
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
        exit_with_error(handle, "offset >= total units count!", offset);
    }

    ret = textparser_alloc_token(handle, token_id, 0);
    if (ret == nullptr) {
        exit_with_error(handle, "Can't allocate memory!", offset);
    }

    size_t len = 0;
    if (!textparser_match_start_token(handle, token_id, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &len, true)) {
        exit_with_error(handle, "Can't find start of the token!", offset);
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
        exit_with_error(handle, "handle == nullptr!", offset);
    }

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];

    if (stop_required) {
        LOGV("enter TEXTPARSER_TOKEN_TYPE_START_STOP");
    } else {
        LOGV("enter TEXTPARSER_TOKEN_TYPE_START_OPT_STOP");
    }

    if (offset >= textparser_get_total_units(handle)) {
        exit_with_error(handle, "offset >= total units count!", offset);
    }

    size_t start_offset = offset;

    ret = textparser_alloc_token(handle, token_id, 0);
    if (ret == nullptr) {
        exit_with_error(handle, "Can't allocate memory!", offset);
    }

    ret->parent = (textparser_token_item *)parent_item;
    ret->prev = (textparser_token_item *)prev_sibling;

    // Search for start token
    if (!textparser_match_start_token(handle, token_id, handle->text_addr + textparser_get_byte_offset(handle, offset), textparser_get_total_units(handle) - offset, nullptr, &len, true)) {
        exit_with_error(handle, "Can't find start of the token!", offset);
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
        exit_with_error(handle, "offset >= total units count!", offset);
    }

    if (offset == textparser_get_total_units(handle)) {
        if (token_def->type == TEXTPARSER_TOKEN_TYPE_START_STOP) {
            exit_with_error(handle, "reached end of text!", offset);
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
                    exit_with_error(handle, "Reached end of text before finding end token!", offset);
                } else {
                    break;
                }
            }

            const textparser_token_item *current_prev = last_child;
            textparser_token_item *new_child = nullptr;
            textparser_token_item *error_child = nullptr;
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
                            error_child = attempt;
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
            }

            if (new_child != nullptr)
            {
                if (handle->error) {
                    if (token_def->other_text_inside && offset < textparser_get_total_units(handle)) {
                        handle->error = nullptr;
                        handle->error_offset = 0;
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
                    exit_with_error(handle, "0-length child token match caused infinite loop", offset);
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
                exit_with_error(handle, "Unexpected token inside start-stop block!", offset);
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
            exit_with_error(handle, "Can't find end of the token!", offset);
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

static textparser_token_item *parse_token_error_error(struct textparser_handle *handle, const char *msg, size_t offset)
{
    exit_with_error(handle, msg, offset);
exit:
    return nullptr;
}

static textparser_token_item *textparser_parse_token(struct textparser_handle *handle, int token_id, int parent_token_id, int parent_start_stop, size_t offset, const textparser_token_item *parent_item, const textparser_token_item *prev_sibling)
{
    if (handle == nullptr) {
        LOGF("handle == nullptr");
        return nullptr;
    }

    textparser_token_item *ret = nullptr;

    if (handle->recursion_depth >= MAX_RECURSION_DEPTH) {
        exit_with_error(handle, "Maximum recursion depth exceeded!", offset);
    }
    handle->recursion_depth++;

    const textparser_language_definition *definition = handle->language;
    const textparser_token *token_def = &definition->tokens[token_id];

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
            exit_with_error(handle, "Token spans multiple lines but multi_line flag is not set!", offset);
        }

        if (token_def->must_have_one_child && textparser_get_semantic_children_count(ret) != 1) {
            exit_with_error(handle, "Token must have exactly one child token!", offset);
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
    return ret;
}

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
    local_hnd.chunk_size = calculate_chunk_size(local_hnd.text_size);

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
    ret->chunk_size = calculate_chunk_size(ret->text_size);
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

static const textparser_token_item *find_token_at_position_internal(const textparser_token_item *token, size_t position, int depth)
{
    if (depth >= MAX_RECURSION_DEPTH || token == nullptr) {
        return nullptr;
    }
    const textparser_token_item *best = nullptr;
    size_t curr_pos = (token->parent != nullptr) ? textparser_get_token_position(token) : 0;
    while (token != nullptr) {
        if (position >= curr_pos && position < curr_pos + token->len) {
            best = token;
            if (token->child != nullptr) {
                const textparser_token_item *child_match = find_token_at_position_internal(token->child, position, depth + 1);
                if (child_match) {
                    best = child_match;
                }
            }
            break;
        }
        curr_pos += token->len;
        token = token->next;
    }
    return best;
}


static const textparser_token_item *find_open_container(const textparser_token_item *token, size_t position, const textparser_language_definition *definition)
{
    const textparser_token_item *curr = token;
    while (curr != nullptr) {
        if (curr->token_id >= 0) {
            const textparser_token *def = &definition->tokens[curr->token_id];
            if (def->type == TEXTPARSER_TOKEN_TYPE_START_STOP ||
                def->type == TEXTPARSER_TOKEN_TYPE_START_OPT_STOP ||
                def->type == TEXTPARSER_TOKEN_TYPE_GROUP ||
                def->type == TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER ||
                def->type == TEXTPARSER_TOKEN_TYPE_SEQUENCE) {
                size_t curr_pos = textparser_get_token_position(curr);
                if (position >= curr_pos && position < curr_pos + curr->len) {
                    return curr;
                }
            }
        }
        curr = curr->parent;
    }
    return nullptr;
}

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
} textparser_lexer_stream_builder;

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
            token->mode = 0;
            token->lexical_goal = 0;
            token->flags = 0;
            for (size_t i = builder->pending_trivia_start; i < builder->trivia_count; i++) {
                token->flags |= builder->trivia[i].flags;
            }
            token->decoded_value = curr->decoded_value;
            builder->pending_trivia_start = builder->trivia_count;
        }
        pos = end;
        curr = curr->next;
    }
    return true;
}

static int textparser_rebuild_lexer_streams(struct textparser_handle *handle)
{
    textparser_lexer_stream_builder builder = {0};
    if (!textparser_collect_lexer_streams(handle, handle->first_item, 0, &builder)) {
        free(builder.tokens);
        free(builder.trivia);
        textparser_clear_lexer_streams(handle);
        return TEXTPARSER_ERROR_OUT_OF_MEMORY;
    }

    textparser_clear_lexer_streams(handle);
    handle->lexer_tokens = builder.tokens;
    handle->lexer_token_count = builder.token_count;
    handle->lexer_trivia = builder.trivia;
    handle->lexer_trivia_count = builder.trivia_count;
    handle->parser.source_offset = textparser_get_total_units(handle);
    handle->parser.token_index = builder.token_count;
    return TEXTPARSER_OK;
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

    textparser_clear_lexer_streams(handle);
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

    // If doing a full parse from offset 0 to EOF, reset existing arena tree
    if (start_pos == 0 && end_pos >= total)
    {
        free_arena(handle);
        handle->first_item = nullptr;
    }

    if (handle->language != definition)
    {
        textparser_free_regex(handle);
        handle->language = definition;
        if (textparser_init_regex(handle) != 0)
            return -1;
    }

    if (handle->regex_ctx) {
        adv_regex_set_utf8_valid(handle->regex_ctx, textparser_validate_text_encoding(handle));
    }

    // Resolve active token from existing AST
    const textparser_token_item *active_token = nullptr;
    if (handle->first_item != nullptr && start_pos > 0) {
        active_token = find_token_at_position_internal(handle->first_item, start_pos - 1, 0);
    }

    const textparser_token_item *open_container = nullptr;
    if (active_token != nullptr) {
        open_container = find_open_container(active_token, start_pos, definition);
    }

    const int *effective_starts_with = definition->starts_with;
    textparser_token_item *parent_container = (textparser_token_item *)open_container;

    if (open_container) {
        effective_starts_with = get_effective_nested_tokens(handle, open_container->token_id, open_container);
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

    textparser_token_item *prev_item = nullptr;
    textparser_token_item *sibling_list = parent_container ? parent_container->child : handle->first_item;

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

    size_t pos = start_pos;
    textparser_token_item *first_new_token = nullptr;
    textparser_token_item *last_new_token = nullptr;

    while(pos < end_pos) {
        size_t ws_skipped = textparser_skip_whitespace(handle, pos) - pos;
        if (ws_skipped > 0) {
            textparser_token_item **head_ptr = parent_container ? &parent_container->child : &handle->first_item;
            append_whitespace_if_needed(handle, parent_container, head_ptr, &prev_item, ws_skipped);
            if (first_new_token == nullptr) first_new_token = prev_item;
            last_new_token = prev_item;
            pos += ws_skipped;
        }
        if (pos >= end_pos)
            break;

        textparser_token_item *token_item = nullptr;
        textparser_token_item *error_token_item = nullptr;
        for (int c = 0; effective_starts_with && effective_starts_with[c] != TextParser_END; c++) {
            int token_id = effective_starts_with[c];
            ssize_t offset = textparser_find_token(handle, token_id, pos, definition->other_text_inside, parent_container, prev_item);
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
                    error_token_item = attempt;
                } else {
                    textparser_arena_checkpoint_restore(handle, &cp);
                    handle->error = saved_err;
                    handle->error_offset = saved_err_off;
                }
            }
        }

        if (token_item == nullptr && error_token_item != nullptr) {
            token_item = error_token_item;
        }

        if (token_item != nullptr) {
            if (parent_container) {
                token_item->parent = parent_container;
            }

            if (first_new_token == nullptr)
                first_new_token = token_item;

            if (prev_item) {
                prev_item->next = token_item;
                token_item->prev = prev_item;
            } else if (parent_container) {
                parent_container->child = token_item;
            } else {
                handle->first_item = token_item;
            }

            size_t token_advance = token_item->len;
            maybe_merge_sign(handle, token_item);

            if ((handle->error)||(token_item->len <= 0))
                return -1;

            pos += token_advance;
            prev_item = token_item;
            last_new_token = token_item;
        } else {
            if (definition->other_text_inside) {
                size_t char_l = textparser_char_len(handle, pos);
                textparser_token_item **head_ptr = parent_container ? &parent_container->child : &handle->first_item;
                append_unprocessed_if_needed(handle, parent_container, head_ptr, &prev_item, char_l);
                if (first_new_token == nullptr) first_new_token = prev_item;
                last_new_token = prev_item;
                pos += char_l;
            } else {
                break;
            }
        }
    }

    if (pos < end_pos) {
        textparser_token_item **head_ptr = parent_container ? &parent_container->child : &handle->first_item;
        append_unprocessed_if_needed(handle, parent_container, head_ptr, &prev_item, end_pos - pos);
        if (first_new_token == nullptr) first_new_token = prev_item;
        last_new_token = prev_item;
        pos = end_pos;
    }

    textparser_token_item *stitch_point = last_new_token ? last_new_token : prev_item;
    if (tail_first) {
        if (stitch_point) {
            stitch_point->next = tail_first;
            tail_first->prev = stitch_point;
        } else if (parent_container) {
            parent_container->child = tail_first;
            tail_first->prev = nullptr;
        } else {
            handle->first_item = tail_first;
            tail_first->prev = nullptr;
        }
    } else if (stitch_point) {
        stitch_point->next = nullptr;
    } else if (parent_container) {
        parent_container->child = nullptr;
    } else {
        handle->first_item = nullptr;
    }

    if (parent_container && delta_units != 0) {
        textparser_token_item *p = parent_container;
        while (p) {
            p->len = (size_t)((ssize_t)p->len + delta_units);
            p = p->parent;
        }
    }

    if (out_range != nullptr) {
        size_t d_start = start_pos;
        if (first_new_token != nullptr) {
            d_start = textparser_get_token_position(first_new_token);
        } else if (prev_item != nullptr) {
            d_start = textparser_get_token_position(prev_item);
        }
        size_t d_end = end_pos;
        if (tail_first != nullptr) {
            d_end = textparser_get_token_position(tail_first);
        } else {
            d_end = textparser_get_total_units(handle);
        }
        out_range->dirty_start = d_start;
        out_range->dirty_end = d_end;
    }

    if (handle->lines && (delta_units != 0 || start_pos == 0)) {
        free(handle->lines);
        handle->lines = nullptr;
        handle->no_lines = 0;
    }

    return textparser_rebuild_lexer_streams(handle);
}

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

static textparser_token_item *previous_significant_token(textparser_token_item *token)
{
    if (token != nullptr) token = token->prev;
    while (token != nullptr && is_trivia_token_id(token->token_id)) token = token->prev;
    return token;
}

static textparser_token_item *next_significant_token(textparser_token_item *token)
{
    if (token != nullptr) token = token->next;
    while (token != nullptr && is_trivia_token_id(token->token_id)) token = token->next;
    return token;
}

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
}

EXPORT_TEXTPARSER const char *textparser_parse_error(textparser_t handle)
{
    if (handle == nullptr)
        return nullptr;

    return handle->error;
}

EXPORT_TEXTPARSER size_t textparser_parse_error_position(textparser_t handle)
{
    if (handle == nullptr)
        return 0;

    return handle->error_offset;
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

static bool textparser_name_ends_with(const char *name, const char *suffix)
{
    if (name == nullptr || suffix == nullptr) return false;
    size_t name_length = strlen(name), suffix_length = strlen(suffix);
    return name_length >= suffix_length &&
        strcmp(name + name_length - suffix_length, suffix) == 0;
}

EXPORT_TEXTPARSER textparser_typescript_cst_category textparser_typescript_cst_category_of(
    const textparser_t handle,
    const textparser_node *node)
{
    if (handle == nullptr || node == nullptr || handle->language == nullptr ||
        handle->language->name == nullptr || strcmp(handle->language->name, "typescript") != 0)
        return TEXTPARSER_TS_CST_UNKNOWN;
    textparser_cst_node_view view = {0};
    if (textparser_get_cst_node_view(handle, node, &view) != 0 || view.kind == nullptr)
        return TEXTPARSER_TS_CST_UNKNOWN;
    if (view.terminal) return TEXTPARSER_TS_CST_TOKEN;
    const char *name = view.kind;
    if (strcmp(name, "SourceFile") == 0) return TEXTPARSER_TS_CST_SOURCE_FILE;
    if (strncmp(name, "JSX", 3) == 0) return TEXTPARSER_TS_CST_JSX;
    if (textparser_name_ends_with(name, "Declaration") ||
        strcmp(name, "VariableStatement") == 0 ||
        strcmp(name, "VariableDeclarationList") == 0 ||
        strcmp(name, "ClassElement") == 0 || strcmp(name, "EnumMember") == 0)
        return TEXTPARSER_TS_CST_DECLARATION;
    if (textparser_name_ends_with(name, "Statement") ||
        strcmp(name, "Statement") == 0 || strcmp(name, "StatementList") == 0 ||
        textparser_name_ends_with(name, "Clause"))
        return TEXTPARSER_TS_CST_STATEMENT;
    if (strstr(name, "Binding") != nullptr || strstr(name, "AssignmentTarget") != nullptr)
        return TEXTPARSER_TS_CST_PATTERN;
    if (textparser_name_ends_with(name, "Type") || strstr(name, "TypeParameter") != nullptr ||
        strcmp(name, "Type") == 0 || strcmp(name, "TypeAnnotation") == 0 ||
        strcmp(name, "TypeArguments") == 0 || strcmp(name, "TypeMember") == 0)
        return TEXTPARSER_TS_CST_TYPE;
    if (textparser_name_ends_with(name, "Expression") ||
        strcmp(name, "Expression") == 0 || strcmp(name, "PrimaryExpression") == 0 ||
        strcmp(name, "Arguments") == 0 || (node->child != nullptr &&
            (node->node_flags & TEXTPARSER_NODE_SYNTHETIC) == 0))
        return TEXTPARSER_TS_CST_EXPRESSION;
    return TEXTPARSER_TS_CST_OTHER;
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
    *out_count = handle ? handle->lexer_token_count : 0;
    return handle ? handle->lexer_tokens : nullptr;
}

EXPORT_TEXTPARSER const textparser_lex_trivia *textparser_get_lexer_trivia(
    const textparser_t handle,
    size_t *out_count)
{
    if (out_count == nullptr) return nullptr;
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

static bool textparser_id_in_list(const int *ids, int id)
{
    if (ids == nullptr) return false;
    for (size_t i = 0; ids[i] >= 0; i++) if (ids[i] == id) return true;
    return false;
}

static bool textparser_typescript_identifier_escapes_valid(
    struct textparser_handle *handle, const char *text, size_t length)
{
    if (handle == nullptr || text == nullptr || length == 0) return false;
    char *decoded = malloc(length + 1);
    if (decoded == nullptr) return false;
    size_t i = text[0] == '#' ? 1 : 0;
    size_t decoded_length = 0;
    while (i < length) {
        if (text[i] != '\\') {
            unsigned char ch = (unsigned char)text[i];
            size_t width = ch < 0x80 ? 1 :
                ((ch & 0xe0) == 0xc0 ? 2 : ((ch & 0xf0) == 0xe0 ? 3 : 4));
            if (i + width > length) { free(decoded); return false; }
            memcpy(decoded + decoded_length, text + i, width);
            decoded_length += width;
            i += width;
            continue;
        }
        if (i + 2 >= length || text[i + 1] != 'u') { free(decoded); return false; }
        i += 2;
        bool braced = i < length && text[i] == '{';
        if (braced) i++;
        uint32_t value = 0;
        size_t digits = 0;
        while (i < length && digits < (braced ? 6u : 4u)) {
            unsigned char ch = (unsigned char)text[i];
            unsigned digit;
            if (ch >= '0' && ch <= '9') digit = ch - '0';
            else if (ch >= 'a' && ch <= 'f') digit = ch - 'a' + 10;
            else if (ch >= 'A' && ch <= 'F') digit = ch - 'A' + 10;
            else break;
            value = value * 16 + digit;
            i++; digits++;
        }
        if ((!braced && digits != 4) || (braced &&
            (digits == 0 || i >= length || text[i++] != '}')) ||
            value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) {
            free(decoded);
            return false;
        }
        if (value <= 0x7f) decoded[decoded_length++] = (char)value;
        else if (value <= 0x7ff) {
            decoded[decoded_length++] = (char)(0xc0 | (value >> 6));
            decoded[decoded_length++] = (char)(0x80 | (value & 0x3f));
        } else if (value <= 0xffff) {
            decoded[decoded_length++] = (char)(0xe0 | (value >> 12));
            decoded[decoded_length++] = (char)(0x80 | ((value >> 6) & 0x3f));
            decoded[decoded_length++] = (char)(0x80 | (value & 0x3f));
        } else {
            decoded[decoded_length++] = (char)(0xf0 | (value >> 18));
            decoded[decoded_length++] = (char)(0x80 | ((value >> 12) & 0x3f));
            decoded[decoded_length++] = (char)(0x80 | ((value >> 6) & 0x3f));
            decoded[decoded_length++] = (char)(0x80 | (value & 0x3f));
        }
    }
    decoded[decoded_length] = '\0';
    static const char *identifier_pattern =
        "(?:[$_]|\\p{ID_Start})(?:[$\\x{200C}\\x{200D}]|\\p{ID_Continue})*";
    void *regex = nullptr;
    size_t found_at = 0, found_length = 0;
    bool valid = adv_regex_find_pattern_ctx(
        handle->regex_ctx, identifier_pattern, &regex, TEXTPARSER_ENCODING_UTF_8,
        decoded, decoded_length, &found_at, &found_length, false, true) &&
        found_at == 0 && found_length == decoded_length;
    adv_regex_free(handle->regex_ctx, &regex, TEXTPARSER_ENCODING_UTF_8);
    free(decoded);
    return valid;
}

static bool textparser_contextual_match(
    struct textparser_handle *handle,
    int token_id,
    size_t offset,
    size_t *length)
{
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
            total - offset, &found_at, &found_len, true) || found_at != 0 || found_len == 0) {
        return false;
    }
    *length = found_len;
    if (rule->name != nullptr &&
        (strcmp(rule->name, "Identifier") == 0 ||
         strcmp(rule->name, "PrivateIdentifier") == 0) &&
        memchr(handle->text_addr + textparser_get_byte_offset(handle, offset), '\\', found_len) != nullptr &&
        !textparser_typescript_identifier_escapes_valid(handle,
            handle->text_addr + textparser_get_byte_offset(handle, offset), found_len))
        return false;
    return true;
}

static int textparser_contextual_scan_one(
    struct textparser_handle *handle,
    size_t source_offset,
    const char *mode_name,
    const char *goal_name,
    const textparser_lex_token **out_token,
    int *out_source_rule)
{
    *out_token = nullptr;
    if (out_source_rule != nullptr) *out_source_rule = -1;
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
        flags |= textparser_lexer_span_flags(handle, offset, offset + best_len);
        offset += best_len;
    }
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
            handle, offset, mode, goal_name, &token, &source_rule);
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
            handle, handle->parser.source_offset, mode, goal_name, out_token, &source_rule) != 0)
        return -1;
    const textparser_contextual_lexer_rule *source = source_rule >= 0
        ? &handle->language->lexer_rules[source_rule] : nullptr;
    const textparser_contextual_lexer_rule *rule = source != nullptr &&
        (source->pop_mode || source->push_mode != nullptr)
        ? source : &handle->language->lexer_rules[(*out_token)->kind];
    if (rule->pop_mode && textparser_pop_mode(handle) != 0) return -1;
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
} textparser_parser_checkpoint;

static void textparser_free_context_list(textparser_context_entry *context)
{
    while (context != nullptr) {
        textparser_context_entry *next = context->next;
        free(context->name);
        free(context);
        context = next;
    }
}

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

    for (size_t i = cp->arena.chunk_count; i < handle->chunk_count; i++) {
        free(handle->chunks[i]);
        handle->chunks[i] = nullptr;
    }
    handle->chunk_count = cp->arena.chunk_count;
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
    const char *typescript_diagnostic_code;
    const char *typescript_diagnostic_message;
    size_t typescript_diagnostic_start;
    size_t typescript_diagnostic_length;
    size_t initial_diagnostic_count;
    textparser_capture_entry *captures;
} textparser_grammar_executor;

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
    node->cst_kind = production->name != nullptr ? production->name :
        (production->kind >= TEXTPARSER_PROD_TOKEN &&
         production->kind <= TEXTPARSER_PROD_MATCH_CAPTURE
            ? production_kinds[production->kind] : "Production");
    node->source_start = source_start;
    node->source_end = source_start + source_length;
    node->child = first_child;
    for (textparser_node *child = first_child; child != nullptr; child = child->next) {
        child->parent = node;
    }
    return node;
}

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

static void textparser_grammar_note_failure(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    const textparser_lex_token *unexpected = nullptr;
    int peek = textparser_grammar_peek_token(executor, &unexpected);
    bool typescript = executor->handle->language != nullptr &&
        executor->handle->language->name != nullptr &&
        strcmp(executor->handle->language->name, "typescript") == 0;
    size_t offset = typescript
        ? (unexpected != nullptr ? unexpected->start : textparser_get_total_units(executor->handle))
        : executor->handle->parser.source_offset;
    size_t length = typescript && unexpected != nullptr
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

static bool textparser_is_typescript_language(const textparser_t handle)
{
    return handle != nullptr && handle->language != nullptr &&
        handle->language->name != nullptr &&
        strcmp(handle->language->name, "typescript") == 0;
}

static const char *textparser_typescript_token_spelling(const char *name)
{
    if (name == nullptr) return nullptr;
    if (strcmp(name, "LParen") == 0) return "(";
    if (strcmp(name, "RParen") == 0) return ")";
    if (strcmp(name, "LBracket") == 0) return "[";
    if (strcmp(name, "RBracket") == 0) return "]";
    if (strcmp(name, "LBrace") == 0) return "{";
    if (strcmp(name, "RBrace") == 0) return "}";
    if (strcmp(name, "Semicolon") == 0) return ";";
    if (strcmp(name, "Colon") == 0) return ":";
    if (strcmp(name, "Comma") == 0) return ",";
    if (strcmp(name, "GreaterThan") == 0) return ">";
    if (strcmp(name, "Assign") == 0) return "=";
    if (strcmp(name, "Arrow") == 0) return "=>";
    return nullptr;
}

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
    if (textparser_is_typescript_language(executor->handle)) {
        if (recovered) {
            const char *code = "TS1128";
            const char *recovery_message = "Declaration or statement expected.";
            if (production != nullptr && production->name != nullptr) {
                if (strcmp(production->name, "ClassElement") == 0) {
                    code = "TS1068";
                    recovery_message = "Unexpected token. A constructor, method, accessor, or property was expected.";
                } else if (strcmp(production->name, "TypeMember") == 0) {
                    code = "TS1131";
                    recovery_message = "Property or signature expected.";
                } else if (strcmp(production->name, "CaseClause") == 0) {
                    code = "TS1130";
                    recovery_message = "'case' or 'default' expected.";
                }
            }
            return textparser_report_diagnostic(executor->handle, TEXTPARSER_SEVERITY_ERROR,
                code, recovery_message, start, length);
        }
        const char *token_name = nullptr;
        if (production != nullptr && production->kind == TEXTPARSER_PROD_TOKEN &&
            executor->handle->language->tokens != nullptr)
            token_name = executor->handle->language->tokens[production->token_id].name;
        const char *spelling = textparser_typescript_token_spelling(token_name);
        bool expression_expected = production != nullptr &&
            production->expected_description != nullptr &&
            strcmp(production->expected_description, "expression") == 0;
        const char *code = expression_expected ? "TS1109" :
            token_name != nullptr && strcmp(token_name, "Identifier") == 0
                ? "TS1003" : "TS1005";
        if (spelling != nullptr) snprintf(message, sizeof(message), "'%s' expected.", spelling);
        else if (expression_expected) snprintf(message, sizeof(message), "Expression expected.");
        else if (token_name != nullptr && strcmp(token_name, "Identifier") == 0)
            snprintf(message, sizeof(message), "Identifier expected.");
        else snprintf(message, sizeof(message), "Expected %s.", name);
        return textparser_report_diagnostic(executor->handle, TEXTPARSER_SEVERITY_ERROR,
            code, message, start, length);
    }
    snprintf(message, sizeof(message), recovered ? "Recovered while parsing %s." : "Expected %s.", name);
    return textparser_report_diagnostic(executor->handle, TEXTPARSER_SEVERITY_ERROR,
        recovered ? "TEXTPARSER_RECOVERED" : "TEXTPARSER_EXPECTED", message, start, length);
}

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

    /* A TypeScript recovery must reach a real boundary. At EOF there is no
       enclosing list iteration to resume, so preserving the original failure
       gives callers the precise terminal diagnostic instead of accepting an
       incomplete file. A final semicolon is likewise not useful by itself. */
    if (textparser_is_typescript_language(handle)) {
        const textparser_lex_token *boundary = nullptr;
        int boundary_status = textparser_grammar_peek_token(executor, &boundary);
        bool can_resume = boundary_status == 0 && boundary != nullptr &&
            textparser_grammar_is_sync(executor, production, boundary->kind);
        if (can_resume && handle->language != nullptr &&
            strcmp(handle->language->tokens[boundary->kind].name, "Semicolon") == 0) {
            const textparser_lex_token *following = nullptr;
            int following_status = handle->language->initial_lexer_mode != nullptr
                ? textparser_lexer_peek(handle, 1, handle->lexical_goal, &following)
                : (handle->parser.token_index + 1 < handle->lexer_token_count
                    ? (following = &handle->lexer_tokens[handle->parser.token_index + 1], 0) : 1);
            can_resume = following_status == 0 && following != nullptr;
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

typedef enum {
    TEXTPARSER_TS_TARGET_INVALID = 0,
    TEXTPARSER_TS_TARGET_ASSIGNABLE = 1,
    TEXTPARSER_TS_TARGET_OPTIONAL_CHAIN = 2,
} textparser_ts_target_state;

static const char *textparser_grammar_node_production_name(
    const textparser_grammar_executor *executor,
    const textparser_node *node)
{
    if (node == nullptr || (node->node_flags & TEXTPARSER_NODE_SYNTHETIC) == 0) return nullptr;
    const textparser_production *production = textparser_find_production(executor, node->token_id);
    return production == nullptr ? nullptr : production->name;
}

static const char *textparser_grammar_node_token_name(
    const textparser_grammar_executor *executor,
    const textparser_node *node)
{
    if (node == nullptr || (node->node_flags & TEXTPARSER_NODE_SYNTHETIC) != 0 ||
        executor->handle->language == nullptr || executor->handle->language->tokens == nullptr)
        return nullptr;
    for (size_t i = 0; executor->handle->language->tokens[i].name != nullptr; i++)
        if ((int)i == node->token_id) return executor->handle->language->tokens[i].name;
    return nullptr;
}

static const textparser_node *textparser_node_first_terminal(const textparser_node *node)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        if ((item->node_flags & TEXTPARSER_NODE_SYNTHETIC) == 0) return item;
        const textparser_node *nested = textparser_node_first_terminal(item->child);
        if (nested != nullptr) return nested;
    }
    return nullptr;
}

typedef struct textparser_typescript_label_frame {
    const textparser_node *name;
    bool iteration;
    unsigned function_depth;
    const struct textparser_typescript_label_frame *previous;
} textparser_typescript_label_frame;

typedef struct textparser_typescript_legality_context {
    unsigned function_depth;
    unsigned iteration_depth;
    unsigned switch_depth;
    bool async_function;
    bool generator_function;
    bool function_body;
    bool ambient;
    bool ambient_declare_allowed;
    const textparser_typescript_label_frame *labels;
} textparser_typescript_legality_context;

static const textparser_node *textparser_typescript_node_token(
    const textparser_node *node,
    const char *token_name)
{
    if (node == nullptr || token_name == nullptr) return nullptr;
    for (const textparser_node *child = node->child; child != nullptr; child = child->next) {
        if ((child->node_flags & TEXTPARSER_NODE_SYNTHETIC) == 0 &&
            child->cst_kind != nullptr && strcmp(child->cst_kind, token_name) == 0)
            return child;
        const textparser_node *nested = textparser_typescript_node_token(child, token_name);
        if (nested != nullptr) return nested;
    }
    return nullptr;
}

static const textparser_node *textparser_typescript_header_token(
    const textparser_node *node,
    const char *token_name)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        if (item->cst_kind != nullptr &&
            (strcmp(item->cst_kind, "BlockStatement") == 0 ||
             strcmp(item->cst_kind, "ClassBody") == 0 ||
             strcmp(item->cst_kind, "ArrowBody") == 0 ||
             strcmp(item->cst_kind, "BindingParameterList") == 0 ||
             strcmp(item->cst_kind, "TypeParametersContext") == 0 ||
             strcmp(item->cst_kind, "TypeAnnotation") == 0 ||
             strcmp(item->cst_kind, "Decorator") == 0 ||
             strcmp(item->cst_kind, "ClassMemberName") == 0 ||
             strcmp(item->cst_kind, "ObjectPropertyName") == 0))
            continue;
        if ((item->node_flags & TEXTPARSER_NODE_SYNTHETIC) == 0 &&
            item->cst_kind != nullptr && strcmp(item->cst_kind, token_name) == 0)
            return item;
        const textparser_node *nested = textparser_typescript_header_token(
            item->child, token_name);
        if (nested != nullptr) return nested;
    }
    return nullptr;
}

static bool textparser_typescript_subtree_has_header_token(
    const textparser_node *node,
    const char *token_name)
{
    return textparser_typescript_header_token(node, token_name) != nullptr;
}

static bool textparser_typescript_same_identifier(
    textparser_t handle,
    const textparser_node *left,
    const textparser_node *right)
{
    if (handle == nullptr || left == nullptr || right == nullptr ||
        left->source_end < left->source_start || right->source_end < right->source_start)
        return false;
    size_t left_length = left->source_end - left->source_start;
    size_t right_length = right->source_end - right->source_start;
    return left_length == right_length && left->source_end <= handle->text_size &&
        right->source_end <= handle->text_size &&
        memcmp(handle->text_addr + left->source_start,
            handle->text_addr + right->source_start, left_length) == 0;
}

static bool textparser_typescript_label_targets_iteration(const textparser_node *node)
{
    if (node == nullptr) return false;
    const char *kind = node->cst_kind;
    if (kind != nullptr &&
        (strcmp(kind, "IterationStatement") == 0 ||
         strcmp(kind, "WhileStatement") == 0 ||
         strcmp(kind, "DoStatement") == 0 ||
         strcmp(kind, "ForStatement") == 0))
        return true;
    if (kind != nullptr && strcmp(kind, "LabeledStatement") != 0 &&
        strcmp(kind, "Statement") != 0 && strncmp(kind, "Production", 10) != 0)
        return false;
    for (const textparser_node *child = node->child; child != nullptr; child = child->next)
        if ((child->node_flags & TEXTPARSER_NODE_SYNTHETIC) != 0 &&
            textparser_typescript_label_targets_iteration(child))
            return true;
    return false;
}

static const textparser_typescript_label_frame *textparser_typescript_find_label(
    textparser_t handle,
    const textparser_typescript_label_frame *labels,
    const textparser_node *name)
{
    for (const textparser_typescript_label_frame *label = labels;
         label != nullptr; label = label->previous)
        if (textparser_typescript_same_identifier(handle, label->name, name)) return label;
    return nullptr;
}

static const textparser_typescript_label_frame *textparser_typescript_find_local_label(
    textparser_t handle,
    const textparser_typescript_label_frame *labels,
    const textparser_node *name,
    unsigned function_depth)
{
    const textparser_typescript_label_frame *label =
        textparser_typescript_find_label(handle, labels, name);
    return label != nullptr && label->function_depth == function_depth ? label : nullptr;
}

static void textparser_typescript_report_node_diagnostic(
    textparser_t handle,
    const textparser_node *node,
    const char *code,
    const char *message)
{
    const textparser_node *terminal = textparser_node_first_terminal(node == nullptr
        ? nullptr : node->child);
    size_t start = terminal != nullptr ? terminal->source_start
        : node != nullptr ? node->source_start : 0;
    size_t end = terminal != nullptr ? terminal->source_end
        : node != nullptr ? node->source_end : start;
    textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR, code, message,
        start, end >= start ? end - start : 0);
}

static bool textparser_typescript_is_header_boundary(const char *kind)
{
    return kind != nullptr &&
        (strcmp(kind, "BlockStatement") == 0 || strcmp(kind, "ArrowBody") == 0 ||
         strcmp(kind, "ClassBody") == 0 ||
         strcmp(kind, "BindingParameterList") == 0 ||
         strcmp(kind, "TypeParametersContext") == 0 ||
         strcmp(kind, "TypeAnnotation") == 0 || strcmp(kind, "Decorator") == 0 ||
         strcmp(kind, "ClassMemberName") == 0 ||
         strcmp(kind, "ObjectPropertyName") == 0);
}

static unsigned textparser_typescript_modifier_bit(const char *kind, bool *accessibility)
{
    *accessibility = false;
    if (kind == nullptr) return 0;
    if (strcmp(kind, "PublicKeyword") == 0) { *accessibility = true; return 1u << 0; }
    if (strcmp(kind, "PrivateKeyword") == 0) { *accessibility = true; return 1u << 1; }
    if (strcmp(kind, "ProtectedKeyword") == 0) { *accessibility = true; return 1u << 2; }
    if (strcmp(kind, "StaticKeyword") == 0) return 1u << 3;
    if (strcmp(kind, "ReadonlyKeyword") == 0) return 1u << 4;
    if (strcmp(kind, "AbstractKeyword") == 0) return 1u << 5;
    if (strcmp(kind, "OverrideKeyword") == 0) return 1u << 6;
    if (strcmp(kind, "DeclareKeyword") == 0) return 1u << 7;
    if (strcmp(kind, "AccessorKeyword") == 0) return 1u << 8;
    if (strcmp(kind, "AsyncKeyword") == 0) return 1u << 9;
    return 0;
}

static void textparser_typescript_report_modifier_diagnostic(
    textparser_t handle,
    const textparser_node *modifier,
    const char *code,
    const char *suffix)
{
    char spelling[24] = {0};
    size_t length = modifier->source_end - modifier->source_start;
    if (length >= sizeof(spelling)) length = sizeof(spelling) - 1;
    if (modifier->source_start + length <= handle->text_size)
        memcpy(spelling, handle->text_addr + modifier->source_start, length);
    char message[128];
    snprintf(message, sizeof(message), "'%s' modifier %s", spelling, suffix);
    textparser_typescript_report_node_diagnostic(handle, modifier, code, message);
}

static void textparser_typescript_check_modifier_nodes(
    textparser_t handle,
    const textparser_node *node,
    unsigned *seen,
    bool *seen_accessibility)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        if (textparser_typescript_is_header_boundary(item->cst_kind)) continue;
        if ((item->node_flags & TEXTPARSER_NODE_SYNTHETIC) == 0) {
            bool accessibility = false;
            unsigned bit = textparser_typescript_modifier_bit(item->cst_kind, &accessibility);
            if (bit != 0) {
                if (accessibility && *seen_accessibility)
                    textparser_typescript_report_node_diagnostic(handle, item, "TS1028",
                        "Accessibility modifier already seen.");
                else if ((*seen & bit) != 0)
                    textparser_typescript_report_modifier_diagnostic(handle, item, "TS1030",
                        "already seen.");
                *seen |= bit;
                if (accessibility) *seen_accessibility = true;
            }
        }
        textparser_typescript_check_modifier_nodes(
            handle, item->child, seen, seen_accessibility);
    }
}

static size_t textparser_typescript_count_parameters_from_span(
    textparser_t handle,
    const textparser_node *parameter_list)
{
    if (handle == nullptr || parameter_list == nullptr ||
        parameter_list->source_end > handle->text_size ||
        parameter_list->source_end <= parameter_list->source_start + 1) return 0;
    size_t start = parameter_list->source_start + 1;
    size_t end = parameter_list->source_end - 1;
    while (start < end && isspace((unsigned char)handle->text_addr[start])) start++;
    while (end > start && isspace((unsigned char)handle->text_addr[end - 1])) end--;
    if (start == end) return 0;
    size_t count = 1;
    unsigned round = 0, square = 0, brace = 0, angle = 0;
    for (size_t i = start; i < end; i++) {
        char c = handle->text_addr[i];
        if (c == '(') round++; else if (c == ')' && round != 0) round--;
        else if (c == '[') square++; else if (c == ']' && square != 0) square--;
        else if (c == '{') brace++; else if (c == '}' && brace != 0) brace--;
        else if (c == '<') angle++; else if (c == '>' && angle != 0) angle--;
        else if (c == ',' && round == 0 && square == 0 && brace == 0 && angle == 0) count++;
    }
    return count;
}

static bool textparser_typescript_preceded_by_declare(
    textparser_t handle,
    const textparser_node *node)
{
    if (handle == nullptr || node == nullptr) return false;
    size_t start = node->source_start;
    while (start > 0 && handle->text_addr[start - 1] != '\n' &&
        handle->text_addr[start - 1] != '\r' && handle->text_addr[start - 1] != ';' &&
        handle->text_addr[start - 1] != '{' && handle->text_addr[start - 1] != '}') start--;
    static const char word[] = "declare";
    for (size_t i = start; i + sizeof(word) - 1 <= node->source_start; i++)
        if (memcmp(handle->text_addr + i, word, sizeof(word) - 1) == 0) return true;
    return false;
}

static bool textparser_typescript_header_has_accessor_word(
    textparser_t handle,
    const textparser_node *node,
    size_t end,
    const char *word)
{
    if (handle == nullptr || node == nullptr || end > handle->text_size) return false;
    size_t region_start = node->source_start;
    while (region_start > 0) {
        char previous = handle->text_addr[region_start - 1];
        if (previous == '\n' || previous == '\r' || previous == ';' || previous == '{' ||
            previous == '}') break;
        region_start--;
    }
    size_t word_length = strlen(word);
    for (size_t i = region_start; i + word_length <= end; i++) {
        if (memcmp(handle->text_addr + i, word, word_length) != 0) continue;
        bool left_boundary = i == region_start ||
            !(isalnum((unsigned char)handle->text_addr[i - 1]) ||
              handle->text_addr[i - 1] == '_' || handle->text_addr[i - 1] == '$');
        size_t after = i + word_length;
        bool right_boundary = after == end ||
            !(isalnum((unsigned char)handle->text_addr[after]) ||
              handle->text_addr[after] == '_' || handle->text_addr[after] == '$');
        if (!left_boundary || !right_boundary) continue;
        while (after < end && isspace((unsigned char)handle->text_addr[after])) after++;
        if (after < end && handle->text_addr[after] != '(') return true;
    }
    return false;
}

static const textparser_node *textparser_typescript_find_kind(
    const textparser_node *node,
    const char *kind)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        if (item->cst_kind != nullptr && strcmp(item->cst_kind, kind) == 0) return item;
        const textparser_node *nested = textparser_typescript_find_kind(item->child, kind);
        if (nested != nullptr) return nested;
    }
    return nullptr;
}

static const textparser_node *textparser_typescript_find_kind_after(
    const textparser_node *node,
    const char *kind,
    size_t position)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        if (item->source_start >= position && item->cst_kind != nullptr &&
            strcmp(item->cst_kind, kind) == 0) return item;
        const textparser_node *nested = textparser_typescript_find_kind_after(
            item->child, kind, position);
        if (nested != nullptr) return nested;
    }
    return nullptr;
}

static bool textparser_typescript_is_ambient_statement(const char *kind)
{
    if (kind == nullptr) return false;
    static const char *statements[] = {
        "ExpressionStatement", "IfStatement", "WhileStatement", "DoStatement",
        "ForStatement", "ContinueStatement", "BreakStatement", "ReturnStatement",
        "ThrowStatement", "SwitchStatement", "TryStatement", "WithStatement",
        "DebuggerStatement", "LabeledStatement", nullptr
    };
    for (size_t i = 0; statements[i] != nullptr; i++)
        if (strcmp(kind, statements[i]) == 0) return true;
    return false;
}

static bool textparser_typescript_declaration_file(const textparser_t handle)
{
    const char *filename = handle == nullptr ? nullptr : handle->filename;
    if (filename == nullptr) return false;
    size_t length = strlen(filename);
#ifdef _WIN32
#define TEXTPARSER_DTS_SUFFIX(suffix) (length >= sizeof(suffix) - 1 && \
    _stricmp(filename + length - (sizeof(suffix) - 1), suffix) == 0)
#else
#define TEXTPARSER_DTS_SUFFIX(suffix) (length >= sizeof(suffix) - 1 && \
    strcasecmp(filename + length - (sizeof(suffix) - 1), suffix) == 0)
#endif
    bool result = TEXTPARSER_DTS_SUFFIX(".d.ts") || TEXTPARSER_DTS_SUFFIX(".d.mts") ||
        TEXTPARSER_DTS_SUFFIX(".d.cts");
#undef TEXTPARSER_DTS_SUFFIX
    return result;
}

static bool textparser_typescript_has_declared_ancestor(const textparser_node *node)
{
    for (const textparser_node *parent = node == nullptr ? nullptr : node->parent;
         parent != nullptr; parent = parent->parent) {
        const char *kind = parent->cst_kind;
        if (kind != nullptr &&
            (strcmp(kind, "SourceFile") == 0 ||
             strcmp(kind, "Statement") == 0 ||
             strcmp(kind, "Repeat") == 0)) break;
        if (textparser_typescript_node_token(parent, "DeclareKeyword") != nullptr) return true;
    }
    return false;
}

static void textparser_typescript_check_legality_nodes(
    textparser_t handle,
    const textparser_node *node,
    textparser_typescript_legality_context context)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        const char *kind = item->cst_kind;
        textparser_typescript_legality_context child_context = context;
        bool declaration_with_modifiers = kind != nullptr &&
            (strcmp(kind, "DeclaredVariableStatement") == 0 ||
             strcmp(kind, "FunctionDeclaration") == 0 ||
             strcmp(kind, "DefaultFunctionDeclaration") == 0 ||
             strcmp(kind, "ClassDeclaration") == 0 ||
             strcmp(kind, "DefaultClassDeclaration") == 0 ||
             strcmp(kind, "InterfaceDeclaration") == 0 ||
             strcmp(kind, "TypeAliasDeclaration") == 0 ||
             strcmp(kind, "EnumDeclaration") == 0 ||
             strcmp(kind, "NamespaceDeclaration") == 0 ||
             strcmp(kind, "GlobalDeclaration") == 0);
        const textparser_node *declare_modifier = declaration_with_modifiers
            ? textparser_typescript_header_token(item->child, "DeclareKeyword") : nullptr;
        bool ambient_declarator = kind != nullptr &&
            (strcmp(kind, "VariableDeclaration") == 0 ||
             strcmp(kind, "PropertyDeclaration") == 0) &&
            (textparser_typescript_has_declared_ancestor(item) ||
             textparser_typescript_preceded_by_declare(handle, item));
        bool establishes_ambient = context.ambient || declare_modifier != nullptr ||
            ambient_declarator ||
            (kind != nullptr && strcmp(kind, "GlobalDeclaration") == 0);
        child_context.ambient = establishes_ambient;
        if (context.ambient && !context.ambient_declare_allowed && declare_modifier != nullptr)
            textparser_typescript_report_node_diagnostic(handle, declare_modifier, "TS1038",
                "A 'declare' modifier cannot be used in an already ambient context.");
        if (declare_modifier != nullptr || (kind != nullptr &&
            (strcmp(kind, "NamespaceDeclaration") == 0 ||
             strcmp(kind, "GlobalDeclaration") == 0 ||
             strcmp(kind, "ClassDeclaration") == 0 ||
             strcmp(kind, "DefaultClassDeclaration") == 0)))
            child_context.ambient_declare_allowed = false;

        if (kind != nullptr &&
            (strcmp(kind, "FunctionDeclaration") == 0 ||
             strcmp(kind, "DefaultFunctionDeclaration") == 0 ||
             strcmp(kind, "ClassDeclaration") == 0 ||
             strcmp(kind, "DefaultClassDeclaration") == 0 ||
             strcmp(kind, "MethodDeclaration") == 0 ||
             strcmp(kind, "PropertyDeclaration") == 0 ||
             strcmp(kind, "ConstructorDeclaration") == 0 ||
             strcmp(kind, "BindingParameter") == 0)) {
            unsigned seen_modifiers = 0;
            bool seen_accessibility = false;
            textparser_typescript_check_modifier_nodes(
                handle, item->child, &seen_modifiers, &seen_accessibility);
        }

        if (kind != nullptr && strcmp(kind, "MethodDeclaration") == 0) {
            const textparser_node *readonly_modifier =
                textparser_typescript_header_token(item->child, "ReadonlyKeyword");
            const textparser_node *accessor_modifier =
                textparser_typescript_header_token(item->child, "AccessorKeyword");
            if (readonly_modifier != nullptr)
                textparser_typescript_report_node_diagnostic(handle, readonly_modifier, "TS1024",
                    "'readonly' modifier can only appear on a property declaration or index signature.");
            if (accessor_modifier != nullptr)
                textparser_typescript_report_modifier_diagnostic(handle, accessor_modifier, "TS1031",
                    "cannot appear on class elements of this kind.");
        } else if (kind != nullptr && strcmp(kind, "PropertyDeclaration") == 0) {
            const textparser_node *async_modifier =
                textparser_typescript_header_token(item->child, "AsyncKeyword");
            if (async_modifier != nullptr)
                textparser_typescript_report_modifier_diagnostic(handle, async_modifier, "TS1042",
                    "cannot be used here.");
        }

        if (kind != nullptr && strcmp(kind, "MethodDeclaration") == 0) {
            const textparser_node *parameter_list_node =
                textparser_typescript_find_kind(item->child, "BindingParameterList");
            size_t member_start = parameter_list_node == nullptr
                ? item->source_end : parameter_list_node->source_start;
            bool getter = textparser_typescript_header_has_accessor_word(
                handle, item, member_start, "get");
            bool setter = textparser_typescript_header_has_accessor_word(
                handle, item, member_start, "set");
            size_t parameters = textparser_typescript_count_parameters_from_span(
                handle, parameter_list_node);
            if (getter && parameters != 0)
                textparser_typescript_report_node_diagnostic(handle, item, "TS1054",
                    "A 'get' accessor cannot have parameters.");
            if (setter && parameters != 1)
                textparser_typescript_report_node_diagnostic(handle, item, "TS1049",
                    "A 'set' accessor must have exactly one parameter.");
            if ((getter || setter) && textparser_typescript_find_kind(
                    item->child, "TypeParametersContext") != nullptr)
                textparser_typescript_report_node_diagnostic(handle, item, "TS1094",
                    "An accessor cannot have type parameters.");
            const textparser_node *parameter_list =
                textparser_typescript_find_kind(item->child, "BindingParameterList");
            if (setter && parameter_list != nullptr && textparser_typescript_find_kind_after(
                    item->child, "TypeAnnotation", parameter_list->source_end) != nullptr)
                textparser_typescript_report_node_diagnostic(handle, item, "TS1095",
                    "A 'set' accessor cannot have a return type annotation.");
            if (establishes_ambient && (getter || setter))
                textparser_typescript_report_node_diagnostic(handle, item, "TS1086",
                    "An accessor cannot be declared in an ambient context.");
        }

        if (establishes_ambient && kind != nullptr &&
            (strcmp(kind, "VariableDeclaration") == 0 ||
             strcmp(kind, "PropertyDeclaration") == 0)) {
            const textparser_node *initializer = textparser_typescript_find_kind(
                item->child, "Assign");
            if (initializer != nullptr)
                textparser_typescript_report_node_diagnostic(handle, initializer, "TS1039",
                    "Initializers are not allowed in ambient contexts.");
        }
        if (context.ambient && textparser_typescript_is_ambient_statement(kind))
            textparser_typescript_report_node_diagnostic(handle, item, "TS1036",
                "Statements are not allowed in ambient contexts.");

        bool implementation_declaration = kind != nullptr &&
            (strcmp(kind, "FunctionDeclaration") == 0 ||
             strcmp(kind, "DefaultFunctionDeclaration") == 0 ||
             strcmp(kind, "MethodDeclaration") == 0 ||
             strcmp(kind, "ConstructorDeclaration") == 0);
        if (establishes_ambient && implementation_declaration) {
            const textparser_node *body =
                textparser_typescript_find_kind(item->child, "BlockStatement");
            if (body != nullptr)
                textparser_typescript_report_node_diagnostic(handle, body, "TS1183",
                    "An implementation cannot be declared in ambient contexts.");
            const textparser_node *async_modifier =
                textparser_typescript_header_token(item->child, "AsyncKeyword");
            if (async_modifier != nullptr)
                textparser_typescript_report_modifier_diagnostic(handle, async_modifier, "TS1040",
                    "cannot be used in an ambient context.");
        }

        bool function_boundary = kind != nullptr &&
            (strcmp(kind, "FunctionDeclaration") == 0 ||
             strcmp(kind, "DefaultFunctionDeclaration") == 0 ||
             strcmp(kind, "FunctionExpression") == 0 ||
             strcmp(kind, "ArrowFunction") == 0 ||
             strcmp(kind, "AsyncArrowFunction") == 0 ||
             strcmp(kind, "GenericArrowFunction") == 0 ||
             strcmp(kind, "ParenthesizedArrowFunction") == 0 ||
             strcmp(kind, "IdentifierArrowFunction") == 0 ||
             strcmp(kind, "MethodDeclaration") == 0 ||
             strcmp(kind, "ObjectMethodDeclaration") == 0 ||
             strcmp(kind, "ObjectAccessorDeclaration") == 0 ||
             strcmp(kind, "ConstructorDeclaration") == 0);
        bool static_block_boundary = kind != nullptr &&
            strcmp(kind, "ClassStaticBlock") == 0;
        if (static_block_boundary) {
            child_context.function_depth++;
            child_context.iteration_depth = 0;
            child_context.switch_depth = 0;
            child_context.async_function = false;
            child_context.generator_function = false;
            child_context.function_body = false;
            child_context.ambient = context.ambient;
        } else if (function_boundary) {
            child_context.function_depth++;
            child_context.iteration_depth = 0;
            child_context.switch_depth = 0;
            child_context.async_function =
                textparser_typescript_subtree_has_header_token(item->child, "AsyncKeyword");
            child_context.generator_function =
                textparser_typescript_subtree_has_header_token(item->child, "Multiply");
            child_context.function_body = true;
            if (establishes_ambient) child_context.ambient = false;
        } else if (kind != nullptr &&
            (strcmp(kind, "WhileStatement") == 0 ||
             strcmp(kind, "DoStatement") == 0 ||
             strcmp(kind, "ForStatement") == 0)) {
            child_context.iteration_depth++;
        } else if (kind != nullptr && strcmp(kind, "SwitchStatement") == 0) {
            child_context.switch_depth++;
        }

        textparser_typescript_label_frame label_frame = {0};
        if (kind != nullptr && strcmp(kind, "LabeledStatement") == 0) {
            label_frame.name = textparser_typescript_node_token(item, "Identifier");
            label_frame.iteration = textparser_typescript_label_targets_iteration(item);
            label_frame.function_depth = context.function_depth;
            label_frame.previous = context.labels;
            if (label_frame.name != nullptr && textparser_typescript_find_local_label(
                    handle, context.labels, label_frame.name, context.function_depth) != nullptr)
                textparser_typescript_report_node_diagnostic(handle, label_frame.name,
                    "TS1114", "Duplicate label.");
            child_context.labels = &label_frame;
        }

        const textparser_node *jump_label = kind != nullptr &&
            (strcmp(kind, "BreakStatement") == 0 || strcmp(kind, "ContinueStatement") == 0)
            ? textparser_typescript_node_token(item, "Identifier") : nullptr;
        const textparser_typescript_label_frame *target = jump_label == nullptr ? nullptr
            : textparser_typescript_find_label(handle, context.labels, jump_label);
        if (kind != nullptr && strcmp(kind, "ReturnStatement") == 0 &&
            !context.function_body) {
            textparser_typescript_report_node_diagnostic(handle, item, "TS1108",
                "A 'return' statement can only be used within a function body.");
        } else if (kind != nullptr && strcmp(kind, "BreakStatement") == 0 &&
            jump_label == nullptr &&
            context.iteration_depth == 0 && context.switch_depth == 0) {
            textparser_typescript_report_node_diagnostic(handle, item, "TS1105",
                "A 'break' statement can only be used within an enclosing iteration or switch statement.");
        } else if (kind != nullptr && strcmp(kind, "ContinueStatement") == 0 &&
            jump_label == nullptr &&
            context.iteration_depth == 0) {
            textparser_typescript_report_node_diagnostic(handle, item, "TS1104",
                "A 'continue' statement can only be used within an enclosing iteration statement.");
        } else if (kind != nullptr && strcmp(kind, "BreakStatement") == 0 &&
            jump_label != nullptr && target != nullptr &&
            target->function_depth != context.function_depth) {
            textparser_typescript_report_node_diagnostic(handle, jump_label, "TS1107",
                "Jump target cannot cross function boundary.");
        } else if (kind != nullptr && strcmp(kind, "ContinueStatement") == 0 &&
            jump_label != nullptr && target != nullptr &&
            target->function_depth != context.function_depth) {
            textparser_typescript_report_node_diagnostic(handle, jump_label, "TS1107",
                "Jump target cannot cross function boundary.");
        } else if (kind != nullptr && strcmp(kind, "BreakStatement") == 0 &&
            jump_label != nullptr && target == nullptr) {
            textparser_typescript_report_node_diagnostic(handle, jump_label, "TS1116",
                "A 'break' statement can only jump to a label of an enclosing statement.");
        } else if (kind != nullptr && strcmp(kind, "ContinueStatement") == 0 &&
            jump_label != nullptr && (target == nullptr || !target->iteration)) {
            textparser_typescript_report_node_diagnostic(handle, jump_label, "TS1115",
                "A 'continue' statement can only jump to a label of an enclosing iteration statement.");
        } else if (kind != nullptr && strcmp(kind, "AwaitKeyword") == 0 &&
            context.function_depth != 0 && !context.async_function) {
            textparser_typescript_report_node_diagnostic(handle, item, "TS1308",
                "'await' expressions are only allowed within async functions and at the top levels of modules.");
        } else if (kind != nullptr && strcmp(kind, "YieldExpression") == 0 &&
            (context.function_depth == 0 || !context.generator_function)) {
            textparser_typescript_report_node_diagnostic(handle, item, "TS1163",
                "A 'yield' expression is only allowed in a generator body.");
        }
        textparser_typescript_check_legality_nodes(handle, item->child, child_context);
    }
}

static void textparser_typescript_check_legality(
    textparser_t handle,
    const textparser_node *root)
{
    textparser_typescript_legality_context context = {0};
    context.ambient = textparser_typescript_declaration_file(handle);
    context.ambient_declare_allowed = context.ambient;
    textparser_typescript_check_legality_nodes(handle, root, context);
}

static const textparser_node *textparser_node_last_child(const textparser_node *node)
{
    const textparser_node *last = node == nullptr ? nullptr : node->child;
    if (last == nullptr) return nullptr;
    while (last->next != nullptr) last = last->next;
    return last;
}

static const textparser_node *textparser_find_named_single_child(
    const textparser_grammar_executor *executor,
    const textparser_node *node,
    const char **name)
{
    const textparser_node *current = node;
    while (current != nullptr) {
        const char *current_name = textparser_grammar_node_production_name(executor, current);
        if (current_name != nullptr) {
            *name = current_name;
            return current;
        }
        if (current->child == nullptr || current->child->next != nullptr) break;
        current = current->child;
    }
    *name = nullptr;
    return current;
}

static textparser_ts_target_state textparser_typescript_assignment_target(
    const textparser_grammar_executor *executor,
    const textparser_node *node,
    bool allow_pattern);

static bool textparser_typescript_pattern_value(
    const textparser_grammar_executor *executor,
    const textparser_node *node)
{
    if (node == nullptr) return false;
    const char *token = textparser_grammar_node_token_name(executor, node);
    if (token != nullptr && strcmp(token, "Assign") == 0 && node->child != nullptr)
        return textparser_typescript_assignment_target(executor, node->child, true) ==
            TEXTPARSER_TS_TARGET_ASSIGNABLE;
    return textparser_typescript_assignment_target(executor, node, true) ==
        TEXTPARSER_TS_TARGET_ASSIGNABLE;
}

static bool textparser_typescript_array_pattern_nodes_internal(
    const textparser_grammar_executor *executor,
    const textparser_node *node,
    bool *seen_rest)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        const char *token = textparser_grammar_node_token_name(executor, item);
        if (*seen_rest && token != nullptr && strcmp(token, "Comma") == 0) return false;
        const char *name = textparser_grammar_node_production_name(executor, item);
        if (name != nullptr && strcmp(name, "ArrayElement") == 0) {
            const textparser_node *value = textparser_node_last_child(item);
            const textparser_node *first = textparser_node_first_terminal(item->child);
            const char *first_name = textparser_grammar_node_token_name(executor, first);
            bool rest = first_name != nullptr && strcmp(first_name, "Ellipsis") == 0;
            if (*seen_rest || (rest
                    ? textparser_typescript_assignment_target(executor, value, true) !=
                        TEXTPARSER_TS_TARGET_ASSIGNABLE
                    : !textparser_typescript_pattern_value(executor, value)))
                return false;
            *seen_rest = rest;
            continue;
        }
        if (!textparser_typescript_array_pattern_nodes_internal(
                executor, item->child, seen_rest)) return false;
    }
    return true;
}

static bool textparser_typescript_array_pattern_nodes(
    const textparser_grammar_executor *executor,
    const textparser_node *node)
{
    bool seen_rest = false;
    return textparser_typescript_array_pattern_nodes_internal(executor, node, &seen_rest);
}

static bool textparser_typescript_object_pattern_element(
    const textparser_grammar_executor *executor,
    const textparser_node *node,
    bool *is_rest)
{
    const char *name = nullptr;
    const textparser_node *element = textparser_find_named_single_child(executor, node, &name);
    if (element == nullptr || name == nullptr) return false;
    *is_rest = false;
    if (strcmp(name, "ObjectShorthandProperty") == 0) return true;
    if (strcmp(name, "ObjectPropertyAssignment") == 0)
        return textparser_typescript_pattern_value(executor, textparser_node_last_child(element));
    if (strcmp(name, "ObjectSpreadAssignment") == 0) {
        *is_rest = true;
        return textparser_typescript_assignment_target(
            executor, textparser_node_last_child(element), true) == TEXTPARSER_TS_TARGET_ASSIGNABLE;
    }
    return false;
}

static bool textparser_typescript_object_pattern_nodes_internal(
    const textparser_grammar_executor *executor,
    const textparser_node *node,
    bool *seen_rest)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        const char *token = textparser_grammar_node_token_name(executor, item);
        if (*seen_rest && token != nullptr && strcmp(token, "Comma") == 0) return false;
        const char *name = textparser_grammar_node_production_name(executor, item);
        if (name != nullptr && (strcmp(name, "ObjectShorthandProperty") == 0 ||
                strcmp(name, "ObjectPropertyAssignment") == 0 ||
                strcmp(name, "ObjectSpreadAssignment") == 0 ||
                strcmp(name, "ObjectMethodDeclaration") == 0 ||
                strcmp(name, "ObjectAccessorDeclaration") == 0)) {
            bool rest = false;
            if (*seen_rest || !textparser_typescript_object_pattern_element(executor, item, &rest))
                return false;
            *seen_rest = rest;
            continue;
        }
        if (!textparser_typescript_object_pattern_nodes_internal(
                executor, item->child, seen_rest)) return false;
    }
    return true;
}

static bool textparser_typescript_object_pattern_nodes(
    const textparser_grammar_executor *executor,
    const textparser_node *node)
{
    bool seen_rest = false;
    return textparser_typescript_object_pattern_nodes_internal(executor, node, &seen_rest);
}

static textparser_ts_target_state textparser_typescript_assignment_target(
    const textparser_grammar_executor *executor,
    const textparser_node *node,
    bool allow_pattern)
{
    if (node == nullptr) return TEXTPARSER_TS_TARGET_INVALID;
    const char *token = textparser_grammar_node_token_name(executor, node);
    if (token != nullptr && strcmp(token, "LogicalNot") == 0 &&
        (node->node_flags & TEXTPARSER_NODE_GRAMMAR_POSTFIX) != 0)
        return textparser_typescript_assignment_target(executor, node->child, allow_pattern);
    if (token != nullptr)
        return strcmp(token, "Identifier") == 0
            ? TEXTPARSER_TS_TARGET_ASSIGNABLE : TEXTPARSER_TS_TARGET_INVALID;

    const char *name = textparser_grammar_node_production_name(executor, node);
    const textparser_node *possible_suffix = node->child == nullptr
        ? nullptr : textparser_node_first_terminal(node->child->next);
    const char *possible_suffix_name = textparser_grammar_node_token_name(
        executor, possible_suffix);
    bool is_postfix_node = possible_suffix_name != nullptr &&
        (strcmp(possible_suffix_name, "OptionalChain") == 0 ||
         strcmp(possible_suffix_name, "LParen") == 0 ||
         strcmp(possible_suffix_name, "LBracket") == 0 ||
         strcmp(possible_suffix_name, "Dot") == 0 ||
         strcmp(possible_suffix_name, "LogicalNot") == 0);
    if (is_postfix_node) {
        const textparser_node *left = node->child;
        const char *suffix = possible_suffix_name;
        if (suffix == nullptr) return TEXTPARSER_TS_TARGET_INVALID;
        if (strcmp(suffix, "OptionalChain") == 0) return TEXTPARSER_TS_TARGET_OPTIONAL_CHAIN;
        if (strcmp(suffix, "LParen") == 0) return TEXTPARSER_TS_TARGET_INVALID;
        textparser_ts_target_state base = textparser_typescript_assignment_target(
            executor, left, allow_pattern);
        if (strcmp(suffix, "LogicalNot") == 0) return base;
        if (strcmp(suffix, "Dot") == 0 || strcmp(suffix, "LBracket") == 0)
            return base == TEXTPARSER_TS_TARGET_OPTIONAL_CHAIN
                ? TEXTPARSER_TS_TARGET_OPTIONAL_CHAIN : TEXTPARSER_TS_TARGET_ASSIGNABLE;
        return TEXTPARSER_TS_TARGET_INVALID;
    }
    if (allow_pattern && name != nullptr && strcmp(name, "ArrayLiteralExpression") == 0)
        return textparser_typescript_array_pattern_nodes(executor, node->child)
            ? TEXTPARSER_TS_TARGET_ASSIGNABLE : TEXTPARSER_TS_TARGET_INVALID;
    if (allow_pattern && name != nullptr && strcmp(name, "ObjectLiteralBody") == 0)
        return textparser_typescript_object_pattern_nodes(executor, node->child)
            ? TEXTPARSER_TS_TARGET_ASSIGNABLE : TEXTPARSER_TS_TARGET_INVALID;

    if (node->child != nullptr && node->child->next == nullptr)
        return textparser_typescript_assignment_target(executor, node->child, allow_pattern);

    const textparser_node *first = textparser_node_first_terminal(node->child);
    const textparser_node *last_child = textparser_node_last_child(node);
    const textparser_node *last = textparser_node_first_terminal(last_child);
    const char *first_name = textparser_grammar_node_token_name(executor, first);
    const char *last_name = textparser_grammar_node_token_name(executor, last);
    if (first_name != nullptr && last_name != nullptr &&
        strcmp(first_name, "LParen") == 0 && strcmp(last_name, "RParen") == 0) {
        const textparser_node *middle = node->child == nullptr ? nullptr : node->child->next;
        textparser_ts_target_state inner = textparser_typescript_assignment_target(
            executor, middle, allow_pattern);
        return inner == TEXTPARSER_TS_TARGET_ASSIGNABLE
            ? inner : TEXTPARSER_TS_TARGET_INVALID;
    }
    return TEXTPARSER_TS_TARGET_INVALID;
}

static bool textparser_pratt_validate_operand(
    textparser_grammar_executor *executor,
    const char *validator,
    const textparser_node *node,
    bool allow_pattern)
{
    if (validator == nullptr) return true;
    bool assignment = strcmp(validator, "typescript.assignmentTarget") == 0;
    bool update = strcmp(validator, "typescript.updateTarget") == 0;
    if (assignment || update) {
        textparser_ts_target_state state = textparser_typescript_assignment_target(
            executor, node, allow_pattern);
        if (state == TEXTPARSER_TS_TARGET_ASSIGNABLE) return true;
        if (textparser_is_typescript_language(executor->handle)) {
            size_t start = node != nullptr ? node->source_start : executor->handle->parser.source_offset;
            size_t end = node != nullptr ? node->source_end : start;
            executor->typescript_diagnostic_code = update ? "TS2357" :
                state == TEXTPARSER_TS_TARGET_OPTIONAL_CHAIN ? "TS2779" : "TS2364";
            executor->typescript_diagnostic_message = update
                ? "The operand of an increment or decrement operator must be a variable or a property access."
                : state == TEXTPARSER_TS_TARGET_OPTIONAL_CHAIN
                    ? "The left-hand side of an assignment expression may not be an optional property access."
                    : "The left-hand side of an assignment expression must be a variable or a property access.";
            executor->typescript_diagnostic_start = start;
            executor->typescript_diagnostic_length = end >= start ? end - start : 0;
        }
        return false;
    }
    return false;
}

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
        textparser_set_lexical_goal(executor->handle, saved_goal);
        textparser_speculate_commit(executor->handle, checkpoint);
    } else {
        textparser_speculate_rollback(executor->handle, checkpoint);
    }
    free(saved_goal);
    return result;
}

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
                result.node->cst_kind = production->name;
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

static textparser_match_result textparser_parse_predicate(
    textparser_grammar_executor *executor,
    const textparser_production *production)
{
    textparser_t handle = executor->handle;
    if (production->predicate_name == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    textparser_predicate_entry *entry = handle->predicates;
    while (entry != nullptr && strcmp(entry->name, production->predicate_name) != 0) entry = entry->next;
    if (entry == nullptr && strcmp(production->predicate_name,
            "typescript.noLineTerminatorBefore") == 0) {
        const textparser_lex_token *current = nullptr;
        int scan = textparser_grammar_peek_token(executor, &current);
        bool accepted = scan > 0 || current == nullptr ||
            (current->flags & TEXTPARSER_LEX_FLAG_CONTAINS_LINE_TERMINATOR) == 0;
        return textparser_match_result_make(
            accepted ? TEXTPARSER_MATCH_OK : TEXTPARSER_MATCH_NO, nullptr, 0);
    }
    if (entry == nullptr && strcmp(production->predicate_name,
            "typescript.isMetaIdentifier") == 0) {
        const textparser_lex_token *current = nullptr;
        int scan = textparser_grammar_peek_token(executor, &current);
        bool accepted = scan == 0 && current != nullptr &&
            current->end - current->start == 4 &&
            current->end <= handle->text_size &&
            memcmp(handle->text_addr + current->start, "meta", 4) == 0;
        return textparser_match_result_make(
            accepted ? TEXTPARSER_MATCH_OK : TEXTPARSER_MATCH_NO, nullptr, 0);
    }
    if (entry == nullptr && strcmp(production->predicate_name,
            "typescript.canFollowTypeArgumentsInExpression") == 0) {
        const textparser_lex_token *current = nullptr;
        int scan = textparser_grammar_peek_token(executor, &current);
        bool accepted = scan > 0 || current == nullptr;
        if (!accepted && current->kind >= 0 &&
            current->kind < (int)handle->token_count) {
            const char *name = handle->language->tokens[current->kind].name;
            static const char *allowed[] = {
                "Dot", "OptionalChain", "LBracket", "NoSubstitutionTemplateLiteral",
                "TemplateHead", "LogicalNot", "Increment", "Decrement", "Exponent",
                "Multiply", "Slash", "Remainder", "Plus", "Minus", "LeftShift",
                "RightShift", "UnsignedRightShift", "LessThan", "LessEqual",
                "GreaterThan", "GreaterEqual", "Equal", "NotEqual", "StrictEqual",
                "StrictNotEqual", "BitAnd", "BitXor", "BitOr", "LogicalAnd",
                "LogicalOr", "NullishCoalesce", "Question", "Colon", "Assign",
                "PlusAssign", "MinusAssign", "MultiplyAssign", "DivideAssign",
                "RemainderAssign", "ExponentAssign", "LeftShiftAssign",
                "RightShiftAssign", "UnsignedRightShiftAssign", "BitAndAssign",
                "BitOrAssign", "BitXorAssign", "NullishCoalesceAssign",
                "LogicalAndAssign", "LogicalOrAssign",
                "Comma", "Semicolon", "RParen", "RBracket", "RBrace", nullptr
            };
            for (size_t i = 0; name != nullptr && allowed[i] != nullptr; i++) {
                if (strcmp(name, allowed[i]) == 0) { accepted = true; break; }
            }
        }
        return textparser_match_result_make(
            accepted ? TEXTPARSER_MATCH_OK : TEXTPARSER_MATCH_NO, nullptr, 0);
    }
    if (entry == nullptr &&
        (strcmp(production->predicate_name, "typescript.allowsJSX") == 0 ||
         strcmp(production->predicate_name, "typescript.disallowsJSX") == 0 ||
         strcmp(production->predicate_name, "typescript.allowsTypeScript") == 0)) {
        const char *filename = handle->filename;
        size_t length = filename == nullptr ? 0 : strlen(filename);
#ifdef _WIN32
#define TEXTPARSER_SUFFIX_EQUAL(suffix) \
        (length >= sizeof(suffix) - 1 && _stricmp(filename + length - (sizeof(suffix) - 1), suffix) == 0)
#else
#define TEXTPARSER_SUFFIX_EQUAL(suffix) \
        (length >= sizeof(suffix) - 1 && strcasecmp(filename + length - (sizeof(suffix) - 1), suffix) == 0)
#endif
        bool jsx = filename != nullptr &&
            (TEXTPARSER_SUFFIX_EQUAL(".tsx") || TEXTPARSER_SUFFIX_EQUAL(".jsx"));
        bool javascript = filename != nullptr &&
            (TEXTPARSER_SUFFIX_EQUAL(".js") || TEXTPARSER_SUFFIX_EQUAL(".jsx") ||
             TEXTPARSER_SUFFIX_EQUAL(".mjs") || TEXTPARSER_SUFFIX_EQUAL(".cjs"));
#undef TEXTPARSER_SUFFIX_EQUAL
        bool accepted = strcmp(production->predicate_name,
            "typescript.allowsJSX") == 0 ? jsx :
            (strcmp(production->predicate_name, "typescript.disallowsJSX") == 0
                ? !jsx : !javascript);
        return textparser_match_result_make(
            accepted ? TEXTPARSER_MATCH_OK : TEXTPARSER_MATCH_NO, nullptr, 0);
    }
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

static textparser_match_result textparser_parse_production(
    textparser_grammar_executor *executor,
    int production_id)
{
    if (executor->recursion_depth >= MAX_RECURSION_DEPTH) {
        return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    }
    const textparser_production *production = textparser_find_production(executor, production_id);
    if (production == nullptr) return textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
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
        if (executor.typescript_diagnostic_code != nullptr) {
            textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR,
                executor.typescript_diagnostic_code, executor.typescript_diagnostic_message,
                executor.typescript_diagnostic_start, executor.typescript_diagnostic_length);
        } else if (executor.furthest_failure != nullptr) {
            textparser_grammar_report_expected(&executor, executor.furthest_failure,
                executor.furthest_failure_offset, executor.furthest_failure_length, false);
        }
    } else if (out_result->status == TEXTPARSER_MATCH_OK &&
        textparser_is_typescript_language(handle) &&
        handle->diagnostic_count == executor.initial_diagnostic_count) {
        const textparser_lex_token *remaining = nullptr;
        int peek = textparser_grammar_peek_token(&executor, &remaining);
        if (peek == 0 && remaining != nullptr) {
            if (executor.typescript_diagnostic_code != nullptr) {
                textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR,
                    executor.typescript_diagnostic_code, executor.typescript_diagnostic_message,
                    executor.typescript_diagnostic_start, executor.typescript_diagnostic_length);
            } else if (executor.furthest_failure != nullptr &&
                executor.furthest_failure_offset > remaining->start) {
                textparser_grammar_report_expected(&executor, executor.furthest_failure,
                    executor.furthest_failure_offset, executor.furthest_failure_length, false);
            } else {
                textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR, "TS1128",
                    "Declaration or statement expected.", remaining->start,
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
    if (textparser_is_typescript_language(handle))
        textparser_typescript_check_legality(handle, out_result->node);
    if (language->grammar->source_complete_handler == nullptr) return status;
    textparser_event event = {0};
    event.type = TEXTPARSER_EVENT_SOURCE_COMPLETE;
    event.node = out_result->node;
    event.start = 0;
    event.end = handle->parser.source_offset;
    event.configuration = language->grammar->source_complete_configuration;
    textparser_action action = textparser_dispatch_event(
        handle, language->grammar->source_complete_handler, &event);
    if (action == TEXTPARSER_ACTION_REJECT)
        *out_result = textparser_match_result_make(TEXTPARSER_MATCH_ERROR, nullptr, 0);
    else if (action == TEXTPARSER_ACTION_ABORT)
        *out_result = textparser_match_result_make(TEXTPARSER_MATCH_ABORT, nullptr, 0);
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
