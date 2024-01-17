#define PCRE2_CODE_UNIT_WIDTH 0

#include "adv_regex.h"
#include <pcre2.h>

#include <stdio.h>
#include <stddef.h>


static bool adv_regex_find_pattern8(const char *regex_str, pcre2_code_8 **regex, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_utf)
{
    bool ret = false;

    PCRE2_SIZE error_offset = 0;
    int error_number = 0;
    pcre2_match_data_8 *match_data = NULL;
    PCRE2_SIZE *ovector = NULL;

    if (*regex == NULL)
    {
        *regex = pcre2_compile_8((PCRE2_SPTR8)regex_str, PCRE2_ZERO_TERMINATED, is_utf?PCRE2_UTF|PCRE2_CASELESS:PCRE2_CASELESS, &error_number, &error_offset, NULL);                 /* use default compile context */
        if (*regex == NULL)
        {
            PCRE2_UCHAR8 buffer[256];
            pcre2_get_error_message_8(error_number, buffer, sizeof(buffer));
            printf("PCRE2 compilation failed at offset %zu: %s\n", error_offset, buffer);
            return -1;
        }
    }

    match_data = pcre2_match_data_create_from_pattern_8(*regex, NULL);

    int rc = pcre2_match_8(
        *regex,               /* the compiled pattern */
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
            if (offset)
                *offset = ovector[0];

            if (length)
                *length = ovector[1];

            ret = true;
        }
    }

    if (match_data)
        pcre2_match_data_free_8(match_data);

    return ret;
}

static bool adv_regex_find_pattern16(const char *regex_str, pcre2_code_16 **regex, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_utf)
{
    bool ret = false;

    PCRE2_SIZE error_offset = 0;
    int error_number = 0;
    pcre2_match_data_16 *match_data = NULL;
    PCRE2_SIZE *ovector = NULL;

    if (*regex == NULL)
    {
        *regex = pcre2_compile_16((PCRE2_SPTR16)regex_str, PCRE2_ZERO_TERMINATED, is_utf?PCRE2_UTF|PCRE2_CASELESS:PCRE2_CASELESS, &error_number, &error_offset, NULL);
        if (*regex == NULL)
        {
            PCRE2_UCHAR8 buffer[256];
            pcre2_get_error_message_8(error_number, buffer, sizeof(buffer));
            printf("PCRE2 compilation failed at offset %zu: %s\n", error_offset, buffer);
            return -1;
        }
    }

    match_data = pcre2_match_data_create_from_pattern_16(*regex, NULL);

    int rc = pcre2_match_16(
        *regex,               // the compiled pattern
        (PCRE2_SPTR16)start,  // the subject string
        max_len,              // the length of the subject
        0,                    // start at offset 0 in the subject
        0,                    // default options
        match_data,           // block for storing the result
        NULL);                // use default match context
    if (rc == 1)
    {
        ovector = pcre2_get_ovector_pointer_16(match_data);
        if (ovector)
        {
            if (offset)
                *offset = ovector[0];

            if (length)
                *length = ovector[1];

            ret = true;
        }
    }

    if (match_data)
        pcre2_match_data_free_16(match_data);

    return ret;
}

static bool adv_regex_find_pattern32(const char *regex_str, pcre2_code_32 **regex, const char *start, size_t max_len, size_t *offset, size_t *length)
{
    bool ret = false;

    PCRE2_SIZE error_offset = 0;
    int error_number = 0;
    pcre2_match_data_32 *match_data = NULL;
    PCRE2_SIZE *ovector = NULL;

    if (*regex == NULL)
    {
        *regex = pcre2_compile_32((PCRE2_SPTR32)regex_str, PCRE2_ZERO_TERMINATED, PCRE2_CASELESS, &error_number, &error_offset, NULL);
        if (*regex == NULL)
        {
            PCRE2_UCHAR8 buffer[256];
            pcre2_get_error_message_8(error_number, buffer, sizeof(buffer));
            printf("PCRE2 compilation failed at offset %zu: %s\n", error_offset, buffer);
            return -1;
        }
    }

    match_data = pcre2_match_data_create_from_pattern_32(*regex, NULL);

    int rc = pcre2_match_32(
        *regex,               // the compiled pattern
        (PCRE2_SPTR32)start,  // the subject string
        max_len,              // the length of the subject
        0,                    // start at offset 0 in the subject
        0,                    // default options
        match_data,           // block for storing the result
        NULL);                // use default match context
    if (rc == 1)
    {
        ovector = pcre2_get_ovector_pointer_32(match_data);
        if (ovector)
        {
            if (offset)
                *offset = ovector[0];

            if (length)
                *length = ovector[1];

            ret = true;
        }
    }

    if (match_data)
        pcre2_match_data_free_32(match_data);

    return ret;
}

bool adv_regex_find_pattern(const char *regex_str, void **regex, enum adv_regex_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length)
{
    switch(encoding)
    {
    case ADV_REGEX_TEXT_LATIN1:
        return adv_regex_find_pattern8(regex_str, (pcre2_code_8 **)regex, start, max_len, offset, length, false);
    case ADV_REGEX_TEXT_UTF_8:
        return adv_regex_find_pattern8(regex_str, (pcre2_code_8 **)regex, start, max_len, offset, length, true);
    case ADV_REGEX_TEXT_UNICODE:
        return adv_regex_find_pattern16(regex_str, (pcre2_code_16 **)regex, start, max_len, offset, length, false);
    case ADV_REGEX_TEXT_UTF_16:
        return adv_regex_find_pattern16(regex_str, (pcre2_code_16 **)regex, start, max_len, offset, length, true);
    case ADV_REGEX_TEXT_UTF_32:
        return adv_regex_find_pattern32(regex_str, (pcre2_code_32 **)regex, start, max_len, offset, length);
    default:
    }

    return false;
}

void adv_regex_free(void **regex, enum adv_regex_encoding encoding)
{
    if ((regex == NULL)||((*regex == NULL)))
        return;

    switch(encoding)
    {
    case ADV_REGEX_TEXT_LATIN1:
    case ADV_REGEX_TEXT_UTF_8:
        pcre2_code_free_8(*(pcre2_code_8 **)regex);
        break;
    case ADV_REGEX_TEXT_UNICODE:
    case ADV_REGEX_TEXT_UTF_16:
        pcre2_code_free_16(*(pcre2_code_16 **)regex);
        break;
    case ADV_REGEX_TEXT_UTF_32:
        pcre2_code_free_32(*(pcre2_code_32 **)regex);
        break;
    default:
    }

    *regex = NULL;
}
