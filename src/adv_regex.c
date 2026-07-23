#define PCRE2_CODE_UNIT_WIDTH 0

#include "adv_regex.h"
#include <pcre2.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

static pcre2_compile_context_8 *ccontext8 = nullptr;
static pcre2_compile_context_16 *ccontext16 = nullptr;
static pcre2_compile_context_32 *ccontext32 = nullptr;

static uint32_t decode_one_utf8_codepoint(const unsigned char **p)
{
    const unsigned char *s = *p;
    if (!*s) return 0;

    uint32_t cp = 0;
    if (*s < 0x80) {
        cp = *s++;
    } else if (*s < 0xE0) {
        cp = (*s++ & 0x1F) << 6;
        if ((*s & 0xC0) == 0x80) cp |= (*s++ & 0x3F);
    } else if (*s < 0xF0) {
        cp = (*s++ & 0x0F) << 12;
        if ((*s & 0xC0) == 0x80) {
            cp |= (*s++ & 0x3F) << 6;
            if ((*s & 0xC0) == 0x80) cp |= (*s++ & 0x3F);
        }
    } else {
        cp = (*s++ & 0x07) << 18;
        if ((*s & 0xC0) == 0x80) {
            cp |= (*s++ & 0x3F) << 12;
            if ((*s & 0xC0) == 0x80) {
                cp |= (*s++ & 0x3F) << 6;
                if ((*s & 0xC0) == 0x80) cp |= (*s++ & 0x3F);
            }
        }
    }
    *p = s;
    return cp;
}

static uint16_t *utf8_to_utf16(const char *utf8, size_t *out_len)
{
    if (!utf8) return nullptr;

    size_t len = 0;
    const unsigned char *p = (const unsigned char *)utf8;
    while (*p) {
        uint32_t cp = decode_one_utf8_codepoint(&p);
        if (cp < 0x10000) {
            len++;
        } else {
            len += 2;
        }
    }
    
    uint16_t *utf16 = malloc((len + 1) * sizeof(uint16_t));
    if (!utf16) return nullptr;
    
    size_t idx = 0;
    p = (const unsigned char *)utf8;
    while (*p) {
        uint32_t cp = decode_one_utf8_codepoint(&p);
        if (cp < 0x10000) {
            utf16[idx++] = (uint16_t)cp;
        } else {
            cp -= 0x10000;
            utf16[idx++] = (uint16_t)((cp >> 10) + 0xD800);
            utf16[idx++] = (uint16_t)((cp & 0x3FF) + 0xDC00);
        }
    }
    utf16[idx] = 0;
    if (out_len) *out_len = idx;
    return utf16;
}

static uint32_t *utf8_to_utf32(const char *utf8, size_t *out_len)
{
    if (!utf8) return nullptr;

    size_t len = 0;
    const unsigned char *p = (const unsigned char *)utf8;
    while (*p) {
        decode_one_utf8_codepoint(&p);
        len++;
    }
    
    uint32_t *utf32 = malloc((len + 1) * sizeof(uint32_t));
    if (!utf32) return nullptr;
    
    size_t idx = 0;
    p = (const unsigned char *)utf8;
    while (*p) {
        utf32[idx++] = decode_one_utf8_codepoint(&p);
    }
    utf32[idx] = 0;
    if (out_len) *out_len = idx;
    return utf32;
}


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
            fprintf(stderr, "PCRE2 compilation_8 failed at offset %zu: %s\n", error_offset, buffer);
            return false;
        }

        pcre2_jit_compile_8(*regex, PCRE2_JIT_COMPLETE);
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
    // rc == 1: No capturing groups present. Use ovector[0]/[1] (overall match).
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
    // rc >= 2: Regex contains capturing groups. By convention, capture group 1
    // (ovector[2]/[3]) specifies the target token slice rather than the full regex match.
    else if (rc >= 2)
    {
        ovector = pcre2_get_ovector_pointer_8(match_data);
        if (ovector && ovector[2] != PCRE2_UNSET && ovector[3] != PCRE2_UNSET && ovector[3] > ovector[2])
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
        uint16_t *pattern16 = utf8_to_utf16(regex_str, nullptr);
        if (pattern16 == nullptr) {
            return false;
        }
        *regex = pcre2_compile_16((PCRE2_SPTR16)pattern16, PCRE2_ZERO_TERMINATED, options, &error_number, &error_offset, ccontext16);
        free(pattern16);
        if (*regex == nullptr)
        {
            PCRE2_UCHAR8 buffer[256];
            pcre2_get_error_message_8(error_number, buffer, sizeof(buffer));
            fprintf(stderr, "PCRE2 compilation_16 failed at offset %zu: %s\n", error_offset, buffer);
            return false;
        }

        pcre2_jit_compile_16(*regex, PCRE2_JIT_COMPLETE);
    }

    match_data = pcre2_match_data_create_from_pattern_16(*regex, nullptr);

    int rc = pcre2_match_16(
        *regex,                                // the compiled pattern
        (PCRE2_SPTR16)(const void *)start,     // the subject string
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
    else if (rc >= 2)
    {
        ovector = pcre2_get_ovector_pointer_16(match_data);
        if (ovector && ovector[2] != PCRE2_UNSET && ovector[3] != PCRE2_UNSET && ovector[3] > ovector[2])
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

static bool adv_regex_find_pattern32(const char *regex_str, pcre2_code_32 **regex, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_utf, bool is_caseless, bool only_at_start)
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
        if (is_utf) options |= PCRE2_UTF;
        if (is_caseless) options |= PCRE2_CASELESS;
        uint32_t *pattern32 = utf8_to_utf32(regex_str, nullptr);
        if (pattern32 == nullptr) {
            return false;
        }
        *regex = pcre2_compile_32((PCRE2_SPTR32)pattern32, PCRE2_ZERO_TERMINATED, options, &error_number, &error_offset, ccontext32);
        free(pattern32);
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
        (PCRE2_SPTR32)(const void *)start,     // the subject string
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
    else if (rc >= 2)
    {
        ovector = pcre2_get_ovector_pointer_32(match_data);
        if (ovector && ovector[2] != PCRE2_UNSET && ovector[3] != PCRE2_UNSET && ovector[3] > ovector[2])
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
        return adv_regex_find_pattern32(regex_str, (pcre2_code_32 **)regex, start, max_len, offset, length, true, is_caseless, only_at_start);
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

void adv_regex_cleanup(void)
{
    if (ccontext8) {
        pcre2_compile_context_free_8(ccontext8);
        ccontext8 = nullptr;
    }
    if (ccontext16) {
        pcre2_compile_context_free_16(ccontext16);
        ccontext16 = nullptr;
    }
    if (ccontext32) {
        pcre2_compile_context_free_32(ccontext32);
        ccontext32 = nullptr;
    }
}
