#pragma once

#include "textparser.h"

#include <stdbool.h>
#include <stddef.h>


typedef struct adv_regex_context adv_regex_context;

/**
 * Allocate and initialize a new advanced regex context with lazy PCRE2 dynamic loading.
 *
 * @return Allocated adv_regex_context pointer, or NULL on memory allocation failure.
 */
adv_regex_context *adv_regex_context_create(void);

/**
 * Release all resources and dynamically loaded PCRE2 instances associated with the context.
 *
 * @param ctx Advanced regex context pointer to destroy.
 */
void adv_regex_context_free(adv_regex_context *ctx);

/**
 * Configure whether the active input text is known to be valid UTF-8, allowing PCRE2_NO_UTF_CHECK optimization.
 *
 * @param ctx Advanced regex context pointer.
 * @param valid True if input buffer is pre-validated UTF-8, false otherwise.
 */
void adv_regex_set_utf8_valid(adv_regex_context *ctx, bool valid);

/**
 * Execute or lazily compile and match a PCRE2 regular expression against a text buffer.
 *
 * @param ctx Advanced regex context pointer.
 * @param regex_str Pattern string in UTF-8 format.
 * @param regex In/out compiled pattern handle cache pointer.
 * @param encoding Text encoding of the subject buffer.
 * @param start Pointer to start of subject text slice.
 * @param max_len Maximum length of subject slice in encoding units.
 * @param offset Output offset where match begins.
 * @param length Output length of matched substring.
 * @param is_caseless Whether pattern matching is case-insensitive.
 * @param only_at_start Whether match must be anchored at subject start.
 * @return True if a match was found, false otherwise.
 */
bool adv_regex_find_pattern_ctx(adv_regex_context *ctx, const char *regex_str, void **regex, enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/**
 * Release a compiled PCRE2 regular expression object for a specific encoding.
 *
 * @param ctx Advanced regex context pointer.
 * @param regex Pointer to compiled regex handle pointer to free.
 * @param encoding Text encoding used when compiling the regex.
 */
void adv_regex_free(adv_regex_context *ctx, void **regex, enum textparser_encoding encoding);


