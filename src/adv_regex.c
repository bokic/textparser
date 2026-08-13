#define PCRE2_CODE_UNIT_WIDTH 0

#include "adv_regex.h"
#include <pcre2.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

/* Width index for the ccontext[] array in adv_regex_context. */
typedef enum {
    PCRE2_WIDTH_8     = 0,
    PCRE2_WIDTH_16    = 1,
    PCRE2_WIDTH_32    = 2,
    PCRE2_WIDTH_COUNT = 3,  /* total number of supported widths; use for array sizing */
} pcre2_width_t;

/* Compile-context slots, one per supported PCRE2 code-unit width. */
struct adv_regex_context {
    void *ccontext[PCRE2_WIDTH_COUNT];
};

adv_regex_context *adv_regex_context_create(void)
{
    return (adv_regex_context *)calloc(1, sizeof(adv_regex_context));
}

void adv_regex_context_free(adv_regex_context *ctx)
{
    if (!ctx) return;
    if (ctx->ccontext[PCRE2_WIDTH_8])  pcre2_compile_context_free_8(ctx->ccontext[PCRE2_WIDTH_8]);
    if (ctx->ccontext[PCRE2_WIDTH_16]) pcre2_compile_context_free_16(ctx->ccontext[PCRE2_WIDTH_16]);
    if (ctx->ccontext[PCRE2_WIDTH_32]) pcre2_compile_context_free_32(ctx->ccontext[PCRE2_WIDTH_32]);
    free(ctx);
}

/* --- UTF-8 decoder -------------------------------------------------------- */

static uint32_t decode_one_utf8_codepoint(const unsigned char **p)
{
    const unsigned char *s = *p;
    if (!*s) return 0;

    uint32_t cp = 0;
    if (*s < 0x80) {
        cp = *s++;
    } else if (*s < 0xE0) {
        cp = (*s++ & 0x1F) << 6;
        if (*s && (*s & 0xC0) == 0x80) cp |= (*s++ & 0x3F);
    } else if (*s < 0xF0) {
        cp = (*s++ & 0x0F) << 12;
        if (*s && (*s & 0xC0) == 0x80) {
            cp |= (*s++ & 0x3F) << 6;
            if (*s && (*s & 0xC0) == 0x80) cp |= (*s++ & 0x3F);
        }
    } else {
        cp = (*s++ & 0x07) << 18;
        if (*s && (*s & 0xC0) == 0x80) {
            cp |= (*s++ & 0x3F) << 12;
            if (*s && (*s & 0xC0) == 0x80) {
                cp |= (*s++ & 0x3F) << 6;
                if (*s && (*s & 0xC0) == 0x80) cp |= (*s++ & 0x3F);
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
        len += (cp < 0x10000) ? 1 : 2;
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
    while (*p) { decode_one_utf8_codepoint(&p); len++; }

    uint32_t *utf32 = malloc((len + 1) * sizeof(uint32_t));
    if (!utf32) return nullptr;

    size_t idx = 0;
    p = (const unsigned char *)utf8;
    while (*p) { utf32[idx++] = decode_one_utf8_codepoint(&p); }
    utf32[idx] = 0;
    if (out_len) *out_len = idx;
    return utf32;
}

/* --- PCRE2 vtable --------------------------------------------------------- */

typedef struct {
    pcre2_width_t index;  /* slot index in adv_regex_context.ccontext[] */
    int           width;  /* bit width: 8, 16 or 32 */
    void *(*ccontext_create)(void *gctx);
    void  (*ccontext_set_newline)(void *cctx, uint32_t value);
    void *(*compile)(const void *pattern, PCRE2_SIZE len, uint32_t options,
                     int *errcode, PCRE2_SIZE *erroffset, void *cctx);
    int   (*jit_compile)(void *code, uint32_t options);
    void *(*match_data_create)(const void *code, void *gctx);
    int   (*match)(const void *code, const void *subject, PCRE2_SIZE length,
                   PCRE2_SIZE startoffset, uint32_t options,
                   void *match_data, void *mctx);
    PCRE2_SIZE *(*get_ovector)(void *match_data);
    void  (*match_data_free)(void *match_data);
} pcre2_api_t;

/*
 * MAKE_PCRE2_VT(bits) generates thin type-casting wrapper functions and a
 * corresponding static pcre2_api_t instance.  Only boilerplate type plumbing
 * lives here — no algorithmic logic.
 */
#define MAKE_PCRE2_VT(bits, slot_index)                                                               \
static void *_vt_cctx_create_##bits(void *g)                                                         \
    { return pcre2_compile_context_create_##bits(g); }                                                \
static void  _vt_cctx_newline_##bits(void *c, uint32_t v)                                            \
    { pcre2_set_newline_##bits(c, v); }                                                               \
static void *_vt_compile_##bits(const void *p, PCRE2_SIZE l, uint32_t o,                             \
    int *e, PCRE2_SIZE *eo, void *c)                                                                  \
    { return pcre2_compile_##bits((PCRE2_SPTR##bits)p, l, o, e, eo, c); }                            \
static int   _vt_jit_##bits(void *c, uint32_t o)                                                     \
    { return pcre2_jit_compile_##bits(c, o); }                                                        \
static void *_vt_mdata_create_##bits(const void *c, void *g)                                         \
    { return pcre2_match_data_create_from_pattern_##bits(c, g); }                                     \
static int   _vt_match_##bits(const void *c, const void *s, PCRE2_SIZE l,                            \
    PCRE2_SIZE so, uint32_t o, void *md, void *mc)                                                    \
    { return pcre2_match_##bits(c, (PCRE2_SPTR##bits)s, l, so, o, md, mc); }                         \
static PCRE2_SIZE *_vt_ovector_##bits(void *md)                                                       \
    { return pcre2_get_ovector_pointer_##bits(md); }                                                   \
static void  _vt_mdata_free_##bits(void *md)                                                          \
    { pcre2_match_data_free_##bits(md); }                                                              \
static const pcre2_api_t k_pcre2_api_##bits = {                                                       \
    slot_index, bits,                                                                                  \
    _vt_cctx_create_##bits, _vt_cctx_newline_##bits, _vt_compile_##bits, _vt_jit_##bits,             \
    _vt_mdata_create_##bits, _vt_match_##bits, _vt_ovector_##bits, _vt_mdata_free_##bits,            \
};

MAKE_PCRE2_VT(8,  PCRE2_WIDTH_8)
MAKE_PCRE2_VT(16, PCRE2_WIDTH_16)
MAKE_PCRE2_VT(32, PCRE2_WIDTH_32)
#undef MAKE_PCRE2_VT

/* --- Single generic implementation --------------------------------------- */

static bool adv_regex_find_pattern_impl(
    adv_regex_context       *ctx,
    const pcre2_api_t       *api,
    const char              *regex_str,
    void                   **regex,
    bool                     is_utf,
    bool                     is_caseless,
    const char              *start,
    size_t                   max_len,
    size_t                  *offset,
    size_t                  *length,
    bool                     only_at_start)
{
    /* Lazily initialise the compile context for this width. */
    void **cctx_slot = &ctx->ccontext[api->index];
    if (*cctx_slot == nullptr) {
        *cctx_slot = api->ccontext_create(nullptr);
        if (*cctx_slot) api->ccontext_set_newline(*cctx_slot, PCRE2_NEWLINE_ANY);
    }

    /* Compile the pattern on first use. */
    if (*regex == nullptr) {
        uint32_t options = 0;
        if (is_utf)      options |= PCRE2_UTF;
        if (is_caseless) options |= PCRE2_CASELESS;

        void       *pattern_conv    = nullptr;
        const void *compile_pattern = regex_str;

        if (api->width == 16) {
            compile_pattern = pattern_conv = utf8_to_utf16(regex_str, nullptr);
            if (!compile_pattern) return false;
        } else if (api->width == 32) {
            compile_pattern = pattern_conv = utf8_to_utf32(regex_str, nullptr);
            if (!compile_pattern) return false;
        }

        PCRE2_SIZE error_offset = 0;
        int        error_number = 0;
        *regex = api->compile(compile_pattern, PCRE2_ZERO_TERMINATED, options,
                              &error_number, &error_offset, *cctx_slot);
        if (pattern_conv) free(pattern_conv);

        if (*regex == nullptr) {
            PCRE2_UCHAR8 buffer[256];
            pcre2_get_error_message_8(error_number, buffer, sizeof(buffer));
            fprintf(stderr, "PCRE2 compilation_%d failed at offset %zu: %s\n",
                    api->width, (size_t)error_offset, buffer);
            return false;
        }
        api->jit_compile(*regex, PCRE2_JIT_COMPLETE);
    }

    void *match_data = api->match_data_create(*regex, nullptr);
    if (!match_data) return false;

    bool ret = false;
    int  rc  = api->match(*regex, (const void *)start, max_len, 0,
                          only_at_start ? PCRE2_ANCHORED : 0, match_data, nullptr);

    if (rc == 1) {
        PCRE2_SIZE *ov = api->get_ovector(match_data);
        if (ov && ov[1] > 0) {
            if (offset) *offset = ov[0];
            if (length) *length = ov[1] - ov[0];
            ret = true;
        }
    } else if (rc >= 2) {
        PCRE2_SIZE *ov = api->get_ovector(match_data);
        if (ov && ov[2] != PCRE2_UNSET && ov[3] != PCRE2_UNSET && ov[3] > ov[2]) {
            if (offset) *offset = ov[2];
            if (length) *length = ov[3] - ov[2];
            ret = true;
        }
    }

    api->match_data_free(match_data);
    return ret;
}

/* --- Public dispatch ------------------------------------------------------- */

bool adv_regex_find_pattern_ctx(
    adv_regex_context       *ctx,
    const char              *regex_str,
    void                   **regex,
    enum textparser_encoding encoding,
    const char              *start,
    size_t                   max_len,
    size_t                  *offset,
    size_t                  *length,
    bool                     is_caseless,
    bool                     only_at_start)
{
    if (!ctx || !regex_str) return false;

    switch (encoding) {
    case TEXTPARSER_ENCODING_LATIN1:
        return adv_regex_find_pattern_impl(ctx, &k_pcre2_api_8,
            regex_str, regex, false, is_caseless, start, max_len, offset, length, only_at_start);
    case TEXTPARSER_ENCODING_UTF_8:
        return adv_regex_find_pattern_impl(ctx, &k_pcre2_api_8,
            regex_str, regex, true,  is_caseless, start, max_len, offset, length, only_at_start);
    case TEXTPARSER_ENCODING_UNICODE:
        return adv_regex_find_pattern_impl(ctx, &k_pcre2_api_16,
            regex_str, regex, false, is_caseless, start, max_len, offset, length, only_at_start);
    case TEXTPARSER_ENCODING_UTF_16:
        return adv_regex_find_pattern_impl(ctx, &k_pcre2_api_16,
            regex_str, regex, true,  is_caseless, start, max_len, offset, length, only_at_start);
    case TEXTPARSER_ENCODING_UTF_32:
        return adv_regex_find_pattern_impl(ctx, &k_pcre2_api_32,
            regex_str, regex, true,  is_caseless, start, max_len, offset, length, only_at_start);
    default:
        fprintf(stderr, "Illegal text encoding(%d) at adv_regex_find_pattern()\n", encoding);
        return false;
    }
}

void adv_regex_free(void **regex, enum textparser_encoding encoding)
{
    if (!regex || !*regex) return;

    switch (encoding) {
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
