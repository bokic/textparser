#ifndef ADV_REGEX_H
#define ADV_REGEX_H

#define adv_regex_pattern void *

enum { ADV_REGEX_TEXT_LATIN1, ADV_REGEX_TEXT_UTF8, ADV_REGEX_TEXT_UNICODE };

adv_regex_pattern adv_regex_compile(const char* pattern);
void *adv_regex_find(const adv_regex_pattern pattern, int encoding, const void *start, const void *end, int *len);
void *adv_regex_find_pattern(const char *regex, int encoding, const void *start, const void *end, int *len);

#endif // ADV_REGEX_H
