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


#define DEFINE_ADV_REGEX_FIND_PATTERN(bits) \
static bool adv_regex_find_pattern##bits(const char *regex_str, pcre2_code_##bits **regex, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_utf, bool is_caseless, bool only_at_start) \
{ \
    bool ret = false; \
 \
    PCRE2_SIZE error_offset = 0; \
    int error_number = 0; \
    pcre2_match_data_##bits *match_data = nullptr; \
    PCRE2_SIZE *ovector = nullptr; \
 \
    if (ccontext##bits == nullptr) \
    { \
        ccontext##bits = pcre2_compile_context_create_##bits(nullptr); \
        pcre2_set_newline_##bits(ccontext##bits, PCRE2_NEWLINE_ANY); \
    } \
 \
    if (*regex == nullptr) \
    { \
        uint32_t options = 0; \
        if (is_utf) options |= PCRE2_UTF; \
        if (is_caseless) options |= PCRE2_CASELESS; \
        uint##bits##_t *pattern_conv = nullptr; \
        void *compile_pattern = (void *)regex_str; \
        if (bits == 16) { \
            pattern_conv = (uint##bits##_t *)(uint16_t *)utf8_to_utf16(regex_str, nullptr); \
            if (pattern_conv == nullptr) return false; \
            compile_pattern = pattern_conv; \
        } else if (bits == 32) { \
            pattern_conv = (uint##bits##_t *)(uint32_t *)utf8_to_utf32(regex_str, nullptr); \
            if (pattern_conv == nullptr) return false; \
            compile_pattern = pattern_conv; \
        } \
        *regex = pcre2_compile_##bits((PCRE2_SPTR##bits)compile_pattern, PCRE2_ZERO_TERMINATED, options, &error_number, &error_offset, ccontext##bits); \
        if (pattern_conv) free(pattern_conv); \
        if (*regex == nullptr) \
        { \
            PCRE2_UCHAR8 buffer[256]; \
            pcre2_get_error_message_8(error_number, buffer, sizeof(buffer)); \
            fprintf(stderr, "PCRE2 compilation_" #bits " failed at offset %zu: %s\n", error_offset, buffer); \
            return false; \
        } \
 \
        pcre2_jit_compile_##bits(*regex, PCRE2_JIT_COMPLETE); \
    } \
 \
    match_data = pcre2_match_data_create_from_pattern_##bits(*regex, nullptr); \
 \
    int rc = pcre2_match_##bits( \
        *regex, \
        (PCRE2_SPTR##bits)(const void *)start, \
        max_len, \
        0, \
        only_at_start ? PCRE2_ANCHORED : 0, \
        match_data, \
        nullptr); \
    if (rc == 1) \
    { \
        ovector = pcre2_get_ovector_pointer_##bits(match_data); \
        if ((ovector) && (ovector[1] > 0)) \
        { \
            if (offset) \
                *offset = ovector[0]; \
 \
            if (length) \
                *length = ovector[1] - ovector[0]; \
 \
            ret = true; \
        } \
    } \
    else if (rc >= 2) \
    { \
        ovector = pcre2_get_ovector_pointer_##bits(match_data); \
        if (ovector && ovector[2] != PCRE2_UNSET && ovector[3] != PCRE2_UNSET && ovector[3] > ovector[2]) \
        { \
            if (offset) \
                *offset = ovector[2]; \
 \
            if (length) \
                *length = ovector[3] - ovector[2]; \
 \
            ret = true; \
        } \
    } \
 \
    if (match_data) \
        pcre2_match_data_free_##bits(match_data); \
 \
    return ret; \
}

DEFINE_ADV_REGEX_FIND_PATTERN(8)
DEFINE_ADV_REGEX_FIND_PATTERN(16)
DEFINE_ADV_REGEX_FIND_PATTERN(32)
#undef DEFINE_ADV_REGEX_FIND_PATTERN

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
