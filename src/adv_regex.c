#define PCRE2_CODE_UNIT_WIDTH 0

#include "adv_regex.h"
#include <pcre2.h>

#include <stdio.h>
#include <stddef.h>


static pcre2_compile_context_8 *ccontext8 = nullptr;
static pcre2_compile_context_16 *ccontext16 = nullptr;
static pcre2_compile_context_32 *ccontext32 = nullptr;

static bool adv_regex_find_pattern8(const char *regex_str, pcre2_code_8 **regex, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_utf, bool is_caseless, bool only_at_start)
{
    bool ret = false;

    PCRE2_SIZE error_offset = 0;
    int error_number = 0;
    pcre2_match_data_8 *match_data = nullptr;
    PCRE2_SIZE *ovector = nullptr;

    if (ccontext8 == nullptr)
    {
        ccontext8 = pcre2_compile_context_create_8(nullptr);
        pcre2_set_newline_8(ccontext8, PCRE2_NEWLINE_ANY);
    }

    if (*regex == nullptr)
    {
        uint32_t options = 0;
        if (is_utf) options |= PCRE2_UTF;
        if (is_caseless) options |= PCRE2_CASELESS;
        *regex = pcre2_compile_8((PCRE2_SPTR8)regex_str, PCRE2_ZERO_TERMINATED, options, &error_number, &error_offset, ccontext8);
        if (*regex == nullptr)
        {
            PCRE2_UCHAR8 buffer[256];
            pcre2_get_error_message_8(error_number, buffer, sizeof(buffer));
            printf("PCRE2 compilation_8 failed for regex [%s]\n", regex_str);
            return false;
        }

        int err = pcre2_jit_compile_8(*regex, PCRE2_JIT_COMPLETE);
        if (err != 0)
        {
            PCRE2_UCHAR8 buffer[256];
            pcre2_get_error_message_8(err, buffer, sizeof(buffer));
            printf("PCRE2 JIT compilation failed for regex [%s]\n", regex_str);
            return false;
        }
    }

    match_data = pcre2_match_data_create_from_pattern_8(*regex, nullptr);

    int rc = pcre2_match_8(
        *regex,                                // the compiled pattern
        (PCRE2_SPTR8)start,                    // the subject string
        max_len,                               // the length of the subject
        0,                                     // start at offset 0 in the subject
        only_at_start?PCRE2_ANCHORED:0,        // default options
        match_data,                            // block for storing the result
        nullptr);                              // use default match context
    if (rc == 1)
    {
        ovector = pcre2_get_ovector_pointer_8(match_data);
        if ((ovector)&&(ovector[1] > 0))
        {
            if (offset)
                *offset = ovector[0];

            if (length)
                *length = ovector[1] - ovector[0];

            ret = true;
        }
    }
    else if (rc == 2)
    {
        ovector = pcre2_get_ovector_pointer_8(match_data);
        if ((ovector)&&(ovector[3] > 0))
        {
            if (offset)
                *offset = ovector[2];

            if (length)
                *length = ovector[3] - ovector[2];

            ret = true;
        }
    }

    if (match_data)
        pcre2_match_data_free_8(match_data);

    return ret;
}

static bool adv_regex_find_pattern16(const char *regex_str, pcre2_code_16 **regex, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_utf, bool is_caseless, bool only_at_start)
{
    bool ret = false;

    PCRE2_SIZE error_offset = 0;
    int error_number = 0;
    pcre2_match_data_16 *match_data = nullptr;
    PCRE2_SIZE *ovector = nullptr;

    if (ccontext16 == nullptr)
    {
        ccontext16 = pcre2_compile_context_create_16(nullptr);
        pcre2_set_newline_16(ccontext16, PCRE2_NEWLINE_ANY);
    }

    if (*regex == nullptr)
    {
        uint32_t options = 0;
        if (is_utf) options |= PCRE2_UTF;
        if (is_caseless) options |= PCRE2_CASELESS;
        *regex = pcre2_compile_16((PCRE2_SPTR16)regex_str, PCRE2_ZERO_TERMINATED, options, &error_number, &error_offset, ccontext16);
        if (*regex == nullptr)
        {
            PCRE2_UCHAR8 buffer[256];
            pcre2_get_error_message_8(error_number, buffer, sizeof(buffer));
            printf("PCRE2 compilation_16 failed at offset %zu: %s\n", error_offset, buffer);
            return false;
        }

        pcre2_jit_compile_16(*regex, PCRE2_JIT_COMPLETE);
    }

    match_data = pcre2_match_data_create_from_pattern_16(*regex, nullptr);

    int rc = pcre2_match_16(
        *regex,                                // the compiled pattern
        (PCRE2_SPTR16)start,                   // the subject string
        max_len,                               // the length of the subject
        0,                                     // start at offset 0 in the subject
        only_at_start?PCRE2_ANCHORED:0,        // default options
        match_data,                            // block for storing the result
        nullptr);                              // use default match context
    if (rc == 1)
    {
        ovector = pcre2_get_ovector_pointer_16(match_data);
        if ((ovector)&&(ovector[1] > 0))
        {
            if (offset)
                *offset = ovector[0];

            if (length)
                *length = ovector[1] - ovector[0];

            ret = true;
        }
    }
    else if (rc == 2)
    {
        ovector = pcre2_get_ovector_pointer_16(match_data);
        if ((ovector)&&(ovector[3] > 0))
        {
            if (offset)
                *offset = ovector[2];

            if (length)
                *length = ovector[3] - ovector[2];

            ret = true;
        }
    }

    if (match_data)
        pcre2_match_data_free_16(match_data);

    return ret;
}

static bool adv_regex_find_pattern32(const char *regex_str, pcre2_code_32 **regex, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    bool ret = false;

    PCRE2_SIZE error_offset = 0;
    int error_number = 0;
    pcre2_match_data_32 *match_data = nullptr;
    PCRE2_SIZE *ovector = nullptr;

    if (ccontext32 == nullptr)
    {
        ccontext32 = pcre2_compile_context_create_32(nullptr);
        pcre2_set_newline_32(ccontext32, PCRE2_NEWLINE_ANY);
    }

    if (*regex == nullptr)
    {
        uint32_t options = 0;
        if (is_caseless) options |= PCRE2_CASELESS;
        *regex = pcre2_compile_32((PCRE2_SPTR32)regex_str, PCRE2_ZERO_TERMINATED, options, &error_number, &error_offset, ccontext32);
        if (*regex == nullptr)
        {
            PCRE2_UCHAR8 buffer[256];
            pcre2_get_error_message_8(error_number, buffer, sizeof(buffer));
            fprintf(stderr, "PCRE2 compilation_32 failed at offset %zu: %s\n", error_offset, buffer);
            return false;
        }

        pcre2_jit_compile_32(*regex, PCRE2_JIT_COMPLETE);
    }

    match_data = pcre2_match_data_create_from_pattern_32(*regex, nullptr);

    int rc = pcre2_match_32(
        *regex,                                // the compiled pattern
        (PCRE2_SPTR32)start,                   // the subject string
        max_len,                               // the length of the subject
        0,                                     // start at offset 0 in the subject
        only_at_start?PCRE2_ANCHORED:0, // default options
        match_data,                            // block for storing the result
        nullptr);                              // use default match context
    if (rc == 1)
    {
        ovector = pcre2_get_ovector_pointer_32(match_data);
        if ((ovector)&&(ovector[1] > 0))
        {
            if (offset)
                *offset = ovector[0];

            if (length)
                *length = ovector[1] - ovector[0];

            ret = true;
        }
    }
    else if (rc == 2)
    {
        ovector = pcre2_get_ovector_pointer_32(match_data);
        if ((ovector)&&(ovector[3] > 0))
        {
            if (offset)
                *offset = ovector[2];

            if (length)
                *length = ovector[3] - ovector[2];

            ret = true;
        }
    }

    if (match_data)
        pcre2_match_data_free_32(match_data);

    return ret;
}

bool adv_regex_find_pattern(const char *regex_str, void **regex, enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (regex_str == nullptr)
    {
        return false;
    }

    switch(encoding)
    {
    case TEXTPARSER_ENCODING_LATIN1:
        return adv_regex_find_pattern8(regex_str, (pcre2_code_8 **)regex, start, max_len, offset, length, false, is_caseless, only_at_start);
    case TEXTPARSER_ENCODING_UTF_8:
        return adv_regex_find_pattern8(regex_str, (pcre2_code_8 **)regex, start, max_len, offset, length, true, is_caseless, only_at_start);
    case TEXTPARSER_ENCODING_UNICODE:
        return adv_regex_find_pattern16(regex_str, (pcre2_code_16 **)regex, start, max_len, offset, length, false, is_caseless, only_at_start);
    case TEXTPARSER_ENCODING_UTF_16:
        return adv_regex_find_pattern16(regex_str, (pcre2_code_16 **)regex, start, max_len, offset, length, true, is_caseless, only_at_start);
    case TEXTPARSER_ENCODING_UTF_32:
        return adv_regex_find_pattern32(regex_str, (pcre2_code_32 **)regex, start, max_len, offset, length, is_caseless, only_at_start);
    default:
        fprintf(stderr, "Illegal text encoding(%d) at adv_regex_find_pattern()\n", encoding);
        break;
    }

    return false;
}

void adv_regex_free(void **regex, enum textparser_encoding encoding)
{
    if ((regex == nullptr)||((*regex == nullptr)))
        return;

    switch(encoding)
    {
    case TEXTPARSER_ENCODING_LATIN1:
    case TEXTPARSER_ENCODING_UTF_8:
        pcre2_code_free_8(*(pcre2_code_8 **)regex);
        break;
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
        pcre2_code_free_16(*(pcre2_code_16 **)regex);
        break;
    case TEXTPARSER_ENCODING_UTF_32:
        pcre2_code_free_32(*(pcre2_code_32 **)regex);
        break;
    default:
        fprintf(stderr, "Illegal text encoding(%d) at adv_regex_free()\n", encoding);
        break;
    }

    *regex = nullptr;
}
