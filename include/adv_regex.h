#pragma once

#include "textparser.h"

#include <stdbool.h>
#include <stddef.h>


bool adv_regex_find_pattern(const char *regex_str, void **regex, enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool only_at_start);
void adv_regex_free(void **regex, enum textparser_encoding encoding);
