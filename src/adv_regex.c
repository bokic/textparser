#define PCRE2_CODE_UNIT_WIDTH 0

#include "adv_regex.h"
#include <pcre2.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>


int adv_regex_find_pattern(const char *regex, int encoding, const char *start, int max_len, int *len)
{
    int ret = -1;

    PCRE2_SIZE erroroffset = 0;
    int errornumber = 0;
    pcre2_match_data_8 *match_data = NULL;
    pcre2_code_8 *re = NULL;
    PCRE2_SIZE *ovector = NULL;
    int rc = 0;

    re = pcre2_compile_8((PCRE2_SPTR8)regex, PCRE2_ZERO_TERMINATED, 0, &errornumber, &erroroffset, NULL);                 /* use default compile context */
    if (re == NULL)
    {
        PCRE2_UCHAR8 buffer[256];
        pcre2_get_error_message_8(errornumber, buffer, sizeof(buffer));
        printf("PCRE2 compilation failed at offset %zu: %s\n", erroroffset, buffer);
        return -1;
    }

    match_data = pcre2_match_data_create_from_pattern_8(re, NULL);

    rc = pcre2_match_8(
        re,                   /* the compiled pattern */
        (PCRE2_SPTR8)start,   /* the subject string */
        max_len,              /* the length of the subject */
        0,                    /* start at offset 0 in the subject */
        0,                    /* default options */
        match_data,           /* block for storing the result */
        NULL);                /* use default match context */
    if (rc == 1)
    {
        ovector = pcre2_get_ovector_pointer_8(match_data);
        if (ovector)
        {
            ret = ovector[0];

            if (len)
                *len = ovector[1];
        }
    }

    if (match_data)
        pcre2_match_data_free_8(match_data);

    pcre2_code_free_8(re);

    return ret;
}
