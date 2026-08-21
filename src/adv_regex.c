#include "adv_regex.h"
#include "os.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define PCRE2_ANCHORED            0x80000000u
#define PCRE2_CASELESS            0x00000008u
#define PCRE2_UTF                 0x00080000u
#define PCRE2_NEWLINE_ANY         0x00040000u
#define PCRE2_ZERO_TERMINATED     (~((size_t)0))
#define PCRE2_UNSET               (~((size_t)0))
#define PCRE2_JIT_COMPLETE        0x00000001u

typedef size_t PCRE2_SIZE;
typedef unsigned char PCRE2_UCHAR8;

/* Width index for the ccontext[] array in adv_regex_context. */
typedef enum {
    PCRE2_WIDTH_8     = 0,
    PCRE2_WIDTH_16    = 1,
    PCRE2_WIDTH_32    = 2,
    PCRE2_WIDTH_COUNT = 3,
} pcre2_width_t;

/* Dynamic function pointers per width */
typedef struct {
    void *lib_handle;
    bool  loaded;
    bool  failed;

    void *(*compile_context_create)(void *gctx);
    void  (*compile_context_set_newline)(void *cctx, uint32_t value);
    void  (*compile_context_free)(void *cctx);

    void *(*compile)(const void *pattern, PCRE2_SIZE len, uint32_t options,
                     int *errcode, PCRE2_SIZE *erroffset, void *cctx);
    int   (*jit_compile)(void *code, uint32_t options);
    void  (*code_free)(void *code);

    void *(*match_data_create)(uint32_t ovecsize, void *gctx);
    int   (*match)(const void *code, const void *subject, PCRE2_SIZE length,
                   PCRE2_SIZE startoffset, uint32_t options,
                   void *match_data, void *mctx);
    PCRE2_SIZE *(*get_ovector_pointer)(void *match_data);
    void  (*match_data_free)(void *match_data);
    int   (*get_error_message)(int enumber, void *buffer, PCRE2_SIZE size);
} pcre2_dyn_api_t;

static pcre2_dyn_api_t g_pcre2_dyn[PCRE2_WIDTH_COUNT];

static bool load_pcre2_dyn(pcre2_width_t width_idx)
{
    pcre2_dyn_api_t *api = &g_pcre2_dyn[width_idx];
    if (api->loaded) return true;
    if (api->failed) return false;

#if defined(_WIN32)
    static const char *const names[PCRE2_WIDTH_COUNT] = { "pcre2-8.dll",       "pcre2-16.dll",       "pcre2-32.dll"       };
#elif defined(__APPLE__)
    static const char *const names[PCRE2_WIDTH_COUNT] = { "libpcre2-8.dylib",  "libpcre2-16.dylib",  "libpcre2-32.dylib"  };
#else
    static const char *const names[PCRE2_WIDTH_COUNT] = { "libpcre2-8.so",     "libpcre2-16.so",     "libpcre2-32.so"     };
#endif
    const char *suffix[PCRE2_WIDTH_COUNT] = { "8", "16", "32" };
    const char *sfx = suffix[width_idx];

    api->lib_handle = os_dlopen(names[width_idx]);

    if (!api->lib_handle) {
        fprintf(stderr, "adv_regex: Failed to dynamically load PCRE2 %s library.\n", sfx);
        api->failed = true;
        return false;
    }

    char sym[128];
#define LOAD_SYM(field, prefix) do { \
        snprintf(sym, sizeof(sym), "%s_%s", prefix, sfx); \
        void *p = os_dlsym(api->lib_handle, sym); \
        if (!p) { \
            fprintf(stderr, "adv_regex: Missing symbol %s in PCRE2 %s library.\n", sym, sfx); \
            api->failed = true; \
            return false; \
        } \
        *(void **)(void *)&api->field = p; \
    } while (0)

    LOAD_SYM(compile_context_create, "pcre2_compile_context_create");
    LOAD_SYM(compile_context_set_newline, "pcre2_set_newline");
    LOAD_SYM(compile_context_free, "pcre2_compile_context_free");
    LOAD_SYM(compile, "pcre2_compile");
    LOAD_SYM(jit_compile, "pcre2_jit_compile");
    LOAD_SYM(code_free, "pcre2_code_free");
    LOAD_SYM(match_data_create, "pcre2_match_data_create");
    LOAD_SYM(match, "pcre2_match");
    LOAD_SYM(get_ovector_pointer, "pcre2_get_ovector_pointer");
    LOAD_SYM(match_data_free, "pcre2_match_data_free");
    LOAD_SYM(get_error_message, "pcre2_get_error_message");
#undef LOAD_SYM

    api->loaded = true;
    return true;
}

/* Compile-context and match-data slots, one per supported PCRE2 code-unit width. */
struct adv_regex_context {
    void *ccontext[PCRE2_WIDTH_COUNT];
    void *match_data[PCRE2_WIDTH_COUNT];
};

adv_regex_context *adv_regex_context_create(void)
{
    return (adv_regex_context *)calloc(1, sizeof(adv_regex_context));
}

void adv_regex_context_free(adv_regex_context *ctx)
{
    if (!ctx) return;
    for (int w = 0; w < PCRE2_WIDTH_COUNT; w++) {
        pcre2_dyn_api_t *api = &g_pcre2_dyn[w];
        if (api->loaded) {
            if (ctx->match_data[w]) api->match_data_free(ctx->match_data[w]);
            if (ctx->ccontext[w])   api->compile_context_free(ctx->ccontext[w]);
        }
    }
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
    if (!utf8) return NULL;

    size_t len = 0;
    const unsigned char *p = (const unsigned char *)utf8;
    while (*p) {
        uint32_t cp = decode_one_utf8_codepoint(&p);
        len += (cp < 0x10000) ? 1 : 2;
    }

    uint16_t *utf16 = malloc((len + 1) * sizeof(uint16_t));
    if (!utf16) return NULL;

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
    if (!utf8) return NULL;

    size_t len = 0;
    const unsigned char *p = (const unsigned char *)utf8;
    while (*p) { decode_one_utf8_codepoint(&p); len++; }

    uint32_t *utf32 = malloc((len + 1) * sizeof(uint32_t));
    if (!utf32) return NULL;

    size_t idx = 0;
    p = (const unsigned char *)utf8;
    while (*p) { utf32[idx++] = decode_one_utf8_codepoint(&p); }
    utf32[idx] = 0;
    if (out_len) *out_len = idx;
    return utf32;
}

/* --- Single generic implementation --------------------------------------- */

static bool adv_regex_find_pattern_impl(
    adv_regex_context       *ctx,
    pcre2_width_t            width_idx,
    int                      width_bits,
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
    if (!load_pcre2_dyn(width_idx)) return false;
    pcre2_dyn_api_t *api = &g_pcre2_dyn[width_idx];

    /* Lazily initialise the compile context for this width. */
    void **cctx_slot = &ctx->ccontext[width_idx];
    if (*cctx_slot == NULL) {
        *cctx_slot = api->compile_context_create(NULL);
        if (!*cctx_slot) return false;
        api->compile_context_set_newline(*cctx_slot, PCRE2_NEWLINE_ANY);
    }

    /* Compile the pattern on first use. */
    if (*regex == NULL) {
        uint32_t options = 0;
        if (is_utf)      options |= PCRE2_UTF;
        if (is_caseless) options |= PCRE2_CASELESS;

        void       *pattern_conv    = NULL;
        const void *compile_pattern = regex_str;

        if (width_bits == 16) {
            compile_pattern = pattern_conv = utf8_to_utf16(regex_str, NULL);
            if (!compile_pattern) return false;
        } else if (width_bits == 32) {
            compile_pattern = pattern_conv = utf8_to_utf32(regex_str, NULL);
            if (!compile_pattern) return false;
        }

        PCRE2_SIZE error_offset = 0;
        int        error_number = 0;
        *regex = api->compile(compile_pattern, PCRE2_ZERO_TERMINATED, options,
                              &error_number, &error_offset, *cctx_slot);
        if (pattern_conv) free(pattern_conv);

        if (*regex == NULL) {
            char buffer[256];
            api->get_error_message(error_number, buffer, sizeof(buffer));
            fprintf(stderr, "PCRE2 compilation_%d failed at offset %zu: %s\n",
                    width_bits, (size_t)error_offset, buffer);
            return false;
        }
        api->jit_compile(*regex, PCRE2_JIT_COMPLETE);
    }

    void **mdata_slot = &ctx->match_data[width_idx];
    if (*mdata_slot == NULL) {
        *mdata_slot = api->match_data_create(16, NULL);
        if (!*mdata_slot) return false;
    }
    void *match_data = *mdata_slot;

    bool ret = false;
    int  rc  = api->match(*regex, (const void *)start, max_len, 0,
                          only_at_start ? PCRE2_ANCHORED : 0, match_data, NULL);

    if (rc == 1) {
        PCRE2_SIZE *ov = api->get_ovector_pointer(match_data);
        if (ov && ov[1] > 0) {
            if (offset) *offset = ov[0];
            if (length) *length = ov[1] - ov[0];
            ret = true;
        }
    } else if (rc >= 2) {
        PCRE2_SIZE *ov = api->get_ovector_pointer(match_data);
        if (ov && ov[2] != PCRE2_UNSET && ov[3] != PCRE2_UNSET && ov[3] > ov[2]) {
            if (offset) *offset = ov[2];
            if (length) *length = ov[3] - ov[2];
            ret = true;
        }
    }

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
        return adv_regex_find_pattern_impl(ctx, PCRE2_WIDTH_8, 8,
            regex_str, regex, false, is_caseless, start, max_len, offset, length, only_at_start);
    case TEXTPARSER_ENCODING_UTF_8:
        return adv_regex_find_pattern_impl(ctx, PCRE2_WIDTH_8, 8,
            regex_str, regex, true,  is_caseless, start, max_len, offset, length, only_at_start);
    case TEXTPARSER_ENCODING_UNICODE:
        return adv_regex_find_pattern_impl(ctx, PCRE2_WIDTH_16, 16,
            regex_str, regex, false, is_caseless, start, max_len, offset, length, only_at_start);
    case TEXTPARSER_ENCODING_UTF_16:
        return adv_regex_find_pattern_impl(ctx, PCRE2_WIDTH_16, 16,
            regex_str, regex, true,  is_caseless, start, max_len, offset, length, only_at_start);
    case TEXTPARSER_ENCODING_UTF_32:
        return adv_regex_find_pattern_impl(ctx, PCRE2_WIDTH_32, 32,
            regex_str, regex, true,  is_caseless, start, max_len, offset, length, only_at_start);
    default:
        fprintf(stderr, "Illegal text encoding(%d) at adv_regex_find_pattern()\n", encoding);
        return false;
    }
}

void adv_regex_free(void **regex, enum textparser_encoding encoding)
{
    if (!regex || !*regex) return;

    pcre2_width_t w = PCRE2_WIDTH_8;
    switch (encoding) {
    case TEXTPARSER_ENCODING_LATIN1:
    case TEXTPARSER_ENCODING_UTF_8:
        w = PCRE2_WIDTH_8;
        break;
    case TEXTPARSER_ENCODING_UNICODE:
    case TEXTPARSER_ENCODING_UTF_16:
        w = PCRE2_WIDTH_16;
        break;
    case TEXTPARSER_ENCODING_UTF_32:
        w = PCRE2_WIDTH_32;
        break;
    default:
        fprintf(stderr, "Illegal text encoding(%d) at adv_regex_free()\n", encoding);
        *regex = NULL;
        return;
    }

    if (g_pcre2_dyn[w].loaded && g_pcre2_dyn[w].code_free) {
        g_pcre2_dyn[w].code_free(*regex);
    }
    *regex = NULL;
}
