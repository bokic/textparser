#ifndef ADV_REGEX_H
#define ADV_REGEX_H

enum { ADV_REGEX_TEXT_LATIN1, ADV_REGEX_TEXT_UTF8, ADV_REGEX_TEXT_UNICODE };

int adv_regex_find_pattern(const char *regex, int encoding, const char *start, int max_len, int *len);

#endif // ADV_REGEX_H
