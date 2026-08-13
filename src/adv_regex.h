#pragma once

#include "textparser.h"

#include <stdbool.h>
#include <stddef.h>


typedef struct adv_regex_context adv_regex_context;

adv_regex_context *adv_regex_context_create(void);
void adv_regex_context_free(adv_regex_context *ctx);

bool adv_regex_find_pattern_ctx(adv_regex_context *ctx, const char *regex_str, void **regex, enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
void adv_regex_free(void **regex, enum textparser_encoding encoding);
