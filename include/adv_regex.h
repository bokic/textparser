#ifndef ADV_REGEX_H
#define ADV_REGEX_H

#include <stdbool.h>
#include <stddef.h>


enum adv_regex_encoding { ADV_REGEX_TEXT_LATIN1, ADV_REGEX_TEXT_UTF_8, ADV_REGEX_TEXT_UNICODE, ADV_REGEX_TEXT_UTF_16, ADV_REGEX_TEXT_UTF_32 };

bool adv_regex_find_pattern(const char *regex, enum adv_regex_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length);

#endif // ADV_REGEX_H
