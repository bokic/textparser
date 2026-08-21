#include "search_function_gen.h"
#include <string.h>
#include <ctype.h>
#include <stdint.h>

/* --- Multi-encoding Helper Macros --- */

static inline uint32_t get_char_at(enum textparser_encoding enc, const void *buf, size_t idx) {
    if (enc == TEXTPARSER_ENCODING_UTF_32) {
        return ((const uint32_t *)buf)[idx];
    } else if (enc == TEXTPARSER_ENCODING_UNICODE || enc == TEXTPARSER_ENCODING_UTF_16) {
        return ((const uint16_t *)buf)[idx];
    } else {
        return (uint32_t)((const unsigned char *)buf)[idx];
    }
}

static inline bool is_space_codepoint(uint32_t ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static inline bool is_alpha_codepoint(uint32_t ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

static inline bool is_digit_codepoint(uint32_t ch) {
    return ch >= '0' && ch <= '9';
}

static inline bool is_alnum_codepoint(uint32_t ch) {
    return is_alpha_codepoint(ch) || is_digit_codepoint(ch);
}

static inline bool is_xdigit_codepoint(uint32_t ch) {
    return is_digit_codepoint(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

static inline uint32_t to_lower_codepoint(uint32_t ch) {
    if (ch >= 'A' && ch <= 'Z') return ch + ('a' - 'A');
    return ch;
}

static inline bool str_match_at(enum textparser_encoding enc, const void *buf, size_t pos, size_t max_len, const char *str, bool caseless) {
    size_t slen = strlen(str);
    if (pos + slen > max_len) return false;
    for (size_t i = 0; i < slen; i++) {
        uint32_t c = get_char_at(enc, buf, pos + i);
        uint32_t expected = (uint32_t)(unsigned char)str[i];
        if (caseless) {
            if (to_lower_codepoint(c) != to_lower_codepoint(expected)) return false;
        } else {
            if (c != expected) return false;
        }
    }
    return true;
}

/* ========================================================================= */
/* --- C Grammar Native Matchers ---                                         */
/* ========================================================================= */

// LineComment: \/\/[^\r\n]*
bool _gen_c_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len >= 2 && get_char_at(encoding, start, 0) == '/' && get_char_at(encoding, start, 1) == '/') {
            size_t i = 2;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (c == '\r' || c == '\n') break;
                i++;
            }
            if (offset) *offset = 0;
            if (length) *length = i;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '/' && get_char_at(encoding, start, pos + 1) == '/') {
            size_t i = pos + 2;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (c == '\r' || c == '\n') break;
                i++;
            }
            if (offset) *offset = pos;
            if (length) *length = i - pos;
            return true;
        }
    }
    return false;
}

// BlockComment: start=/\*, end=\*/
bool _gen_c_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len >= 2 && get_char_at(encoding, start, 0) == '/' && get_char_at(encoding, start, 1) == '*') {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '/' && get_char_at(encoding, start, pos + 1) == '*') {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

bool _gen_c_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len >= 2 && get_char_at(encoding, start, 0) == '*' && get_char_at(encoding, start, 1) == '/') {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '*' && get_char_at(encoding, start, pos + 1) == '/') {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

// Preprocessor: #[ \t]*[a-zA-Z_][a-zA-Z0-9_]*
static size_t c_preproc_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len || get_char_at(encoding, start, pos) != '#') return 0;
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (c == ' ' || c == '\t') i++;
        else break;
    }
    if (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alpha_codepoint(c) || c == '_') {
            i++;
            while (i < max_len) {
                uint32_t c2 = get_char_at(encoding, start, i);
                if (is_alnum_codepoint(c2) || c2 == '_') i++;
                else break;
            }
            return i - pos;
        }
    }
    return 0;
}

bool _gen_c_Preprocessor_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = c_preproc_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = c_preproc_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Keyword:
static const char *const c_keywords[] = {
    "int", "char", "double", "float", "void", "short", "long", "unsigned", "signed",
    "struct", "union", "enum", "typedef", "const", "static", "extern", "volatile",
    "if", "else", "for", "while", "do", "switch", "case", "default", "break",
    "continue", "return", "sizeof", "goto", "register", "auto", "inline", "restrict",
    "_Bool", "bool", "size_t", "ssize_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    "int8_t", "int16_t", "int32_t", "int64_t", "uintptr_t", "intptr_t", "ptrdiff_t",
    NULL
};

static size_t c_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; c_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(c_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, c_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_c_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = c_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = c_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Variable: [a-zA-Z_][a-zA-Z0-9_]*
static size_t c_var_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t c2 = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c2) || c2 == '_') i++;
        else break;
    }
    return i - pos;
}

bool _gen_c_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = c_var_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = c_var_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// CodeBlock: start=\{ end=\}
bool _gen_c_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '{') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '{') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

bool _gen_c_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '}') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '}') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

// Operator:
static size_t c_operator_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        if (str_match_at(encoding, start, pos, max_len, "<<=", false) ||
            str_match_at(encoding, start, pos, max_len, ">>=", false)) {
            return 3;
        }
    }
    if (pos + 2 <= max_len) {
        static const char *const ops2[] = {
            "+=", "-=", "*=", "/=", "%=", "&=", "^=", "|=", "<<", ">>",
            "++", "--", "&&", "||", "<=", ">=", "==", "!=", "->", NULL
        };
        for (int k = 0; ops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, ops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '&' ||
        c == '|' || c == '^' || c == '~' || c == '+' || c == '-' ||
        c == '*' || c == '/' || c == '%' || c == '?' || c == ':' ||
        c == ';' || c == '.' || c == ',') {
        return 1;
    }
    return 0;
}

bool _gen_c_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = c_operator_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = c_operator_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// SingleString: '
bool _gen_c_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '\'') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '\'') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}
bool _gen_c_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// DoubleString: "
bool _gen_c_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '"') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '"') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}
bool _gen_c_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// StringEscape: \\\\|\\\"|\\\'|\\n|\\r|\\t|\\u[0-9a-fA-F]{4}|\\x[0-9a-fA-F]{2}
static size_t c_escape_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '\\') return 0;
    uint32_t c = get_char_at(encoding, start, pos + 1);
    if (c == '\\' || c == '"' || c == '\'' || c == 'n' || c == 'r' || c == 't') return 2;
    if (c == 'x' && pos + 4 <= max_len &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 2)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 3))) {
        return 4;
    }
    if (c == 'u' && pos + 6 <= max_len &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 2)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 3)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 4)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 5))) {
        return 6;
    }
    return 0;
}

bool _gen_c_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = c_escape_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = c_escape_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Number: [0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?[fFdDlL]?
static size_t c_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    size_t i = pos;
    bool has_digits = false;
    while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) {
        i++;
        has_digits = true;
    }
    if (i < max_len && get_char_at(encoding, start, i) == '.') {
        if (i + 1 < max_len && is_digit_codepoint(get_char_at(encoding, start, i + 1))) {
            i += 2;
            while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
            has_digits = true;
        } else if (!has_digits) {
            return 0;
        }
    }
    if (!has_digits) return 0;
    if (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (c == 'e' || c == 'E') {
            size_t e_pos = i;
            i++;
            if (i < max_len) {
                uint32_t sign = get_char_at(encoding, start, i);
                if (sign == '+' || sign == '-') i++;
            }
            size_t exp_digits = 0;
            while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) {
                i++;
                exp_digits++;
            }
            if (exp_digits == 0) i = e_pos;
        }
    }
    if (i < max_len) {
        uint32_t suf = get_char_at(encoding, start, i);
        if (suf == 'f' || suf == 'F' || suf == 'd' || suf == 'D' || suf == 'l' || suf == 'L') i++;
    }
    return i - pos;
}

bool _gen_c_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = c_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = c_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Boolean: true\b|false\b
static size_t c_bool_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (str_match_at(encoding, start, pos, max_len, "true", false)) {
        if (pos + 4 == max_len) return 4;
        uint32_t after = get_char_at(encoding, start, pos + 4);
        if (!is_alnum_codepoint(after) && after != '_') return 4;
    }
    if (str_match_at(encoding, start, pos, max_len, "false", false)) {
        if (pos + 5 == max_len) return 5;
        uint32_t after = get_char_at(encoding, start, pos + 5);
        if (!is_alnum_codepoint(after) && after != '_') return 5;
    }
    return 0;
}

bool _gen_c_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = c_bool_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = c_bool_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Parenthesis: ( and )
bool _gen_c_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '(') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '(') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

bool _gen_c_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == ')') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == ')') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

// ArrayIndex: [ and ]
bool _gen_c_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '[') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '[') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

bool _gen_c_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == ']') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == ']') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

bool _gen_c_TypeCast_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_c_TypeCast_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === JSON Matchers Implementation ===                                      */
/* ========================================================================= */

bool _gen_json_Object_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_json_Object_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_json_Array_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_json_Array_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Key start: \"(?=[^\"\\\\]*(?:\\\\.[^\"\\\\]*)*\"\\s*:)
static bool json_key_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos >= max_len || get_char_at(encoding, start, pos) != '"') return false;
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (c == '\\') {
            i += 2;
        } else if (c == '"') {
            i++;
            while (i < max_len && is_space_codepoint(get_char_at(encoding, start, i))) {
                i++;
            }
            if (i < max_len && get_char_at(encoding, start, i) == ':') {
                return true;
            }
            return false;
        } else {
            i++;
        }
    }
    return false;
}

bool _gen_json_Key_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (json_key_match_at(encoding, start, 0, max_len)) {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (json_key_match_at(encoding, start, pos, max_len)) {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

// Key end: (\")\s*: -> captures quote at group 1
static bool json_key_end_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos >= max_len || get_char_at(encoding, start, pos) != '"') return false;
    size_t i = pos + 1;
    while (i < max_len && is_space_codepoint(get_char_at(encoding, start, i))) {
        i++;
    }
    if (i < max_len && get_char_at(encoding, start, i) == ':') {
        return true;
    }
    return false;
}

bool _gen_json_Key_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (json_key_end_match_at(encoding, start, 0, max_len)) {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (json_key_end_match_at(encoding, start, pos, max_len)) {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

bool _gen_json_String_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_json_String_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Number: \d+(?:\.\d+)?(?:e[+-]?\d+)?
static size_t json_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len || !is_digit_codepoint(get_char_at(encoding, start, pos))) return 0;
    size_t i = pos + 1;
    while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
    if (i < max_len && get_char_at(encoding, start, i) == '.') {
        if (i + 1 < max_len && is_digit_codepoint(get_char_at(encoding, start, i + 1))) {
            i += 2;
            while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
        }
    }
    if (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (c == 'e' || c == 'E') {
            size_t e_pos = i;
            i++;
            if (i < max_len) {
                uint32_t sign = get_char_at(encoding, start, i);
                if (sign == '+' || sign == '-') i++;
            }
            size_t exp_digits = 0;
            while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) {
                i++;
                exp_digits++;
            }
            if (exp_digits == 0) i = e_pos;
        }
    }
    return i - pos;
}

bool _gen_json_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = json_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = json_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// StringEscape: \\\\|\\\/|\\b|\\f|\\n|\\r|\\t|\\u[0-9a-f]{4}
static size_t json_escape_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '\\') return 0;
    uint32_t c = get_char_at(encoding, start, pos + 1);
    if (c == '\\' || c == '/' || c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't') return 2;
    if (c == 'u' && pos + 6 <= max_len &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 2)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 3)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 4)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 5))) {
        return 6;
    }
    return 0;
}

bool _gen_json_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = json_escape_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = json_escape_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Bool: true|false
static size_t json_bool_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (str_match_at(encoding, start, pos, max_len, "true", caseless)) return 4;
    if (str_match_at(encoding, start, pos, max_len, "false", caseless)) return 5;
    return 0;
}

bool _gen_json_Bool_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = json_bool_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = json_bool_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Null: null
static size_t json_null_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (str_match_at(encoding, start, pos, max_len, "null", caseless)) return 4;
    return 0;
}

bool _gen_json_Null_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = json_null_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = json_null_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// ValueSeparator: ,
bool _gen_json_ValueSeparator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == ',') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == ',') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

// KeyValueSeparator: :
bool _gen_json_KeyValueSeparator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == ':') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == ':') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

/* ========================================================================= */
/* === HTML Matchers Implementation ===                                      */
/* ========================================================================= */

bool _gen_html_Comment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "<!--", false)) {
            if (offset) *offset = 0;
            if (length) *length = 4;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 3 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "<!--", false)) {
            if (offset) *offset = pos;
            if (length) *length = 4;
            return true;
        }
    }
    return false;
}

bool _gen_html_Comment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "-->", false)) {
            if (offset) *offset = 0;
            if (length) *length = 3;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 2 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "-->", false)) {
            if (offset) *offset = pos;
            if (length) *length = 3;
            return true;
        }
    }
    return false;
}

// Doctype: <!doctype\b[^>]*>
static size_t html_doctype_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos + 9 > max_len || !str_match_at(encoding, start, pos, max_len, "<!doctype", true)) return 0;
    if (pos + 9 < max_len) {
        uint32_t after = get_char_at(encoding, start, pos + 9);
        if (is_alnum_codepoint(after) || after == '_') return 0;
    }
    size_t i = pos + 9;
    while (i < max_len && get_char_at(encoding, start, i) != '>') i++;
    if (i < max_len && get_char_at(encoding, start, i) == '>') return (i + 1) - pos;
    return 0;
}

bool _gen_html_Doctype_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = html_doctype_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = html_doctype_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// ClosingTag: <\/[a-zA-Z0-9:-]+\s*>
static size_t html_closing_tag_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos + 2 >= max_len || get_char_at(encoding, start, pos) != '<' || get_char_at(encoding, start, pos + 1) != '/') return 0;
    size_t i = pos + 2;
    size_t tag_chars = 0;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == ':' || c == '-') {
            i++;
            tag_chars++;
        } else break;
    }
    if (tag_chars == 0) return 0;
    while (i < max_len && is_space_codepoint(get_char_at(encoding, start, i))) i++;
    if (i < max_len && get_char_at(encoding, start, i) == '>') return (i + 1) - pos;
    return 0;
}

bool _gen_html_ClosingTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = html_closing_tag_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = html_closing_tag_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Tag: start=<[a-zA-Z0-9:-]+ end=\/?>
static size_t html_tag_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '<') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (c1 == '/' || c1 == '!') return 0;
    size_t i = pos + 1;
    size_t tag_chars = 0;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == ':' || c == '-') {
            i++;
            tag_chars++;
        } else break;
    }
    if (tag_chars == 0) return 0;
    return i - pos;
}

bool _gen_html_Tag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = html_tag_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = html_tag_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_html_Tag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "/>", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        if (max_len >= 1 && get_char_at(encoding, start, 0) == '>') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "/>", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
        if (get_char_at(encoding, start, pos) == '>') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

bool _gen_html_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_json_String_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_html_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_json_String_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_html_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_html_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// StringEscape: \\\\|\\\"|\\\'|\\n|\\r|\\t
static size_t html_escape_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '\\') return 0;
    uint32_t c = get_char_at(encoding, start, pos + 1);
    if (c == '\\' || c == '"' || c == '\'' || c == 'n' || c == 'r' || c == 't') return 2;
    return 0;
}

bool _gen_html_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = html_escape_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = html_escape_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_html_Equal_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '=') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '=') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

// AttributeName: [a-zA-Z0-9:-]+
static size_t html_attr_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    uint32_t c0 = get_char_at(encoding, start, pos);
    if (!is_alnum_codepoint(c0) && c0 != ':' && c0 != '-') return 0;
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == ':' || c == '-') i++;
        else break;
    }
    return i - pos;
}

bool _gen_html_AttributeName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = html_attr_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = html_attr_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

/* ========================================================================= */
/* === CSS Matchers Implementation ===                                       */
/* ========================================================================= */

bool _gen_css_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_css_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// AtRule: @[a-zA-Z-]+
static size_t css_atrule_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '@') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (!is_alpha_codepoint(c1) && c1 != '-') return 0;
    size_t i = pos + 2;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alpha_codepoint(c) || c == '-') i++;
        else break;
    }
    return i - pos;
}

bool _gen_css_AtRule_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = css_atrule_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = css_atrule_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// ClassName: \.[a-zA-Z_-][a-zA-Z0-9_-]*
static size_t css_classname_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '.') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (!is_alpha_codepoint(c1) && c1 != '_' && c1 != '-') return 0;
    size_t i = pos + 2;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_' || c == '-') i++;
        else break;
    }
    return i - pos;
}

bool _gen_css_ClassName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = css_classname_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = css_classname_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// IdName: #[a-zA-Z_-][a-zA-Z0-9_-]*
static size_t css_idname_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '#') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (!is_alpha_codepoint(c1) && c1 != '_' && c1 != '-') return 0;
    size_t i = pos + 2;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_' || c == '-') i++;
        else break;
    }
    return i - pos;
}

bool _gen_css_IdName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = css_idname_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = css_idname_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// PseudoClass: :{1,2}[a-zA-Z-]+
static size_t css_pseudo_class_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != ':') return 0;
    size_t i = pos + 1;
    if (i < max_len && get_char_at(encoding, start, i) == ':') i++;
    size_t chars = 0;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alpha_codepoint(c) || c == '-') {
            i++;
            chars++;
        } else break;
    }
    if (chars == 0) return 0;
    return i - pos;
}

bool _gen_css_PseudoClass_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = css_pseudo_class_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = css_pseudo_class_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// TagName: [a-zA-Z-][a-zA-Z0-9-]*
static size_t css_tagname_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    uint32_t c0 = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c0) && c0 != '-') return 0;
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '-') i++;
        else break;
    }
    return i - pos;
}

bool _gen_css_TagName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = css_tagname_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = css_tagname_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Operator: [\,>+~*]
bool _gen_css_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0) {
            uint32_t c = get_char_at(encoding, start, 0);
            if (c == ',' || c == '>' || c == '+' || c == '~' || c == '*') {
                if (offset) *offset = 0;
                if (length) *length = 1;
                return true;
            }
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        uint32_t c = get_char_at(encoding, start, pos);
        if (c == ',' || c == '>' || c == '+' || c == '~' || c == '*') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

bool _gen_css_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_css_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Declaration: start=[a-zA-Z-][a-zA-Z0-9-]*\s*: end=;
static size_t css_declaration_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos >= max_len) return 0;
    uint32_t c0 = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c0) && c0 != '-') return 0;
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '-') i++;
        else break;
    }
    while (i < max_len && is_space_codepoint(get_char_at(encoding, start, i))) i++;
    if (i < max_len && get_char_at(encoding, start, i) == ':') return (i + 1) - pos;
    return 0;
}

bool _gen_css_Declaration_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = css_declaration_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = css_declaration_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_css_Declaration_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == ';') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == ';') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

// HexColor: #[0-9a-fA-F]{3,8}
static size_t css_hexcolor_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 3 >= max_len || get_char_at(encoding, start, pos) != '#') return 0;
    size_t i = pos + 1;
    while (i < max_len && (i - pos) <= 8 && is_xdigit_codepoint(get_char_at(encoding, start, i))) i++;
    if ((i - pos - 1) >= 3) return i - pos;
    return 0;
}

bool _gen_css_HexColor_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = css_hexcolor_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = css_hexcolor_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Number: [0-9]*\.?[0-9]+(?:[a-zA-Z%]+)?
static size_t css_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    size_t i = pos;
    bool has_digits = false;
    while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) {
        i++;
        has_digits = true;
    }
    if (i < max_len && get_char_at(encoding, start, i) == '.') {
        if (i + 1 < max_len && is_digit_codepoint(get_char_at(encoding, start, i + 1))) {
            i += 2;
            while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
            has_digits = true;
        } else if (!has_digits) {
            return 0;
        }
    }
    if (!has_digits) return 0;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alpha_codepoint(c) || c == '%') i++;
        else break;
    }
    return i - pos;
}

bool _gen_css_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = css_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = css_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// FunctionCall: url\([^)]*\)|rgb\([^)]*\)|rgba\([^)]*\)|hsl\([^)]*\)|hsla\([^)]*\)|var\([^)]*\)|calc\([^)]*\)
static size_t css_func_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    static const char *const prefixes[] = { "url(", "rgba(", "rgb(", "hsla(", "hsl(", "var(", "calc(", NULL };
    for (int k = 0; prefixes[k] != NULL; k++) {
        size_t plen = strlen(prefixes[k]);
        if (str_match_at(encoding, start, pos, max_len, prefixes[k], true)) {
            size_t i = pos + plen;
            while (i < max_len && get_char_at(encoding, start, i) != ')') i++;
            if (i < max_len && get_char_at(encoding, start, i) == ')') return (i + 1) - pos;
        }
    }
    return 0;
}

bool _gen_css_FunctionCall_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = css_func_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = css_func_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_css_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_css_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_css_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_css_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_css_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_html_StringEscape_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Important: !important\b
static size_t css_important_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (str_match_at(encoding, start, pos, max_len, "!important", true)) {
        if (pos + 10 == max_len) return 10;
        uint32_t after = get_char_at(encoding, start, pos + 10);
        if (!is_alnum_codepoint(after) && after != '_') return 10;
    }
    return 0;
}

bool _gen_css_Important_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = css_important_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = css_important_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_css_Value_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_css_TagName_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// DeclOperator: [,/!]
bool _gen_css_DeclOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0) {
            uint32_t c = get_char_at(encoding, start, 0);
            if (c == ',' || c == '/' || c == '!') {
                if (offset) *offset = 0;
                if (length) *length = 1;
                return true;
            }
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        uint32_t c = get_char_at(encoding, start, pos);
        if (c == ',' || c == '/' || c == '!') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

bool _gen_css_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_css_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_css_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_css_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === CFML Matchers Implementation ===                                      */
/* ========================================================================= */

// Helper for cf tags: <cfscript(?=[\>\s]), <cfoutput(?=[\>\s]), etc.
static size_t cf_tag_prefix_match(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, const char *prefix, bool caseless)
{
    size_t plen = strlen(prefix);
    if (!str_match_at(encoding, start, pos, max_len, prefix, caseless)) return 0;
    if (pos + plen < max_len) {
        uint32_t c = get_char_at(encoding, start, pos + plen);
        if (c == '>' || is_space_codepoint(c) || c == '/') {
            return plen;
        }
        return 0;
    }
    return 0;
}

bool _gen_cfml_ScriptStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cf_tag_prefix_match(encoding, start, 0, max_len, "<cfscript", is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cf_tag_prefix_match(encoding, start, pos, max_len, "<cfscript", is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_ScriptStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_html_Tag_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_cfml_ScriptEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "</cfscript>", is_caseless)) {
            if (offset) *offset = 0;
            if (length) *length = 11;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "</cfscript>", is_caseless)) {
            if (offset) *offset = pos;
            if (length) *length = 11;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_OutputStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cf_tag_prefix_match(encoding, start, 0, max_len, "<cfoutput", is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cf_tag_prefix_match(encoding, start, pos, max_len, "<cfoutput", is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_OutputStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_html_Tag_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_cfml_OutputEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "</cfoutput>", is_caseless)) {
            if (offset) *offset = 0;
            if (length) *length = 11;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "</cfoutput>", is_caseless)) {
            if (offset) *offset = pos;
            if (length) *length = 11;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_QueryStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cf_tag_prefix_match(encoding, start, 0, max_len, "<cfquery", is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cf_tag_prefix_match(encoding, start, pos, max_len, "<cfquery", is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_QueryStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_html_Tag_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_cfml_QueryEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "</cfquery>", is_caseless)) {
            if (offset) *offset = 0;
            if (length) *length = 10;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "</cfquery>", is_caseless)) {
            if (offset) *offset = pos;
            if (length) *length = 10;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_LoopStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cf_tag_prefix_match(encoding, start, 0, max_len, "<cfloop", is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cf_tag_prefix_match(encoding, start, pos, max_len, "<cfloop", is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_LoopStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_html_Tag_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_cfml_LoopEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "</cfloop>", is_caseless)) {
            if (offset) *offset = 0;
            if (length) *length = 9;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "</cfloop>", is_caseless)) {
            if (offset) *offset = pos;
            if (length) *length = 9;
            return true;
        }
    }
    return false;
}

// StartTag: <cf[a-z0-9_]+
static size_t cfml_start_tag_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless)
{
    if (!str_match_at(encoding, start, pos, max_len, "<cf", caseless)) return 0;
    size_t i = pos + 3;
    size_t tag_chars = 0;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_') {
            i++;
            tag_chars++;
        } else break;
    }
    if (tag_chars == 0) return 0;
    return i - pos;
}

bool _gen_cfml_StartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cfml_start_tag_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_start_tag_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_StartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_html_Tag_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// EndTag: <\/cf(?!output)(?!script)(?!query)(?!loop)[a-z0-9_]+
static size_t cfml_end_tag_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless)
{
    if (!str_match_at(encoding, start, pos, max_len, "</cf", caseless)) return 0;
    size_t i = pos + 4;
    // Check negative lookaheads: output, script, query, loop
    if (str_match_at(encoding, start, i, max_len, "output", caseless)) {
        size_t next_pos = i + 6;
        if (next_pos == max_len || !is_alnum_codepoint(get_char_at(encoding, start, next_pos))) return 0;
    }
    if (str_match_at(encoding, start, i, max_len, "script", caseless)) {
        size_t next_pos = i + 6;
        if (next_pos == max_len || !is_alnum_codepoint(get_char_at(encoding, start, next_pos))) return 0;
    }
    if (str_match_at(encoding, start, i, max_len, "query", caseless)) {
        size_t next_pos = i + 5;
        if (next_pos == max_len || !is_alnum_codepoint(get_char_at(encoding, start, next_pos))) return 0;
    }
    if (str_match_at(encoding, start, i, max_len, "loop", caseless)) {
        size_t next_pos = i + 4;
        if (next_pos == max_len || !is_alnum_codepoint(get_char_at(encoding, start, next_pos))) return 0;
    }
    size_t tag_chars = 0;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_') {
            i++;
            tag_chars++;
        } else break;
    }
    if (tag_chars == 0) return 0;
    return i - pos;
}

bool _gen_cfml_EndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cfml_end_tag_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_end_tag_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_EndTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_html_Tag_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Comment: <!--- and --->
bool _gen_cfml_Comment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "<!---", false)) {
            if (offset) *offset = 0;
            if (length) *length = 5;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "<!---", false)) {
            if (offset) *offset = pos;
            if (length) *length = 5;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_Comment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "--->", false)) {
            if (offset) *offset = 0;
            if (length) *length = 4;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "--->", false)) {
            if (offset) *offset = pos;
            if (length) *length = 4;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cfml_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cfml_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cfml_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// SingleChar: ''
bool _gen_cfml_SingleChar_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "''", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "''", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

// DoubleChar: ""
bool _gen_cfml_DoubleChar_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "\"\"", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "\"\"", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

// SharpChar: ##
bool _gen_cfml_SharpChar_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "##", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "##", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

// SharpExpression: #
bool _gen_cfml_SharpExpression_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '#') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '#') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}
bool _gen_cfml_SharpExpression_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_cfml_SharpExpression_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_cfml_ScriptBlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cfml_ScriptBlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// ScriptLineComment: \/\/.*[^\r\n]
bool _gen_cfml_ScriptLineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// ExpressionEnd: ;
bool _gen_cfml_ExpressionEnd_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_css_Declaration_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Number: ([0-9]+\.?[0-9]*(?:[eE][-+]?[0-9]+)?|[0-9]*\.[0-9]+(?:[eE][-+]?[0-9]+)?)
bool _gen_cfml_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Number_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Boolean: true\b|false\b|yes\b|no\b
static size_t cfml_bool_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless)
{
    static const char *const bool_kws[] = { "true", "false", "yes", "no", NULL };
    for (int k = 0; bool_kws[k] != NULL; k++) {
        size_t blen = strlen(bool_kws[k]);
        if (str_match_at(encoding, start, pos, max_len, bool_kws[k], caseless)) {
            if (pos + blen == max_len) return blen;
            uint32_t after = get_char_at(encoding, start, pos + blen);
            if (!is_alnum_codepoint(after) && after != '_') return blen;
        }
    }
    return 0;
}

bool _gen_cfml_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cfml_bool_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_bool_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// ObjectMember: \.
bool _gen_cfml_ObjectMember_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '.') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '.') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

// Function: ([a-z_]+[a-z0-9_]*)[\s]*\( -> group 1 is function name
static size_t cfml_function_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos >= max_len) return 0;
    uint32_t c0 = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c0) && c0 != '_') return 0;
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_') i++;
        else break;
    }
    size_t func_name_len = i - pos;
    while (i < max_len && is_space_codepoint(get_char_at(encoding, start, i))) i++;
    if (i < max_len && get_char_at(encoding, start, i) == '(') {
        return func_name_len; // PCRE2 regex captures group 1 as the token length
    }
    return 0;
}

bool _gen_cfml_Function_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = cfml_function_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_function_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Separator: ,
bool _gen_cfml_Separator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_json_ValueSeparator_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Variable: [a-z_\$]+[a-z0-9_\$]*
static size_t cfml_var_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos >= max_len) return 0;
    uint32_t c0 = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c0) && c0 != '_' && c0 != '$') return 0;
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_' || c == '$') i++;
        else break;
    }
    return i - pos;
}

bool _gen_cfml_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = cfml_var_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_var_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// AssignOperator: \+=|\-=|\*=|\/=|%=|&=|=
static size_t cfml_assign_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos >= max_len) return 0;
    if (pos + 2 <= max_len) {
        static const char *const aops2[] = { "+=", "-=", "*=", "/=", "%=", "&=", NULL };
        for (int k = 0; aops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, aops2[k], false)) return 2;
        }
    }
    if (get_char_at(encoding, start, pos) == '=') return 1;
    return 0;
}

bool _gen_cfml_AssignOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = cfml_assign_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_assign_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// TernaryOperator: \?:|\?|\:
static size_t cfml_ternary_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos >= max_len) return 0;
    if (str_match_at(encoding, start, pos, max_len, "?:", false)) return 2;
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '?' || c == ':') return 1;
    return 0;
}

bool _gen_cfml_TernaryOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = cfml_ternary_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_ternary_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Word operators: imp\b, eqv\b, xor\b
static size_t cfml_word_op_match(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, const char *word, bool caseless)
{
    size_t wlen = strlen(word);
    if (str_match_at(encoding, start, pos, max_len, word, caseless)) {
        if (pos + wlen == max_len) return wlen;
        uint32_t after = get_char_at(encoding, start, pos + wlen);
        if (!is_alnum_codepoint(after) && after != '_') return wlen;
    }
    return 0;
}

bool _gen_cfml_LogicalImpOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cfml_word_op_match(encoding, start, 0, max_len, "imp", is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_word_op_match(encoding, start, pos, max_len, "imp", is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_LogicalEqvOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cfml_word_op_match(encoding, start, 0, max_len, "eqv", is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_word_op_match(encoding, start, pos, max_len, "eqv", is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_LogicalXorOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cfml_word_op_match(encoding, start, 0, max_len, "xor", is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_word_op_match(encoding, start, pos, max_len, "xor", is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// LogicalOr: \|||or\b
static size_t cfml_or_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless)
{
    if (str_match_at(encoding, start, pos, max_len, "||", false)) return 2;
    return cfml_word_op_match(encoding, start, pos, max_len, "or", caseless);
}

bool _gen_cfml_LogicalOrOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cfml_or_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_or_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// LogicalAnd: \&\&|and\b
static size_t cfml_and_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless)
{
    if (str_match_at(encoding, start, pos, max_len, "&&", false)) return 2;
    return cfml_word_op_match(encoding, start, pos, max_len, "and", caseless);
}

bool _gen_cfml_LogicalAndOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cfml_and_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_and_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// LogicalNot: not\b|!
static size_t cfml_not_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless)
{
    size_t wm = cfml_word_op_match(encoding, start, pos, max_len, "not", caseless);
    if (wm > 0) return wm;
    if (pos < max_len && get_char_at(encoding, start, pos) == '!') return 1;
    return 0;
}

bool _gen_cfml_LogicalNotOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cfml_not_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_not_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t cfml_match_words_sequence(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, const char *const *words, bool caseless)
{
    size_t i = pos;
    for (int k = 0; words[k] != NULL; k++) {
        size_t wlen = strlen(words[k]);
        if (k > 0) {
            size_t sp = 0;
            while (i < max_len && is_space_codepoint(get_char_at(encoding, start, i))) { i++; sp++; }
            if (sp == 0) return 0;
        }
        if (!str_match_at(encoding, start, i, max_len, words[k], caseless)) return 0;
        i += wlen;
    }
    if (i == max_len || (!is_alnum_codepoint(get_char_at(encoding, start, i)) && get_char_at(encoding, start, i) != '_')) {
        return i - pos;
    }
    return 0;
}

// CompareOperator: greater\s+than\s+or\s+equal\s+to\b|less\s+than\s+or\s+equal\s+to\b|does\s+not\s+contain\b|is\s+not\b|contains\b|less\s+than\b|greater\s+than\b|not\s+equal\b|equal\b|neq\b|lte\b|gte\b|eq\b|==|>=|<=|!=|ge\b|lt\b|gt\b|le\b|\bis\b|>|<
static size_t cfml_compare_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless)
{
    if (pos >= max_len) return 0;

    static const char *const seq_gte[] = { "greater", "than", "or", "equal", "to", NULL };
    static const char *const seq_lte[] = { "less", "than", "or", "equal", "to", NULL };
    static const char *const seq_dnc[] = { "does", "not", "contain", NULL };
    static const char *const seq_isnot[] = { "is", "not", NULL };
    static const char *const seq_lt[] = { "less", "than", NULL };
    static const char *const seq_gt[] = { "greater", "than", NULL };
    static const char *const seq_neq[] = { "not", "equal", NULL };

    size_t m = 0;
    if ((m = cfml_match_words_sequence(encoding, start, pos, max_len, seq_gte, caseless)) > 0) return m;
    if ((m = cfml_match_words_sequence(encoding, start, pos, max_len, seq_lte, caseless)) > 0) return m;
    if ((m = cfml_match_words_sequence(encoding, start, pos, max_len, seq_dnc, caseless)) > 0) return m;
    if ((m = cfml_match_words_sequence(encoding, start, pos, max_len, seq_isnot, caseless)) > 0) return m;
    if ((m = cfml_match_words_sequence(encoding, start, pos, max_len, seq_lt, caseless)) > 0) return m;
    if ((m = cfml_match_words_sequence(encoding, start, pos, max_len, seq_gt, caseless)) > 0) return m;
    if ((m = cfml_match_words_sequence(encoding, start, pos, max_len, seq_neq, caseless)) > 0) return m;

    static const char *const cmp_words[] = { "contains", "equal", "neq", "lte", "gte", "eq", "ge", "lt", "gt", "le", "is", NULL };
    for (int k = 0; cmp_words[k] != NULL; k++) {
        size_t wm = cfml_word_op_match(encoding, start, pos, max_len, cmp_words[k], caseless);
        if (wm > 0) return wm;
    }

    if (pos + 2 <= max_len) {
        if (str_match_at(encoding, start, pos, max_len, "==", false) ||
            str_match_at(encoding, start, pos, max_len, ">=", false) ||
            str_match_at(encoding, start, pos, max_len, "<=", false) ||
            str_match_at(encoding, start, pos, max_len, "!=", false)) {
            return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '>' || c == '<') return 1;
    return 0;
}

bool _gen_cfml_CompareOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cfml_compare_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_compare_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Concat: \&
bool _gen_cfml_ConcatOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '&') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '&') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

// Add: \+|-
bool _gen_cfml_AddOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0) {
            uint32_t c = get_char_at(encoding, start, 0);
            if (c == '+' || c == '-') {
                if (offset) *offset = 0;
                if (length) *length = 1;
                return true;
            }
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        uint32_t c = get_char_at(encoding, start, pos);
        if (c == '+' || c == '-') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

// Mul: \*|\/|\\|mod\b|\%
static size_t cfml_mul_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless)
{
    if (pos >= max_len) return 0;
    size_t wm = cfml_word_op_match(encoding, start, pos, max_len, "mod", caseless);
    if (wm > 0) return wm;
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '*' || c == '/' || c == '\\' || c == '%') return 1;
    return 0;
}

bool _gen_cfml_MulOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cfml_mul_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_mul_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Power: \^
bool _gen_cfml_PowerOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '^') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '^') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cfml_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Keyword: var\b|function\b|this\b|try\b|catch\b|if\b|then\b|else\b
static size_t cfml_kw_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless)
{
    static const char *const kws[] = { "function", "catch", "this", "then", "else", "var", "try", "if", NULL };
    for (int k = 0; kws[k] != NULL; k++) {
        size_t wm = cfml_word_op_match(encoding, start, pos, max_len, kws[k], caseless);
        if (wm > 0) return wm;
    }
    return 0;
}

bool _gen_cfml_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cfml_kw_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cfml_kw_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_cfml_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cfml_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cfml_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cfml_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === CPP Matchers Implementation ===                                       */
/* ========================================================================= */

bool _gen_cpp_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_Preprocessor_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Preprocessor_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// C++ Keywords list
static const char *const cpp_keywords[] = {
    "class", "namespace", "template", "typename", "using", "public", "private", "protected",
    "virtual", "override", "final", "friend", "operator", "this", "new", "delete", "throw",
    "catch", "try", "constexpr", "consteval", "constinit", "decltype", "explicit", "export",
    "import", "module", "mutable", "noexcept", "nullptr", "static_cast", "dynamic_cast",
    "const_cast", "reinterpret_cast", "thread_local", "concept", "requires", "co_await",
    "co_return", "co_yield", "char8_t", "char16_t", "char32_t", "wchar_t", "int", "char",
    "double", "float", "void", "short", "long", "unsigned", "signed", "struct", "union",
    "enum", "typedef", "const", "static", "extern", "volatile", "if", "else", "for", "while",
    "do", "switch", "case", "default", "break", "continue", "return", "sizeof", "goto",
    "register", "auto", "inline", "restrict", "_Bool", "bool", "size_t", "ssize_t", "uint8_t",
    "uint16_t", "uint32_t", "uint64_t", "int8_t", "int16_t", "int32_t", "int64_t", "uintptr_t",
    "intptr_t", "ptrdiff_t", NULL
};

static size_t cpp_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; cpp_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(cpp_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, cpp_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_cpp_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cpp_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cpp_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_cpp_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// ScopeResolution: ::
bool _gen_cpp_ScopeResolution_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "::", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "::", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

// Operator: \+=|\-=|\*=|\/=|%=|&=|\^=|\|=|<{2}=|>{2}=|<{2}|>{2}|\+\+|\-\-|&&|\|\||<=>|<=|>=|==|!=|->\*|->|\.\*|\.|[=!|^~+\-/%?:;.]
static size_t cpp_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        if (str_match_at(encoding, start, pos, max_len, "<<=", false) ||
            str_match_at(encoding, start, pos, max_len, ">>=", false) ||
            str_match_at(encoding, start, pos, max_len, "<=>", false) ||
            str_match_at(encoding, start, pos, max_len, "->*", false)) {
            return 3;
        }
    }
    if (pos + 2 <= max_len) {
        static const char *const cpp_ops2[] = {
            "+=", "-=", "*=", "/=", "%=", "&=", "^=", "|=", "<<", ">>",
            "++", "--", "&&", "||", "<=", ">=", "==", "!=", "->", ".*", NULL
        };
        for (int k = 0; cpp_ops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, cpp_ops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '!' || c == '|' || c == '^' || c == '~' || c == '+' ||
        c == '-' || c == '/' || c == '%' || c == '?' || c == ':' || c == ';' || c == '.') {
        return 1;
    }
    return 0;
}

bool _gen_cpp_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = cpp_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cpp_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// PointerOrRef: [*&]
bool _gen_cpp_PointerOrRef_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0) {
            uint32_t c = get_char_at(encoding, start, 0);
            if (c == '*' || c == '&') {
                if (offset) *offset = 0;
                if (length) *length = 1;
                return true;
            }
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        uint32_t c = get_char_at(encoding, start, pos);
        if (c == '*' || c == '&') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

// TemplateOpen: <
bool _gen_cpp_TemplateOpen_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '<') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '<') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

// TemplateClose: >
bool _gen_cpp_TemplateClose_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '>') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '>') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

// Comma: ,
bool _gen_cpp_Comma_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_json_ValueSeparator_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_cpp_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_StringEscape_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Number: (?:0[xX][0-9a-fA-F]+[uUlL]*|0[bB][01]+[uUlL]*|[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?[fFdDlL]?)
static size_t cpp_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    // 0x / 0X hex
    if (pos + 2 < max_len && get_char_at(encoding, start, pos) == '0') {
        uint32_t c1 = get_char_at(encoding, start, pos + 1);
        if (c1 == 'x' || c1 == 'X') {
            size_t i = pos + 2;
            size_t hex_digits = 0;
            while (i < max_len && is_xdigit_codepoint(get_char_at(encoding, start, i))) { i++; hex_digits++; }
            if (hex_digits > 0) {
                while (i < max_len) {
                    uint32_t suf = get_char_at(encoding, start, i);
                    if (suf == 'u' || suf == 'U' || suf == 'l' || suf == 'L') i++;
                    else break;
                }
                return i - pos;
            }
        }
        if (c1 == 'b' || c1 == 'B') {
            size_t i = pos + 2;
            size_t bin_digits = 0;
            while (i < max_len) {
                uint32_t b = get_char_at(encoding, start, i);
                if (b == '0' || b == '1') { i++; bin_digits++; }
                else break;
            }
            if (bin_digits > 0) {
                while (i < max_len) {
                    uint32_t suf = get_char_at(encoding, start, i);
                    if (suf == 'u' || suf == 'U' || suf == 'l' || suf == 'L') i++;
                    else break;
                }
                return i - pos;
            }
        }
    }
    return c_number_match_at(encoding, start, pos, max_len);
}

bool _gen_cpp_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = cpp_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cpp_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_cpp_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_TemplateGroup_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_cpp_TemplateOpen_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_TemplateGroup_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_cpp_TemplateClose_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_TypeCast_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_cpp_TypeCast_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === Python Matchers Implementation ===                                    */
/* ========================================================================= */

// LineComment: #[^\r\n]*
bool _gen_python_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '#') {
            size_t i = 1;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (c == '\r' || c == '\n') break;
                i++;
            }
            if (offset) *offset = 0;
            if (length) *length = i;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '#') {
            size_t i = pos + 1;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (c == '\r' || c == '\n') break;
                i++;
            }
            if (offset) *offset = pos;
            if (length) *length = i - pos;
            return true;
        }
    }
    return false;
}

// TripleSingleString: '''
bool _gen_python_TripleSingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "'''", false)) {
            if (offset) *offset = 0;
            if (length) *length = 3;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 2 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "'''", false)) {
            if (offset) *offset = pos;
            if (length) *length = 3;
            return true;
        }
    }
    return false;
}
bool _gen_python_TripleSingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_python_TripleSingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// TripleDoubleString: """
bool _gen_python_TripleDoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "\"\"\"", false)) {
            if (offset) *offset = 0;
            if (length) *length = 3;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 2 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "\"\"\"", false)) {
            if (offset) *offset = pos;
            if (length) *length = 3;
            return true;
        }
    }
    return false;
}
bool _gen_python_TripleDoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_python_TripleDoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_python_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_python_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_python_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_python_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// FString: [fF](?:"""|'''|"|')
static size_t python_fstring_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos + 1 >= max_len) return 0;
    uint32_t c0 = get_char_at(encoding, start, pos);
    if (c0 != 'f' && c0 != 'F') return 0;
    if (str_match_at(encoding, start, pos + 1, max_len, "\"\"\"", false)) return 4;
    if (str_match_at(encoding, start, pos + 1, max_len, "'''", false)) return 4;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (c1 == '"' || c1 == '\'') return 2;
    return 0;
}

bool _gen_python_FString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = python_fstring_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = python_fstring_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// FString end: (?:"""|'''|"|')
static size_t python_fstring_end_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos >= max_len) return 0;
    if (str_match_at(encoding, start, pos, max_len, "\"\"\"", false)) return 3;
    if (str_match_at(encoding, start, pos, max_len, "'''", false)) return 3;
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '"' || c == '\'') return 1;
    return 0;
}

bool _gen_python_FString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = python_fstring_end_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = python_fstring_end_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_python_FStringInterpolation_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_python_FStringInterpolation_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// StringEscape: \\[\"'nrt0\\]|\\x[0-9a-fA-F]{2}|\\u[0-9a-fA-F]{4}|\\U[0-9a-fA-F]{8}|\\N\{[a-zA-Z_][a-zA-Z0-9_]*\}
static size_t python_escape_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len)
{
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '\\') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (c1 == '\\' || c1 == '"' || c1 == '\'' || c1 == 'n' || c1 == 'r' || c1 == 't' || c1 == '0') return 2;
    if (c1 == 'x' && pos + 4 <= max_len &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 2)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 3))) return 4;
    if (c1 == 'u' && pos + 6 <= max_len &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 2)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 3)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 4)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 5))) return 6;
    if (c1 == 'U' && pos + 10 <= max_len) {
        bool all_hex = true;
        for (size_t k = 2; k < 10; k++) {
            if (!is_xdigit_codepoint(get_char_at(encoding, start, pos + k))) { all_hex = false; break; }
        }
        if (all_hex) return 10;
    }
    if (c1 == 'N' && pos + 3 < max_len && get_char_at(encoding, start, pos + 2) == '{') {
        size_t i = pos + 3;
        uint32_t c_first = get_char_at(encoding, start, i);
        if (is_alpha_codepoint(c_first) || c_first == '_') {
            i++;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (is_alnum_codepoint(c) || c == '_') i++;
                else break;
            }
            if (i < max_len && get_char_at(encoding, start, i) == '}') return (i + 1) - pos;
        }
    }
    return 0;
}

bool _gen_python_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = python_escape_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = python_escape_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Keywords list for Python
static const char *const python_keywords[] = {
    "None", "continue", "nonlocal", "assert", "finally", "import", "return", "async",
    "await", "break", "class", "elif", "else", "except", "from", "global", "lambda",
    "pass", "raise", "while", "with", "yield", "def", "del", "for", "try", "and", "as",
    "if", "in", "is", "not", "or", NULL
};

static size_t python_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; python_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(python_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, python_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_python_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = python_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = python_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Boolean: True\b|False\b
static size_t python_bool_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (str_match_at(encoding, start, pos, max_len, "True", false)) {
        if (pos + 4 == max_len) return 4;
        uint32_t after = get_char_at(encoding, start, pos + 4);
        if (!is_alnum_codepoint(after) && after != '_') return 4;
    }
    if (str_match_at(encoding, start, pos, max_len, "False", false)) {
        if (pos + 5 == max_len) return 5;
        uint32_t after = get_char_at(encoding, start, pos + 5);
        if (!is_alnum_codepoint(after) && after != '_') return 5;
    }
    return 0;
}

bool _gen_python_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = python_bool_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = python_bool_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Number: 0[xX][0-9a-fA-F][0-9a-fA-F_]*|0[oO][0-7][0-7_]*|0[bB][01][01_]*|[0-9][0-9_]*(?:\.[0-9][0-9_]*)?(?:[eE][-+]?[0-9][0-9_]*)?[jJ]?
static size_t python_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 < max_len && get_char_at(encoding, start, pos) == '0') {
        uint32_t c1 = get_char_at(encoding, start, pos + 1);
        if (c1 == 'x' || c1 == 'X') {
            if (is_xdigit_codepoint(get_char_at(encoding, start, pos + 2))) {
                size_t i = pos + 3;
                while (i < max_len) {
                    uint32_t c = get_char_at(encoding, start, i);
                    if (is_xdigit_codepoint(c) || c == '_') i++;
                    else break;
                }
                return i - pos;
            }
        }
        if (c1 == 'o' || c1 == 'O') {
            uint32_t c2 = get_char_at(encoding, start, pos + 2);
            if (c2 >= '0' && c2 <= '7') {
                size_t i = pos + 3;
                while (i < max_len) {
                    uint32_t c = get_char_at(encoding, start, i);
                    if ((c >= '0' && c <= '7') || c == '_') i++;
                    else break;
                }
                return i - pos;
            }
        }
        if (c1 == 'b' || c1 == 'B') {
            uint32_t c2 = get_char_at(encoding, start, pos + 2);
            if (c2 == '0' || c2 == '1') {
                size_t i = pos + 3;
                while (i < max_len) {
                    uint32_t c = get_char_at(encoding, start, i);
                    if (c == '0' || c == '1' || c == '_') i++;
                    else break;
                }
                return i - pos;
            }
        }
    }
    if (is_digit_codepoint(get_char_at(encoding, start, pos))) {
        size_t i = pos + 1;
        while (i < max_len) {
            uint32_t c = get_char_at(encoding, start, i);
            if (is_digit_codepoint(c) || c == '_') i++;
            else break;
        }
        if (i < max_len && get_char_at(encoding, start, i) == '.') {
            if (i + 1 < max_len && is_digit_codepoint(get_char_at(encoding, start, i + 1))) {
                i += 2;
                while (i < max_len) {
                    uint32_t c = get_char_at(encoding, start, i);
                    if (is_digit_codepoint(c) || c == '_') i++;
                    else break;
                }
            }
        }
        if (i < max_len) {
            uint32_t c = get_char_at(encoding, start, i);
            if (c == 'e' || c == 'E') {
                size_t e_pos = i;
                i++;
                if (i < max_len) {
                    uint32_t sign = get_char_at(encoding, start, i);
                    if (sign == '+' || sign == '-') i++;
                }
                size_t exp_digits = 0;
                while (i < max_len) {
                    uint32_t ec = get_char_at(encoding, start, i);
                    if (is_digit_codepoint(ec) || ec == '_') { i++; exp_digits++; }
                    else break;
                }
                if (exp_digits == 0) i = e_pos;
            }
        }
        if (i < max_len) {
            uint32_t c = get_char_at(encoding, start, i);
            if (c == 'j' || c == 'J') i++;
        }
        return i - pos;
    }
    return 0;
}

bool _gen_python_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = python_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = python_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Variable: @?[a-zA-Z_][a-zA-Z0-9_]*
static size_t python_var_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    size_t i = pos;
    if (get_char_at(encoding, start, i) == '@') i++;
    if (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alpha_codepoint(c) || c == '_') {
            i++;
            while (i < max_len) {
                uint32_t c2 = get_char_at(encoding, start, i);
                if (is_alnum_codepoint(c2) || c2 == '_') i++;
                else break;
            }
            return i - pos;
        }
    }
    return 0;
}

bool _gen_python_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = python_var_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = python_var_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Operator: :=|\.\.\.|->|//|\*\*|<<|>>|==|!=|<=|>=|\+=|\-=|\*=|\/=|//=|%=|@=|&=|\|=|\^=|<<=|>>=|\*\*=|(?=[=<>!&|^~+\-*/%@.,;:])
static size_t python_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        static const char *const ops3[] = { "...", "//=", "<<=", ">>=", "**=", NULL };
        for (int k = 0; ops3[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, ops3[k], false)) return 3;
        }
    }
    if (pos + 2 <= max_len) {
        static const char *const ops2[] = {
            ":=", "->", "//", "**", "<<", ">>", "==", "!=", "<=", ">=",
            "+=", "-=", "*=", "/=", "%=", "@=", "&=", "|=", "^=", NULL
        };
        for (int k = 0; ops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, ops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '&' || c == '|' ||
        c == '^' || c == '~' || c == '+' || c == '-' || c == '*' || c == '/' ||
        c == '%' || c == '@' || c == '.' || c == ',' || c == ';' || c == ':') {
        return 1;
    }
    return 0;
}

bool _gen_python_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = python_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = python_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_python_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_python_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_python_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_python_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_python_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_python_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === JavaScript Matchers Implementation ===                                */
/* ========================================================================= */

bool _gen_javascript_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_javascript_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_javascript_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Keywords list for JavaScript
static const char *const js_keywords[] = {
    "instanceof", "undefined", "debugger", "function", "continue", "default",
    "extends", "finally", "typeof", "delete", "export", "import", "return",
    "switch", "catch", "class", "const", "super", "throw", "while", "yield",
    "async", "await", "break", "false", "null", "this", "void", "with", "case",
    "else", "from", "true", "var", "let", "for", "try", "do", "if", "in", "of",
    NULL
};

static size_t js_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; js_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(js_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, js_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_' && after != '$') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_javascript_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = js_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = js_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_javascript_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Variable: [a-zA-Z_$][a-zA-Z0-9_$]*
static size_t js_var_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_' && c != '$') return 0;
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t c2 = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c2) || c2 == '_' || c2 == '$') i++;
        else break;
    }
    return i - pos;
}

bool _gen_javascript_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = js_var_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = js_var_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_javascript_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_javascript_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Regex: \/(?:[^\/\\\r\n]|\\.)+\/[a-zA-Z]*
static size_t js_regex_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 2 >= max_len || get_char_at(encoding, start, pos) != '/') return 0;
    size_t i = pos + 1;
    size_t pattern_chars = 0;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (c == '\r' || c == '\n') return 0;
        if (c == '\\') {
            i += 2;
            pattern_chars++;
        } else if (c == '/') {
            i++;
            break;
        } else {
            i++;
            pattern_chars++;
        }
    }
    if (pattern_chars == 0 || i > max_len) return 0;
    while (i < max_len && is_alpha_codepoint(get_char_at(encoding, start, i))) i++;
    return i - pos;
}

bool _gen_javascript_Regex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = js_regex_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = js_regex_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Operator: ===|==|!==|!=|=>|\+=|\-=|\*=|\/=|%=|\+\+|\-\-|&&|\|\||<=|>=|\?\.|\?\?|[=<>!&|^~+\-*/%?:;.,]
static size_t js_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        if (str_match_at(encoding, start, pos, max_len, "===", false) ||
            str_match_at(encoding, start, pos, max_len, "!==", false)) {
            return 3;
        }
    }
    if (pos + 2 <= max_len) {
        static const char *const js_ops2[] = {
            "==", "!=", "=>", "+=", "-=", "*=", "/=", "%=", "++", "--",
            "&&", "||", "<=", ">=", "?.", "??", NULL
        };
        for (int k = 0; js_ops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, js_ops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '&' ||
        c == '|' || c == '^' || c == '~' || c == '+' || c == '-' ||
        c == '*' || c == '/' || c == '%' || c == '?' || c == ':' ||
        c == ';' || c == '.' || c == ',') {
        return 1;
    }
    return 0;
}

bool _gen_javascript_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = js_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = js_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_javascript_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_javascript_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_javascript_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_javascript_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// TemplateString: `
bool _gen_javascript_TemplateString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '`') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '`') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}
bool _gen_javascript_TemplateString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_TemplateString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// StringEscape: \\\\|\\\"|\\\'|\\n|\\r|\\t|\\`|\\u[0-9a-fA-F]{4}|\\x[0-9a-fA-F]{2}
static size_t js_escape_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '\\') return 0;
    uint32_t c = get_char_at(encoding, start, pos + 1);
    if (c == '\\' || c == '"' || c == '\'' || c == 'n' || c == 'r' || c == 't' || c == '`') return 2;
    if (c == 'x' && pos + 4 <= max_len &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 2)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 3))) return 4;
    if (c == 'u' && pos + 6 <= max_len &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 2)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 3)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 4)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 5))) return 6;
    return 0;
}

bool _gen_javascript_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = js_escape_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = js_escape_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Number: 0[xX][0-9a-fA-F]+|0[oO][0-7]+|0[bB][01]+|[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?
static size_t js_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 < max_len && get_char_at(encoding, start, pos) == '0') {
        uint32_t c1 = get_char_at(encoding, start, pos + 1);
        if (c1 == 'x' || c1 == 'X') {
            size_t i = pos + 2;
            size_t hex_digits = 0;
            while (i < max_len && is_xdigit_codepoint(get_char_at(encoding, start, i))) { i++; hex_digits++; }
            if (hex_digits > 0) return i - pos;
        }
        if (c1 == 'o' || c1 == 'O') {
            size_t i = pos + 2;
            size_t oct_digits = 0;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (c >= '0' && c <= '7') { i++; oct_digits++; }
                else break;
            }
            if (oct_digits > 0) return i - pos;
        }
        if (c1 == 'b' || c1 == 'B') {
            size_t i = pos + 2;
            size_t bin_digits = 0;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (c == '0' || c == '1') { i++; bin_digits++; }
                else break;
            }
            if (bin_digits > 0) return i - pos;
        }
    }
    return json_number_match_at(encoding, start, pos, max_len);
}

bool _gen_javascript_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = js_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = js_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_javascript_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_javascript_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_javascript_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_javascript_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === Rust Matchers Implementation ===                                      */
/* ========================================================================= */

bool _gen_rust_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_rust_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_rust_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Attribute: #!?\[
static size_t rust_attr_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '#') return 0;
    size_t i = pos + 1;
    if (get_char_at(encoding, start, i) == '!') i++;
    if (i < max_len && get_char_at(encoding, start, i) == '[') return (i + 1) - pos;
    return 0;
}

bool _gen_rust_Attribute_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = rust_attr_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = rust_attr_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_rust_Attribute_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Macro: [a-zA-Z_][a-zA-Z0-9_]*!
static size_t rust_macro_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    uint32_t c0 = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c0) && c0 != '_') return 0;
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_') i++;
        else break;
    }
    if (i < max_len && get_char_at(encoding, start, i) == '!') {
        return (i + 1) - pos;
    }
    return 0;
}

bool _gen_rust_Macro_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = rust_macro_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = rust_macro_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Keywords list for Rust
static const char *const rust_keywords[] = {
    "continue", "extern", "return", "static", "struct", "unsafe", "where",
    "async", "await", "break", "const", "crate", "match", "macro", "isize",
    "usize", "while", "yield", "bool", "char", "enum", "impl", "loop", "move",
    "Self", "self", "super", "trait", "type", "i128", "u128", "else", "dyn",
    "for", "let", "mut", "pub", "ref", "str", "use", "i16", "u16", "i32",
    "u32", "i64", "u64", "f32", "f64", "fn", "if", "in", "mod", "i8", "u8",
    "as", NULL
};

static size_t rust_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; rust_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(rust_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, rust_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_rust_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = rust_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = rust_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Boolean: true\b|false\b|Some\b|None\b|Ok\b|Err\b
static size_t rust_bool_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    static const char *const rbools[] = { "false", "true", "Some", "None", "Err", "Ok", NULL };
    for (int k = 0; rbools[k] != NULL; k++) {
        size_t blen = strlen(rbools[k]);
        if (str_match_at(encoding, start, pos, max_len, rbools[k], false)) {
            if (pos + blen == max_len) return blen;
            uint32_t after = get_char_at(encoding, start, pos + blen);
            if (!is_alnum_codepoint(after) && after != '_') return blen;
        }
    }
    return 0;
}

bool _gen_rust_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = rust_bool_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = rust_bool_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// CharLiteral: '(?:[^\\']|\\.)'
static size_t rust_char_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 2 >= max_len || get_char_at(encoding, start, pos) != '\'') return 0;
    size_t i = pos + 1;
    if (get_char_at(encoding, start, i) == '\\') {
        i += 2;
    } else {
        i++;
    }
    if (i < max_len && get_char_at(encoding, start, i) == '\'') {
        return (i + 1) - pos;
    }
    return 0;
}

bool _gen_rust_CharLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = rust_char_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = rust_char_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Lifetime: '[a-zA-Z_][a-zA-Z0-9_]*\b
static size_t rust_lifetime_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '\'') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (!is_alpha_codepoint(c1) && c1 != '_') return 0;
    size_t i = pos + 2;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_') i++;
        else break;
    }
    if (i < max_len && get_char_at(encoding, start, i) == '\'') return 0; // It's a char literal, not lifetime
    return i - pos;
}

bool _gen_rust_Lifetime_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = rust_lifetime_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = rust_lifetime_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_rust_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_rust_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_rust_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// Operator: ::|:=|:|===|==|!=|<=|>=|&&|\|\||\+=|\-=|\*=|\/=|%=|&=|\^=|\|=|<<=|>>=|->|<<|>>|=>|[=<>!&|^~+\-*/%?:;.,]
static size_t rust_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        static const char *const rops3[] = { "===", "<<=", ">>=", NULL };
        for (int k = 0; rops3[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, rops3[k], false)) return 3;
        }
    }
    if (pos + 2 <= max_len) {
        static const char *const rops2[] = {
            "::", ":=", "==", "!=", "<=", ">=", "&&", "||", "+=", "-=",
            "*=", "/=", "%=", "&=", "^=", "|=", "->", "<<", ">>", "=>", NULL
        };
        for (int k = 0; rops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, rops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '&' ||
        c == '|' || c == '^' || c == '~' || c == '+' || c == '-' ||
        c == '*' || c == '/' || c == '%' || c == '?' || c == ':' ||
        c == ';' || c == '.' || c == ',') {
        return 1;
    }
    return 0;
}

bool _gen_rust_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = rust_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = rust_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_rust_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_rust_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// StringEscape: \\\\|\\\"|\\\'|\\n|\\r|\\t|\\u\{[0-9a-fA-F]{1,6}\}|\\x[0-9a-fA-F]{2}
static size_t rust_escape_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '\\') return 0;
    uint32_t c = get_char_at(encoding, start, pos + 1);
    if (c == '\\' || c == '"' || c == '\'' || c == 'n' || c == 'r' || c == 't') return 2;
    if (c == 'x' && pos + 4 <= max_len &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 2)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 3))) return 4;
    if (c == 'u' && pos + 3 < max_len && get_char_at(encoding, start, pos + 2) == '{') {
        size_t i = pos + 3;
        size_t hex_cnt = 0;
        while (i < max_len && hex_cnt <= 6 && is_xdigit_codepoint(get_char_at(encoding, start, i))) {
            i++;
            hex_cnt++;
        }
        if (hex_cnt >= 1 && hex_cnt <= 6 && i < max_len && get_char_at(encoding, start, i) == '}') {
            return (i + 1) - pos;
        }
    }
    return 0;
}

bool _gen_rust_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = rust_escape_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = rust_escape_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// Number: 0[xX][0-9a-fA-F_]+\b|0[bB][01_]+\b|0[oO][0-7_]+\b|[0-9_]*\.?[0-9_]+(?:[eE][-+]?[0-9_]+)?\b
static size_t rust_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 < max_len && get_char_at(encoding, start, pos) == '0') {
        uint32_t c1 = get_char_at(encoding, start, pos + 1);
        if (c1 == 'x' || c1 == 'X') {
            size_t i = pos + 2;
            size_t hex_cnt = 0;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (is_xdigit_codepoint(c) || c == '_') { i++; hex_cnt++; }
                else break;
            }
            if (hex_cnt > 0) return i - pos;
        }
        if (c1 == 'b' || c1 == 'B') {
            size_t i = pos + 2;
            size_t bin_cnt = 0;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (c == '0' || c == '1' || c == '_') { i++; bin_cnt++; }
                else break;
            }
            if (bin_cnt > 0) return i - pos;
        }
        if (c1 == 'o' || c1 == 'O') {
            size_t i = pos + 2;
            size_t oct_cnt = 0;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if ((c >= '0' && c <= '7') || c == '_') { i++; oct_cnt++; }
                else break;
            }
            if (oct_cnt > 0) return i - pos;
        }
    }
    size_t i = pos;
    bool has_digits = false;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_digit_codepoint(c) || c == '_') { i++; has_digits = true; }
        else break;
    }
    if (i < max_len && get_char_at(encoding, start, i) == '.') {
        if (i + 1 < max_len && (is_digit_codepoint(get_char_at(encoding, start, i + 1)) || get_char_at(encoding, start, i + 1) == '_')) {
            i += 2;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (is_digit_codepoint(c) || c == '_') i++;
                else break;
            }
            has_digits = true;
        } else if (!has_digits) {
            return 0;
        }
    }
    if (!has_digits) return 0;
    if (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (c == 'e' || c == 'E') {
            size_t e_pos = i;
            i++;
            if (i < max_len) {
                uint32_t sign = get_char_at(encoding, start, i);
                if (sign == '+' || sign == '-') i++;
            }
            size_t exp_digits = 0;
            while (i < max_len) {
                uint32_t ec = get_char_at(encoding, start, i);
                if (is_digit_codepoint(ec) || ec == '_') { i++; exp_digits++; }
                else break;
            }
            if (exp_digits == 0) i = e_pos;
        }
    }
    return i - pos;
}

bool _gen_rust_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = rust_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = rust_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_rust_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_rust_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_rust_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_rust_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */

/* ========================================================================= */
/* === TypeScript Matchers Implementation ===                                */
/* ========================================================================= */

bool _gen_typescript_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// TypeScript Keywords
static const char *const ts_keywords[] = {
    "implements", "instanceof", "satisfies", "undefined", "interface", "namespace",
    "debugger", "function", "continue", "abstract", "readonly", "declare",
    "default", "extends", "finally", "private", "protected", "unknown",
    "typeof", "delete", "export", "import", "module", "public", "return",
    "static", "switch", "catch", "class", "const", "infer", "keyof", "never",
    "super", "throw", "while", "yield", "async", "await", "break", "false",
    "null", "this", "type", "void", "with", "case", "else", "enum", "from",
    "true", "var", "let", "any", "for", "try", "do", "if", "in", "is", "of",
    NULL
};

static size_t ts_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; ts_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(ts_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, ts_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_' && after != '$') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_typescript_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = ts_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = ts_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_typescript_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_Regex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_Regex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// TypeScript Operator
static size_t ts_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 4 <= max_len) {
        if (str_match_at(encoding, start, pos, max_len, ">>>=", false)) return 4;
    }
    if (pos + 3 <= max_len) {
        static const char *const ts_ops3[] = {
            "\?\?=", "&&=", "||=", "===", "!==", "**=", "<<=", ">>=", NULL
        };
        for (int k = 0; ts_ops3[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, ts_ops3[k], false)) return 3;
        }
    }
    if (pos + 2 <= max_len) {
        static const char *const ts_ops2[] = {
            "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
            "=>", "==", "!=", "<=", ">=", "&&", "||", "??", "?.", "?:",
            "<<", ">>", "**", "::", NULL
        };
        for (int k = 0; ts_ops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, ts_ops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '&' ||
        c == '|' || c == '^' || c == '~' || c == '+' || c == '-' ||
        c == '*' || c == '/' || c == '%' || c == '?' || c == ':' ||
        c == ';' || c == '.' || c == ',') {
        return 1;
    }
    return 0;
}

bool _gen_typescript_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = ts_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = ts_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_typescript_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_TemplateString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_TemplateString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_TemplateString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_TemplateString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_StringEscape_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_Number_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_typescript_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === Java Matchers Implementation ===                                      */
/* ========================================================================= */

bool _gen_java_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_java_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_java_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t java_annotation_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '@') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (!is_alpha_codepoint(c1) && c1 != '_') return 0;
    size_t i = pos + 2;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_') i++;
        else break;
    }
    return i - pos;
}

bool _gen_java_Annotation_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = java_annotation_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = java_annotation_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static const char *const java_keywords[] = {
    "synchronized", "implements", "instanceof", "transient", "interface",
    "protected", "strictfp", "continue", "abstract", "boolean", "default",
    "extends", "finally", "package", "private", "volatile", "assert",
    "double", "import", "native", "public", "return", "static", "switch",
    "throws", "break", "catch", "class", "final", "float", "short", "super",
    "throw", "while", "byte", "case", "char", "else", "null", "this", "void",
    "for", "int", "new", "try", "var", "do", "if", NULL
};

static size_t java_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; java_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(java_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, java_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_java_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = java_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = java_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_java_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_java_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_java_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t java_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 4 <= max_len) {
        if (str_match_at(encoding, start, pos, max_len, ">>>=", false)) return 4;
    }
    if (pos + 3 <= max_len) {
        if (str_match_at(encoding, start, pos, max_len, "<<=", false) ||
            str_match_at(encoding, start, pos, max_len, ">>=", false)) return 3;
    }
    if (pos + 2 <= max_len) {
        static const char *const jops2[] = {
            "+=", "-=", "*=", "/=", "%=", "&=", "^=", "|=", "++", "--",
            "&&", "||", "<=", ">=", "==", "!=", "->", "::", NULL
        };
        for (int k = 0; jops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, jops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '&' ||
        c == '|' || c == '^' || c == '~' || c == '+' || c == '-' ||
        c == '*' || c == '/' || c == '%' || c == '?' || c == ':' ||
        c == ';' || c == '.' || c == ',') {
        return 1;
    }
    return 0;
}

bool _gen_java_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = java_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = java_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_java_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_java_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_java_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_java_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t java_escape_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '\\') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (c1 == '\\' || c1 == '"' || c1 == '\'' || c1 == 'n' || c1 == 'r' || c1 == 't') return 2;
    if (c1 == 'u' && pos + 6 <= max_len &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 2)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 3)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 4)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 5))) return 6;
    return 0;
}

bool _gen_java_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = java_escape_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = java_escape_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t java_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 < max_len && get_char_at(encoding, start, pos) == '0') {
        uint32_t c1 = get_char_at(encoding, start, pos + 1);
        if (c1 == 'x' || c1 == 'X') {
            if (is_xdigit_codepoint(get_char_at(encoding, start, pos + 2))) {
                size_t i = pos + 3;
                while (i < max_len) {
                    uint32_t c = get_char_at(encoding, start, i);
                    if (is_xdigit_codepoint(c) || c == '_') i++;
                    else break;
                }
                while (i < max_len) {
                    uint32_t suf = get_char_at(encoding, start, i);
                    if (suf == 'l' || suf == 'L') i++;
                    else break;
                }
                return i - pos;
            }
        }
        if (c1 == 'b' || c1 == 'B') {
            uint32_t c2 = get_char_at(encoding, start, pos + 2);
            if (c2 == '0' || c2 == '1') {
                size_t i = pos + 3;
                while (i < max_len) {
                    uint32_t c = get_char_at(encoding, start, i);
                    if (c == '0' || c == '1' || c == '_') i++;
                    else break;
                }
                while (i < max_len) {
                    uint32_t suf = get_char_at(encoding, start, i);
                    if (suf == 'l' || suf == 'L') i++;
                    else break;
                }
                return i - pos;
            }
        }
    }
    if (is_digit_codepoint(get_char_at(encoding, start, pos))) {
        size_t i = pos + 1;
        while (i < max_len) {
            uint32_t c = get_char_at(encoding, start, i);
            if (is_digit_codepoint(c) || c == '_') i++;
            else break;
        }
        if (i < max_len && get_char_at(encoding, start, i) == '.') {
            if (i + 1 < max_len && is_digit_codepoint(get_char_at(encoding, start, i + 1))) {
                i += 2;
                while (i < max_len) {
                    uint32_t c = get_char_at(encoding, start, i);
                    if (is_digit_codepoint(c) || c == '_') i++;
                    else break;
                }
            }
        }
        if (i < max_len) {
            uint32_t c = get_char_at(encoding, start, i);
            if (c == 'e' || c == 'E') {
                size_t e_pos = i;
                i++;
                if (i < max_len) {
                    uint32_t sign = get_char_at(encoding, start, i);
                    if (sign == '+' || sign == '-') i++;
                }
                size_t exp_digits = 0;
                while (i < max_len) {
                    uint32_t ec = get_char_at(encoding, start, i);
                    if (is_digit_codepoint(ec) || ec == '_') { i++; exp_digits++; }
                    else break;
                }
                if (exp_digits == 0) i = e_pos;
            }
        }
        if (i < max_len) {
            uint32_t suf = get_char_at(encoding, start, i);
            if (suf == 'f' || suf == 'F' || suf == 'd' || suf == 'D' || suf == 'l' || suf == 'L') i++;
        }
        return i - pos;
    }
    return 0;
}

bool _gen_java_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = java_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = java_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_java_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_java_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_java_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_java_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_java_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === C# Matchers Implementation ===                                        */
/* ========================================================================= */

bool _gen_csharp_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_csharp_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_csharp_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static const char *const cs_keywords[] = {
    "namespace", "interface", "protected", "volatile", "continue", "abstract",
    "delegate", "explicit", "implicit", "internal", "operator", "override",
    "readonly", "decimal", "default", "finally", "foreach", "partial",
    "private", "virtual", "dynamic", "operator", "params", "record", "remove",
    "return", "sealed", "sizeof", "static", "string", "struct", "switch",
    "typeof", "unsafe", "ushort", "double", "extern", "global", "object",
    "public", "sizeof", "stackalloc", "string", "struct", "switch", "target",
    "throw", "ulong", "using", "value", "while", "yield", "async", "await",
    "break", "catch", "class", "const", "event", "false", "fixed", "float",
    "is", "as", "by", "do", "if", "in", "on", "or", "to", "try", "var",
    "byte", "case", "char", "descending", "else", "enum", "from", "goto",
    "into", "join", "let", "lock", "long", "null", "out", "ref", "sbyte",
    "select", "short", "this", "true", "uint", "void", "when", "where",
    "add", "and", "get", "not", "set", NULL
};

static size_t cs_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; cs_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(cs_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, cs_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_csharp_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = cs_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cs_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_csharp_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_csharp_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_csharp_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t cs_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        if (str_match_at(encoding, start, pos, max_len, "\?\?=", false) ||
            str_match_at(encoding, start, pos, max_len, "<<=", false) ||
            str_match_at(encoding, start, pos, max_len, ">>=", false)) return 3;
    }
    if (pos + 2 <= max_len) {
        static const char *const cs_ops2[] = {
            "?.", "??", "=>", "+=", "-=", "*=", "/=", "%=", "&=", "^=",
            "|=", "++", "--", "&&", "||", "<<", ">>", "<=", ">=", "==",
            "!=", "->", "::", NULL
        };
        for (int k = 0; cs_ops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, cs_ops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '&' ||
        c == '|' || c == '^' || c == '~' || c == '+' || c == '-' ||
        c == '*' || c == '/' || c == '%' || c == '?' || c == ':' ||
        c == ';' || c == '.' || c == ',') {
        return 1;
    }
    return 0;
}

bool _gen_csharp_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = cs_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cs_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_csharp_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_csharp_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_csharp_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_csharp_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_csharp_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_StringEscape_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t cs_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 < max_len && get_char_at(encoding, start, pos) == '0') {
        uint32_t c1 = get_char_at(encoding, start, pos + 1);
        if (c1 == 'x' || c1 == 'X') {
            if (is_xdigit_codepoint(get_char_at(encoding, start, pos + 2))) {
                size_t i = pos + 3;
                while (i < max_len) {
                    uint32_t c = get_char_at(encoding, start, i);
                    if (is_xdigit_codepoint(c) || c == '_') i++;
                    else break;
                }
                while (i < max_len) {
                    uint32_t suf = get_char_at(encoding, start, i);
                    if (suf == 'u' || suf == 'U' || suf == 'l' || suf == 'L') i++;
                    else break;
                }
                return i - pos;
            }
        }
        if (c1 == 'b' || c1 == 'B') {
            uint32_t c2 = get_char_at(encoding, start, pos + 2);
            if (c2 == '0' || c2 == '1') {
                size_t i = pos + 3;
                while (i < max_len) {
                    uint32_t c = get_char_at(encoding, start, i);
                    if (c == '0' || c == '1' || c == '_') i++;
                    else break;
                }
                while (i < max_len) {
                    uint32_t suf = get_char_at(encoding, start, i);
                    if (suf == 'u' || suf == 'U' || suf == 'l' || suf == 'L') i++;
                    else break;
                }
                return i - pos;
            }
        }
    }
    if (is_digit_codepoint(get_char_at(encoding, start, pos))) {
        size_t i = pos + 1;
        while (i < max_len) {
            uint32_t c = get_char_at(encoding, start, i);
            if (is_digit_codepoint(c) || c == '_') i++;
            else break;
        }
        if (i < max_len && get_char_at(encoding, start, i) == '.') {
            if (i + 1 < max_len && is_digit_codepoint(get_char_at(encoding, start, i + 1))) {
                i += 2;
                while (i < max_len) {
                    uint32_t c = get_char_at(encoding, start, i);
                    if (is_digit_codepoint(c) || c == '_') i++;
                    else break;
                }
            }
        }
        if (i < max_len) {
            uint32_t c = get_char_at(encoding, start, i);
            if (c == 'e' || c == 'E') {
                size_t e_pos = i;
                i++;
                if (i < max_len) {
                    uint32_t sign = get_char_at(encoding, start, i);
                    if (sign == '+' || sign == '-') i++;
                }
                size_t exp_digits = 0;
                while (i < max_len) {
                    uint32_t ec = get_char_at(encoding, start, i);
                    if (is_digit_codepoint(ec) || ec == '_') { i++; exp_digits++; }
                    else break;
                }
                if (exp_digits == 0) i = e_pos;
            }
        }
        if (i < max_len) {
            uint32_t suf = get_char_at(encoding, start, i);
            if (suf == 'f' || suf == 'F' || suf == 'd' || suf == 'D' || suf == 'l' || suf == 'L' || suf == 'm' || suf == 'M') i++;
        }
        return i - pos;
    }
    return 0;
}

bool _gen_csharp_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = cs_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = cs_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_csharp_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_csharp_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_csharp_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_csharp_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_csharp_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === PHP Matchers Implementation ===                                       */
/* ========================================================================= */

static size_t php_tag_start_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 2 <= max_len && str_match_at(encoding, start, pos, max_len, "<?", false)) {
        if (pos + 5 <= max_len && str_match_at(encoding, start, pos, max_len, "<?php", true)) {
            return 5;
        }
        return 2;
    }
    return 0;
}

bool _gen_php_Tag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = php_tag_start_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        size_t m = php_tag_start_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_php_Tag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "?>", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "?>", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

static size_t php_line_comment_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    uint32_t c0 = get_char_at(encoding, start, pos);
    if (c0 == '#') {
        size_t i = pos + 1;
        while (i < max_len) {
            uint32_t c = get_char_at(encoding, start, i);
            if (c == '\r' || c == '\n') break;
            i++;
        }
        return i - pos;
    }
    if (c0 == '/' && pos + 1 < max_len && get_char_at(encoding, start, pos + 1) == '/') {
        size_t i = pos + 2;
        while (i < max_len) {
            uint32_t c = get_char_at(encoding, start, i);
            if (c == '\r' || c == '\n') break;
            i++;
        }
        return i - pos;
    }
    return 0;
}

bool _gen_php_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = php_line_comment_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = php_line_comment_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_php_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_php_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_php_ArrayKeyValue_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "=>", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "=>", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

bool _gen_php_MemberAccess_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "->", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "->", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

static size_t php_var_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '$') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (!is_alpha_codepoint(c1) && c1 != '_') return 0;
    size_t i = pos + 2;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_') i++;
        else break;
    }
    return i - pos;
}

bool _gen_php_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = php_var_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = php_var_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_php_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_php_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t php_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        if (str_match_at(encoding, start, pos, max_len, "===", false) ||
            str_match_at(encoding, start, pos, max_len, "!==", false)) return 3;
    }
    if (pos + 2 <= max_len) {
        static const char *const php_ops2[] = {
            "==", "!=", "+=", "-=", "*=", "/=", ".=", "%=", "++", "--",
            "<=", ">=", "&&", "||", NULL
        };
        for (int k = 0; php_ops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, php_ops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '&' ||
        c == '|' || c == '^' || c == '~' || c == '+' || c == '-' ||
        c == '*' || c == '/' || c == '%' || c == '?' || c == ':' ||
        c == ';') {
        return 1;
    }
    return 0;
}

bool _gen_php_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = php_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = php_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_php_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_php_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_php_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_php_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t php_escape_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '\\') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (c1 == '\\' || c1 == '$' || c1 == '"' || c1 == '\'' || c1 == 'n' || c1 == 'r' || c1 == 't') return 2;
    return 0;
}

bool _gen_php_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = php_escape_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = php_escape_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t php_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len || !is_digit_codepoint(get_char_at(encoding, start, pos))) return 0;
    size_t i = pos + 1;
    while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
    if (i < max_len && get_char_at(encoding, start, i) == '.') {
        i++;
        while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
    }
    return i - pos;
}

bool _gen_php_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = php_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = php_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_php_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static const char *const php_keywords[] = {
    "require_once", "include_once", "function", "require", "include",
    "namespace", "protected", "continue", "default", "finally", "foreach",
    "private", "elseif", "global", "public", "return", "static", "switch",
    "class", "print", "throw", "while", "break", "catch", "echo", "else",
    "case", "for", "new", "try", "use", "do", "if", "as", NULL
};

static size_t php_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; php_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(php_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, php_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_php_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = php_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = php_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_php_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_php_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_php_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_php_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === Go Matchers Implementation ===                                        */
/* ========================================================================= */

bool _gen_go_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_go_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_go_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static const char *const go_keywords[] = {
    "fallthrough", "interface", "continue", "default", "package", "return",
    "select", "switch", "break", "const", "defer", "range", "struct", "case",
    "chan", "else", "func", "goto", "type", "for", "map", "var", "go", "if",
    NULL
};

static size_t go_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; go_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(go_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, go_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_go_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = go_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = go_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t go_bool_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    static const char *const gbools[] = { "false", "true", "iota", "nil", NULL };
    for (int k = 0; gbools[k] != NULL; k++) {
        size_t blen = strlen(gbools[k]);
        if (str_match_at(encoding, start, pos, max_len, gbools[k], false)) {
            if (pos + blen == max_len) return blen;
            uint32_t after = get_char_at(encoding, start, pos + blen);
            if (!is_alnum_codepoint(after) && after != '_') return blen;
        }
    }
    return 0;
}

bool _gen_go_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = go_bool_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = go_bool_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_go_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_go_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_go_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t go_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        static const char *const gops3[] = { "...", "<<=", ">>=", "&^=", NULL };
        for (int k = 0; gops3[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, gops3[k], false)) return 3;
        }
    }
    if (pos + 2 <= max_len) {
        static const char *const gops2[] = {
            "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
            "&&", "||", "<=", ">=", "==", "!=", ":=", "<-", "<<", ">>",
            "&^", NULL
        };
        for (int k = 0; gops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, gops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '+' ||
        c == '-' || c == '*' || c == '/' || c == '%' || c == '&' ||
        c == '|' || c == '^' || c == ':' || c == ';' || c == '.' ||
        c == ',') {
        return 1;
    }
    return 0;
}

bool _gen_go_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = go_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = go_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_go_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_go_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_go_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_go_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_go_BacktickString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_TemplateString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_go_BacktickString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_TemplateString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t go_escape_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '\\') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (c1 == 'a' || c1 == 'b' || c1 == 'f' || c1 == 'n' || c1 == 'r' || c1 == 't' || c1 == 'v' ||
        c1 == '\\' || c1 == '"' || c1 == '\'') return 2;
    if (c1 == 'x' && pos + 4 <= max_len &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 2)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 3))) return 4;
    if (c1 == 'u' && pos + 6 <= max_len &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 2)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 3)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 4)) &&
        is_xdigit_codepoint(get_char_at(encoding, start, pos + 5))) return 6;
    if (c1 == 'U' && pos + 10 <= max_len) {
        bool all_hex = true;
        for (size_t k = 2; k < 10; k++) {
            if (!is_xdigit_codepoint(get_char_at(encoding, start, pos + k))) { all_hex = false; break; }
        }
        if (all_hex) return 10;
    }
    if (c1 >= '0' && c1 <= '7' && pos + 4 <= max_len) {
        uint32_t c2 = get_char_at(encoding, start, pos + 2);
        uint32_t c3 = get_char_at(encoding, start, pos + 3);
        if (c2 >= '0' && c2 <= '7' && c3 >= '0' && c3 <= '7') return 4;
    }
    return 0;
}

bool _gen_go_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = go_escape_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = go_escape_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_go_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_Number_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_go_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_go_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_go_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_go_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === SQL Matchers Implementation ===                                       */
/* ========================================================================= */

bool _gen_sql_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "--", false)) {
            size_t i = 2;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (c == '\r' || c == '\n') break;
                i++;
            }
            if (offset) *offset = 0;
            if (length) *length = i;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "--", false)) {
            size_t i = pos + 2;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (c == '\r' || c == '\n') break;
                i++;
            }
            if (offset) *offset = pos;
            if (length) *length = i - pos;
            return true;
        }
    }
    return false;
}

bool _gen_sql_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static const char *const sql_keywords[] = {
    "transaction", "references", "constraint", "procedure", "intersect",
    "recursive", "distinct", "database", "rollback", "between", "declare",
    "foreign", "primary", "returns", "trigger", "default", "delete", "except",
    "exists", "having", "insert", "offset", "revoke", "select", "unique",
    "update", "values", "alter", "begin", "check", "column", "commit",
    "create", "filter", "grant", "group", "index", "inner", "limit", "order",
    "outer", "right", "table", "union", "where", "after", "check", "drop",
    "else", "from", "into", "join", "left", "like", "null", "then", "view",
    "with", "all", "and", "any", "end", "not", "set", "use", "as", "by",
    "if", "in", "is", "on", "or", NULL
};

static size_t sql_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; sql_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(sql_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, sql_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_sql_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = sql_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = sql_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static const char *const sql_datatypes[] = {
    "timestamp", "datetime", "boolean", "decimal", "integer", "numeric",
    "varchar", "double", "float", "blob", "bool", "char", "clob", "date",
    "json", "real", "text", "time", "uuid", "int", NULL
};

static size_t sql_datatype_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; sql_datatypes[k] != NULL; k++) {
        size_t kw_len = strlen(sql_datatypes[k]);
        if (str_match_at(encoding, start, pos, max_len, sql_datatypes[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_sql_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = sql_datatype_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = sql_datatype_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_sql_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_BacktickIdentifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_TemplateString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_BacktickIdentifier_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_TemplateString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_BracketIdentifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_BracketIdentifier_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t sql_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 <= max_len) {
        static const char *const sql_ops2[] = { "<>", "<=", ">=", "!=", ":=", NULL };
        for (int k = 0; sql_ops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, sql_ops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '-' || c == '+' || c == '*' || c == '/' || c == '%' ||
        c == '&' || c == '|' || c == '^' || c == '~' || c == '<' ||
        c == '>' || c == '!' || c == '=' || c == ';' || c == '.' ||
        c == ',') {
        return 1;
    }
    return 0;
}

bool _gen_sql_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = sql_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = sql_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_sql_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_StringEscape_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t sql_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len || !is_digit_codepoint(get_char_at(encoding, start, pos))) return 0;
    if (pos > 0 && (is_alnum_codepoint(get_char_at(encoding, start, pos - 1)) || get_char_at(encoding, start, pos - 1) == '_')) return 0;
    size_t i = pos + 1;
    while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
    if (i < max_len && get_char_at(encoding, start, i) == '.') {
        if (i + 1 < max_len && is_digit_codepoint(get_char_at(encoding, start, i + 1))) {
            i += 2;
            while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
        }
    }
    if (i < max_len && (is_alnum_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) return 0;
    return i - pos;
}

bool _gen_sql_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = sql_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = sql_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_sql_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_sql_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === Bash Matchers Implementation ===                                      */
/* ========================================================================= */

bool _gen_bash_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_python_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static const char *const bash_keywords[] = {
    "function", "continue", "declare", "readonly", "unalias", "builtin",
    "command", "select", "return", "source", "export", "until", "while",
    "alias", "break", "eval", "exec", "shift", "time", "local", "case",
    "done", "elif", "else", "esac", "exit", "for", "then", "do", "fi",
    "if", "in", NULL
};

static size_t bash_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; bash_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(bash_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, bash_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_bash_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = bash_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = bash_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_bash_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t bash_var_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (get_char_at(encoding, start, pos) == '$') {
        if (pos + 1 >= max_len) return 0;
        uint32_t c1 = get_char_at(encoding, start, pos + 1);
        if (c1 == '{') {
            size_t i = pos + 2;
            if (i < max_len && (is_alpha_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) {
                i++;
                while (i < max_len) {
                    uint32_t c = get_char_at(encoding, start, i);
                    if (is_alnum_codepoint(c) || c == '_') i++;
                    else break;
                }
                if (i < max_len && get_char_at(encoding, start, i) == '}') return (i + 1) - pos;
            }
            return 0;
        }
        if (is_alpha_codepoint(c1) || c1 == '_') {
            size_t i = pos + 2;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (is_alnum_codepoint(c) || c == '_') i++;
                else break;
            }
            return i - pos;
        }
        if (is_digit_codepoint(c1) || c1 == '@' || c1 == '?' || c1 == '*' || c1 == '#' || c1 == '$' || c1 == '-') {
            return 2;
        }
        return 0;
    }
    uint32_t c0 = get_char_at(encoding, start, pos);
    if (is_alpha_codepoint(c0) || c0 == '_') {
        size_t i = pos + 1;
        while (i < max_len) {
            uint32_t c = get_char_at(encoding, start, i);
            if (is_alnum_codepoint(c) || c == '_') i++;
            else break;
        }
        if (i < max_len && get_char_at(encoding, start, i) == '=') {
            return i - pos;
        }
    }
    return 0;
}

bool _gen_bash_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = bash_var_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = bash_var_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_bash_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_bash_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t bash_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 <= max_len) {
        static const char *const bops2[] = { "&&", "||", ">>", "<<", "==", "!=", "+=", "-=", NULL };
        for (int k = 0; bops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, bops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '-' || c == '+' || c == '*' || c == '/' || c == '%' ||
        c == '&' || c == '|' || c == '^' || c == '~' || c == '<' ||
        c == '>' || c == '!' || c == '=' || c == ';' || c == '.' ||
        c == ',') {
        return 1;
    }
    return 0;
}

bool _gen_bash_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = bash_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = bash_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_bash_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_bash_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_bash_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_bash_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t bash_escape_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '\\') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (c1 == '\\' || c1 == '"' || c1 == '\'' || c1 == 'n' || c1 == 'r' || c1 == 't' || c1 == '$') return 2;
    return 0;
}

bool _gen_bash_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = bash_escape_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = bash_escape_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_bash_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_sql_Number_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_bash_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_bash_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_bash_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_bash_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === C3 Matchers Implementation ===                                        */
/* ========================================================================= */

bool _gen_c3_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_c3_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_c3_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static const char *const c3_keywords[] = {
    "interface", "continue", "default", "defer", "distinct", "fault", "finally",
    "inline", "module", "return", "static", "struct", "switch", "typedef",
    "union", "bitstruct", "break", "catch", "const", "macro", "tlocal",
    "var", "while", "case", "else", "enum", "fn", "for", "import", "nextcase",
    "null", "typeid", "try", "do", "if", NULL
};

static size_t c3_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; c3_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(c3_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, c3_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_c3_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = c3_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = c3_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static const char *const c3_types[] = {
    "bfloat16", "float128", "float16", "float64", "float", "double", "float80",
    "double", "int128", "uint128", "int64", "uint64", "int32", "uint32",
    "int16", "uint16", "int8", "uint8", "short", "ushort", "long", "ulong",
    "char", "byte", "bool", "void", "any", "ichar", "int", "uint", "iptr", "uptr",
    "isz", "usz", NULL
};

static size_t c3_datatype_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; c3_types[k] != NULL; k++) {
        size_t kw_len = strlen(c3_types[k]);
        if (str_match_at(encoding, start, pos, max_len, c3_types[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_c3_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = c3_datatype_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = c3_datatype_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_c3_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t c3_builtin_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '$') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (!is_alpha_codepoint(c1) && c1 != '_') return 0;
    size_t i = pos + 2;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_') i++;
        else break;
    }
    return i - pos;
}

bool _gen_c3_Builtin_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = c3_builtin_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = c3_builtin_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_c3_Attribute_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_java_Annotation_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_c3_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_c3_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_c3_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_c3_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_c3_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_StringEscape_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_c3_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_Number_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_c3_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t c3_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        static const char *const c3_ops3[] = { "...", ">>>", "<<=", ">>=", NULL };
        for (int k = 0; c3_ops3[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, c3_ops3[k], false)) return 3;
        }
    }
    if (pos + 2 <= max_len) {
        static const char *const c3_ops2[] = {
            "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
            "&&", "||", "<=", ">=", "==", "!=", "->", "<<", ">>", "??",
            "?:", "::", "??", NULL
        };
        for (int k = 0; c3_ops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, c3_ops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '|' ||
        c == '^' || c == '&' || c == '*' || c == '/' || c == '%' ||
        c == '+' || c == '-' || c == '.' || c == '~' || c == '?' ||
        c == ':' || c == ',' || c == ';') {
        return 1;
    }
    return 0;
}

bool _gen_c3_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = c3_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = c3_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_c3_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_c3_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_c3_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_c3_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_c3_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_c3_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === Zig Matchers Implementation ===                                       */
/* ========================================================================= */

bool _gen_zig_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static const char *const zig_keywords[] = {
    "unreachable", "threadlocal", "comptime", "continue", "noreturn",
    "suspend", "test", "volatile", "extern", "opaque", "packed", "resume",
    "return", "struct", "switch", "align", "allowzero", "anyframe", "anytype",
    "async", "await", "break", "catch", "const", "defer", "error", "export",
    "inline", "noalias", "noinline", "pub", "try", "union", "usingnamespace",
    "var", "while", "else", "enum", "fn", "for", "if", "or", "and", "asm", NULL
};

static size_t zig_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; zig_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(zig_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, zig_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_zig_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = zig_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = zig_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static const char *const zig_types[] = {
    "c_longdouble", "c_longlong", "c_ulonglong", "c_ulong", "c_ushort", "c_uint",
    "c_char", "c_int", "c_long", "c_short", "isize", "usize", "f128", "f16", "f32", "f64", "f80",
    "i128", "i16", "i32", "i64", "i8", "u128", "u16", "u32", "u64", "u8",
    "bool", "void", "type", "anyerror", "anyopaque", "comptime_float", "comptime_int",
    "noreturn", NULL
};

static size_t zig_datatype_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; zig_types[k] != NULL; k++) {
        size_t kw_len = strlen(zig_types[k]);
        if (str_match_at(encoding, start, pos, max_len, zig_types[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_zig_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = zig_datatype_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = zig_datatype_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_zig_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_zig_Builtin_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_java_Annotation_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_zig_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_zig_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_zig_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_zig_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_zig_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_StringEscape_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_zig_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_Number_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_zig_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t zig_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        static const char *const zops3[] = { "... ", "...", "<<=", ">>=", NULL };
        for (int k = 0; zops3[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, zops3[k], false)) return strlen(zops3[k]);
        }
    }
    if (pos + 2 <= max_len) {
        static const char *const zops2[] = {
            "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
            "==", "!=", "<=", ">=", "=>", "->", "..", "||", "&&", "<<",
            ">>", "?.", "?:", ".*", ".?", NULL
        };
        for (int k = 0; zops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, zops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '|' ||
        c == '&' || c == '^' || c == '*' || c == '/' || c == '%' ||
        c == '+' || c == '-' || c == '~' || c == '?' || c == ':' ||
        c == '.' || c == ',' || c == ';') {
        return 1;
    }
    return 0;
}

bool _gen_zig_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = zig_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = zig_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_zig_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_zig_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_zig_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_zig_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_zig_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_zig_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === Swift Matchers Implementation ===                                     */
/* ========================================================================= */

bool _gen_swift_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static const char *const swift_keywords[] = {
    "associatedtype", "convenience", "fileprivate", "precedencegroup", "deinit",
    "dynamic", "extension", "indirect", "infix", "init", "inout", "internal",
    "lazy", "mutating", "nonmutating", "operator", "optional", "override", "postfix",
    "prefix", "protocol", "required", "rethrows", "struct", "subscript", "typealias",
    "unowned", "weak", "where", "while", "class", "defer", "enum", "fallthrough",
    "final", "func", "guard", "import", "open", "private", "public", "repeat",
    "return", "static", "switch", "throw", "throws", "break", "case", "catch",
    "continue", "default", "do", "else", "for", "if", "in", "is", "as", "let",
    "nil", "self", "Self", "some", "super", "try", "var", "any", "async", "await",
    NULL
};

static size_t swift_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; swift_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(swift_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, swift_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_swift_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = swift_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = swift_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static const char *const swift_types[] = {
    "Character", "Float80", "Double", "Float", "Int64", "UInt64", "Int32", "UInt32",
    "Int16", "UInt16", "Int8", "UInt8", "String", "Bool", "Void", "Int", "UInt",
    "Any", "Never", NULL
};

static size_t swift_datatype_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; swift_types[k] != NULL; k++) {
        size_t kw_len = strlen(swift_types[k]);
        if (str_match_at(encoding, start, pos, max_len, swift_types[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_swift_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = swift_datatype_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = swift_datatype_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_swift_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_swift_Attribute_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_java_Annotation_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_swift_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_StringEscape_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_Number_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t swift_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        static const char *const sops3[] = { "...", "..<", "===", "!==", "<<=", ">>=", NULL };
        for (int k = 0; sops3[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, sops3[k], false)) return 3;
        }
    }
    if (pos + 2 <= max_len) {
        static const char *const sops2[] = {
            "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
            "==", "!=", "<=", ">=", "->", "&&", "||", "??", "?.", "?:",
            "<<", ">>", "~=", "..", NULL
        };
        for (int k = 0; sops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, sops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '|' ||
        c == '&' || c == '^' || c == '*' || c == '/' || c == '%' ||
        c == '+' || c == '-' || c == '~' || c == '?' || c == ':' ||
        c == '.' || c == ',' || c == ';') {
        return 1;
    }
    return 0;
}

bool _gen_swift_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = swift_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = swift_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_swift_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === Pascal Matchers Implementation ===                                    */
/* ========================================================================= */

bool _gen_pascal_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t pascal_brace_comment_start_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len || get_char_at(encoding, start, pos) != '{') return 0;
    return 1;
}

bool _gen_pascal_BlockCommentBrace_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (pascal_brace_comment_start_at(encoding, start, 0, max_len)) {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (pascal_brace_comment_start_at(encoding, start, pos, max_len)) {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

bool _gen_pascal_BlockCommentBrace_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (max_len > 0 && get_char_at(encoding, start, 0) == '}') {
            if (offset) *offset = 0;
            if (length) *length = 1;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (get_char_at(encoding, start, pos) == '}') {
            if (offset) *offset = pos;
            if (length) *length = 1;
            return true;
        }
    }
    return false;
}

bool _gen_pascal_BlockCommentParen_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "(*", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "(*", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

bool _gen_pascal_BlockCommentParen_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "*)", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "*)", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

static const char *const pascal_keywords[] = {
    "constructor", "destructor", "procedure", "inherited", "otherwise",
    "interface", "published", "protected", "function", "continue", "abstract",
    "override", "virtual", "dynamic", "default", "finally", "package",
    "private", "program", "forward", "strict", "exports", "library", "message",
    "record", "repeat", "threadvar", "inline", "packed", "public", "reintroduce",
    "resourcestring", "safecall", "stdcall", "string", "switch", "cdecl",
    "class", "const", "except", "final", "label", "raise", "until", "while",
    "array", "begin", "cdecl", "case", "else", "file", "goto", "then", "type",
    "unit", "uses", "with", "asm", "div", "downto", "end", "for", "mod", "nil",
    "not", "out", "set", "try", "var", "as", "do", "if", "in", "is", "of", "on",
    "or", "to", "and", "xor", "shl", "shr", NULL
};

static size_t pascal_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; pascal_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(pascal_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, pascal_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_pascal_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = pascal_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = pascal_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static const char *const pascal_types[] = {
    "ansistring", "widestring", "unicodestring", "shortstring", "longword",
    "cardinal", "smallint", "extended", "currency", "datetime", "boolean",
    "integer", "single", "double", "int64", "qword", "word", "byte", "char",
    "comp", "date", "real", "time", "uint64", "pointer", "pchar", NULL
};

static size_t pascal_datatype_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; pascal_types[k] != NULL; k++) {
        size_t kw_len = strlen(pascal_types[k]);
        if (str_match_at(encoding, start, pos, max_len, pascal_types[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_pascal_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = pascal_datatype_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = pascal_datatype_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_pascal_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_ada_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_pascal_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_pascal_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_pascal_EscapedQuote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "''", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "''", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

static size_t pascal_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    uint32_t c0 = get_char_at(encoding, start, pos);
    if (c0 == '$' || c0 == '&' || c0 == '%') {
        if (pos + 1 >= max_len) return 0;
        size_t i = pos + 1;
        while (i < max_len && (is_xdigit_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) i++;
        if (i > pos + 1) return i - pos;
        return 0;
    }
    if (is_digit_codepoint(c0)) {
        size_t i = pos + 1;
        while (i < max_len && (is_digit_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) i++;
        if (i < max_len && get_char_at(encoding, start, i) == '.') {
            if (i + 1 < max_len && is_digit_codepoint(get_char_at(encoding, start, i + 1))) {
                i += 2;
                while (i < max_len && (is_digit_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) i++;
            }
        }
        if (i < max_len) {
            uint32_t e = get_char_at(encoding, start, i);
            if (e == 'e' || e == 'E') {
                size_t e_pos = i++;
                if (i < max_len && (get_char_at(encoding, start, i) == '+' || get_char_at(encoding, start, i) == '-')) i++;
                size_t ed = 0;
                while (i < max_len && (is_digit_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) { i++; ed++; }
                if (ed == 0) i = e_pos;
            }
        }
        return i - pos;
    }
    return 0;
}

bool _gen_pascal_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = pascal_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = pascal_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_pascal_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t pascal_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 <= max_len) {
        static const char *const pops2[] = {
            ":=", "<>", "<=", ">=", "+=", "-=", "*=", "/=", "..", "(*", "*)", NULL
        };
        for (int k = 0; pops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, pops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '+' || c == '-' ||
        c == '*' || c == '/' || c == '@' || c == '^' || c == '.' ||
        c == ',' || c == ';' || c == ':') {
        return 1;
    }
    return 0;
}

bool _gen_pascal_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = pascal_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = pascal_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_pascal_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_pascal_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_pascal_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_pascal_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_pascal_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_pascal_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === Perl Matchers Implementation ===                                      */
/* ========================================================================= */

bool _gen_perl_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_python_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static const char *const perl_keywords[] = {
    "continue", "package", "require", "return", "format", "local", "my", "our",
    "state", "sub", "use", "break", "given", "when", "default", "while", "until",
    "unless", "redo", "next", "last", "goto", "exit", "eval", "else", "elsif",
    "for", "foreach", "do", "if", NULL
};

static size_t perl_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; perl_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(perl_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, perl_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_perl_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = perl_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = perl_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t perl_bool_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 5 <= max_len && str_match_at(encoding, start, pos, max_len, "undef", false)) {
        if (pos + 5 == max_len || (!is_alnum_codepoint(get_char_at(encoding, start, pos + 5)) && get_char_at(encoding, start, pos + 5) != '_')) {
            return 5;
        }
    }
    return 0;
}

bool _gen_perl_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = perl_bool_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = perl_bool_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_perl_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_perl_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_perl_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_perl_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_perl_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_StringEscape_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_perl_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_Number_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t perl_var_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    uint32_t c0 = get_char_at(encoding, start, pos);
    if (c0 != '$' && c0 != '@' && c0 != '%' && c0 != '*') return 0;
    if (pos + 1 >= max_len) return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (c1 == '$') return 2;
    if (c1 == '^' && pos + 2 < max_len && is_alpha_codepoint(get_char_at(encoding, start, pos + 2))) return 3;
    if (is_digit_codepoint(c1)) {
        size_t i = pos + 2;
        while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
        return i - pos;
    }
    if (c1 == ':' && pos + 2 < max_len && get_char_at(encoding, start, pos + 2) == ':') {
        size_t i = pos + 3;
        if (i < max_len && (is_alpha_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) {
            i++;
            while (i < max_len && (is_alnum_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) i++;
            return i - pos;
        }
    }
    if (is_alpha_codepoint(c1) || c1 == '_') {
        size_t i = pos + 2;
        while (i < max_len && (is_alnum_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) i++;
        return i - pos;
    }
    return 0;
}

bool _gen_perl_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = perl_var_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = perl_var_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t perl_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        static const char *const pops3[] = { "...", "<=>", "||=", NULL };
        for (int k = 0; pops3[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, pops3[k], false)) return 3;
        }
    }
    if (pos + 2 <= max_len) {
        static const char *const pops2[] = {
            "||", "=>", "->", "==", "!=", "<=", ">=", "=~", "!~", "&&",
            "//", "..", "++", "--", "**", "+=", "-=", "*=", "/=", "%=",
            "&=", "^=", "<<", ">>", NULL
        };
        for (int k = 0; pops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, pops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '|' ||
        c == '^' || c == '&' || c == '*' || c == '/' || c == '%' ||
        c == '+' || c == '-' || c == '.' || c == '~' || c == ',' ||
        c == ';') {
        return 1;
    }
    return 0;
}

bool _gen_perl_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = perl_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = perl_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_perl_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_perl_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_perl_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_perl_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_perl_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_perl_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === Fortran Matchers Implementation ===                                   */
/* ========================================================================= */

static size_t fortran_line_comment_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len || get_char_at(encoding, start, pos) != '!') return 0;
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (c == '\r' || c == '\n') break;
        i++;
    }
    return i - pos;
}

bool _gen_fortran_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = fortran_line_comment_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = fortran_line_comment_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static const char *const fortran_keywords[] = {
    "allocatable", "associated", "assignment", "deallocate", "equivalence",
    "subroutine", "abstract", "allocate", "contains", "continue", "critical",
    "deferred", "dimension", "elemental", "elsewhere", "interface", "intrinsic",
    "parameter", "procedure", "protected", "recursive", "external", "function",
    "implicit", "nopass", "nullify", "operator", "optional", "program",
    "block", "bind", "call", "case", "class", "close", "common", "cycle",
    "data", "default", "do", "else", "elseif", "end", "enddo", "endif",
    "enum", "error", "exit", "extends", "final", "forall", "format", "generic",
    "go", "if", "import", "in", "include", "inout", "intent", "module", "new",
    "none", "null", "only", "open", "out", "pass", "pointer", "print", "private",
    "public", "pure", "read", "return", "save", "select", "stop", "target",
    "then", "to", "type", "use", "wait", "where", "while", "write", NULL
};

static size_t fortran_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; fortran_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(fortran_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, fortran_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_fortran_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = fortran_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = fortran_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t fortran_bool_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos + 6 <= max_len && str_match_at(encoding, start, pos, max_len, ".true.", caseless)) return 6;
    if (pos + 7 <= max_len && str_match_at(encoding, start, pos, max_len, ".false.", caseless)) return 7;
    return 0;
}

bool _gen_fortran_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = fortran_bool_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = fortran_bool_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_fortran_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_fortran_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_fortran_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_fortran_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_fortran_EscapedApostrophe_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_pascal_EscapedQuote_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_fortran_EscapedDoubleQuote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "\"\"", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "\"\"", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

bool _gen_fortran_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Number_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_fortran_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t fortran_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 <= max_len) {
        static const char *const fops2[] = { "::", "==", "/=", ">=", "<=", "=>", "**", NULL };
        for (int k = 0; fops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, fops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '=' ||
        c == '<' || c == '>' || c == ',' || c == ':' || c == '%' ||
        c == ';') {
        return 1;
    }
    return 0;
}

bool _gen_fortran_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = fortran_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = fortran_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_fortran_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_fortran_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_fortran_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_fortran_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_fortran_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_fortran_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === Ada Matchers Implementation ===                                       */
/* ========================================================================= */

bool _gen_ada_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "--", false)) {
            size_t i = 2;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (c == '\r' || c == '\n') break;
                i++;
            }
            if (offset) *offset = 0;
            if (length) *length = i;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "--", false)) {
            size_t i = pos + 2;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (c == '\r' || c == '\n') break;
                i++;
            }
            if (offset) *offset = pos;
            if (length) *length = i - pos;
            return true;
        }
    }
    return false;
}

static const char *const ada_keywords[] = {
    "synchronized", "overriding", "terminate", "protected", "exception",
    "abstract", "constant", "function", "generic", "limited", "package",
    "pragma", "procedure", "renames", "requeue", "reverse", "separate",
    "subtype", "accept", "access", "aliased", "declare", "digits", "elsif",
    "record", "tagged", "until", "abort", "array", "begin", "delta", "entry",
    "range", "select", "while", "body", "case", "else", "exit", "goto", "loop",
    "null", "some", "task", "then", "type", "when", "with", "all", "and", "abs",
    "at", "do", "for", "if", "in", "is", "mod", "new", "not", "of", "or", "out",
    "rem", "use", "xor", "end", NULL
};

static size_t ada_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; ada_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(ada_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, ada_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_ada_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = ada_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = ada_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static const char *const ada_types[] = {
    "wide_character", "wide_string", "long_integer", "short_integer",
    "long_float", "short_float", "character", "duration", "positive",
    "boolean", "integer", "natural", "address", "string", "float", "count",
    NULL
};

static size_t ada_datatype_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; ada_types[k] != NULL; k++) {
        size_t kw_len = strlen(ada_types[k]);
        if (str_match_at(encoding, start, pos, max_len, ada_types[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_ada_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = ada_datatype_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = ada_datatype_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_ada_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "true", true)) {
            if (max_len == 4 || (!is_alnum_codepoint(get_char_at(encoding, start, 4)) && get_char_at(encoding, start, 4) != '_')) {
                if (offset) *offset = 0;
                if (length) *length = 4;
                return true;
            }
        }
        if (str_match_at(encoding, start, 0, max_len, "false", true)) {
            if (max_len == 5 || (!is_alnum_codepoint(get_char_at(encoding, start, 5)) && get_char_at(encoding, start, 5) != '_')) {
                if (offset) *offset = 0;
                if (length) *length = 5;
                return true;
            }
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "true", true)) {
            if (pos + 4 == max_len || (!is_alnum_codepoint(get_char_at(encoding, start, pos + 4)) && get_char_at(encoding, start, pos + 4) != '_')) {
                if (offset) *offset = pos;
                if (length) *length = 4;
                return true;
            }
        }
        if (str_match_at(encoding, start, pos, max_len, "false", true)) {
            if (pos + 5 == max_len || (!is_alnum_codepoint(get_char_at(encoding, start, pos + 5)) && get_char_at(encoding, start, pos + 5) != '_')) {
                if (offset) *offset = pos;
                if (length) *length = 5;
                return true;
            }
        }
    }
    return false;
}

static size_t ada_charlit_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 3 <= max_len && get_char_at(encoding, start, pos) == '\'' && get_char_at(encoding, start, pos + 2) == '\'') {
        return 3;
    }
    return 0;
}

bool _gen_ada_CharLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = ada_charlit_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 2 < max_len; pos++) {
        size_t m = ada_charlit_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_ada_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_ada_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_ada_EscapedQuote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_fortran_EscapedDoubleQuote_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t ada_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len || !is_digit_codepoint(get_char_at(encoding, start, pos))) return 0;
    size_t i = pos + 1;
    while (i < max_len && (is_digit_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) i++;
    if (i < max_len && get_char_at(encoding, start, i) == '#') {
        i++;
        while (i < max_len && (is_xdigit_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) i++;
        if (i < max_len && get_char_at(encoding, start, i) == '.') {
            i++;
            while (i < max_len && (is_xdigit_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) i++;
        }
        if (i < max_len && get_char_at(encoding, start, i) == '#') i++;
    } else if (i < max_len && get_char_at(encoding, start, i) == '.') {
        if (i + 1 < max_len && is_digit_codepoint(get_char_at(encoding, start, i + 1))) {
            i += 2;
            while (i < max_len && (is_digit_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) i++;
        }
    }
    if (i < max_len) {
        uint32_t e = get_char_at(encoding, start, i);
        if (e == 'e' || e == 'E') {
            size_t e_pos = i++;
            if (i < max_len && (get_char_at(encoding, start, i) == '+' || get_char_at(encoding, start, i) == '-')) i++;
            size_t ed = 0;
            while (i < max_len && (is_digit_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) { i++; ed++; }
            if (ed == 0) i = e_pos;
        }
    }
    return i - pos;
}

bool _gen_ada_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = ada_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = ada_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t ada_attrib_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '\'') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (!is_alpha_codepoint(c1) && c1 != '_') return 0;
    size_t i = pos + 2;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_') i++;
        else break;
    }
    return i - pos;
}

bool _gen_ada_Attribute_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = ada_attrib_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = ada_attrib_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_ada_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t ada_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 <= max_len) {
        static const char *const aops2[] = { ":=", "=>", "..", "<>", ">=", "<=", "**", "/=", NULL };
        for (int k = 0; aops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, aops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '+' || c == '-' ||
        c == '*' || c == '/' || c == '&' || c == '.' || c == ',' ||
        c == ';' || c == ':') {
        return 1;
    }
    return 0;
}

bool _gen_ada_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = ada_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = ada_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_ada_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_ada_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_ada_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_ada_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_ada_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_ada_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* === Missing Batch 3 Matchers === */

// zig MultiLineString: \\\\([^\r\n]*)
bool _gen_zig_MultiLineString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "\\\\", false)) {
            size_t i = 2;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (c == '\r' || c == '\n') break;
                i++;
            }
            if (offset) *offset = 0;
            if (length) *length = i;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "\\\\", false)) {
            size_t i = pos + 2;
            while (i < max_len) {
                uint32_t c = get_char_at(encoding, start, i);
                if (c == '\r' || c == '\n') break;
                i++;
            }
            if (offset) *offset = pos;
            if (length) *length = i - pos;
            return true;
        }
    }
    return false;
}

// zig Directive: @[a-zA-Z_][a-zA-Z0-9_]*\b
bool _gen_zig_Directive_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_java_Annotation_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// swift MultiLineString: """
bool _gen_swift_MultiLineString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_python_TripleDoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_swift_MultiLineString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_python_TripleDoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// swift StringInterpolation: \( ... \)
bool _gen_swift_StringInterpolation_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "\\(", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "\\(", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}
bool _gen_swift_StringInterpolation_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// perl PODBlock: ^=\w+ ... ^=cut
static size_t perl_pod_start_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len || get_char_at(encoding, start, pos) != '=') return 0;
    if (pos > 0) {
        uint32_t prev = get_char_at(encoding, start, pos - 1);
        if (prev != '\n' && prev != '\r') return 0;
    }
    if (pos + 1 >= max_len) return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (!is_alnum_codepoint(c1) && c1 != '_') return 0;
    size_t i = pos + 2;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_') i++;
        else break;
    }
    return i - pos;
}

bool _gen_perl_PODBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = perl_pod_start_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = perl_pod_start_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t perl_pod_end_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 4 > max_len) return 0;
    if (pos > 0) {
        uint32_t prev = get_char_at(encoding, start, pos - 1);
        if (prev != '\n' && prev != '\r') return 0;
    }
    if (str_match_at(encoding, start, pos, max_len, "=cut", false)) {
        return 4;
    }
    return 0;
}

bool _gen_perl_PODBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = perl_pod_end_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 3 < max_len; pos++) {
        size_t m = perl_pod_end_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// pascal CompilerDirective: {\$ ... }
bool _gen_pascal_CompilerDirective_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "{$", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "{$", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}
bool _gen_pascal_CompilerDirective_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_pascal_BlockCommentBrace_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// pascal BlockCommentCurly: { ... }
bool _gen_pascal_BlockCommentCurly_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_pascal_BlockCommentBrace_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_pascal_BlockCommentCurly_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_pascal_BlockCommentBrace_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// pascal CharLiteral: #[0-9]+|#\$[0-9a-fA-F]+
static size_t pascal_charlit_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '#') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (c1 == '$') {
        if (pos + 2 >= max_len) return 0;
        size_t i = pos + 2;
        while (i < max_len && is_xdigit_codepoint(get_char_at(encoding, start, i))) i++;
        if (i > pos + 2) return i - pos;
        return 0;
    }
    if (is_digit_codepoint(c1)) {
        size_t i = pos + 2;
        while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
        return i - pos;
    }
    return 0;
}

bool _gen_pascal_CharLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = pascal_charlit_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = pascal_charlit_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

// c3 DocComment: <* ... *>
bool _gen_c3_DocComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "<*", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "<*", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}
bool _gen_c3_DocComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "*>", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "*>", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

// c3 Directive: \$[a-zA-Z_][a-zA-Z0-9_]*\b
bool _gen_c3_Directive_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c3_Builtin_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// c3 Annotation: @[a-zA-Z_][a-zA-Z0-9_]*\b
bool _gen_c3_Annotation_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_java_Annotation_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === ASM Matchers Implementation ===                                       */
/* ========================================================================= */

bool _gen_asm_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        uint32_t c = get_char_at(encoding, start, 0);
        if (c == ';' || c == '#') {
            size_t i = 1;
            while (i < max_len) {
                uint32_t cc = get_char_at(encoding, start, i);
                if (cc == '\r' || cc == '\n') break;
                i++;
            }
            if (offset) *offset = 0;
            if (length) *length = i;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        uint32_t c = get_char_at(encoding, start, pos);
        if (c == ';' || c == '#') {
            size_t i = pos + 1;
            while (i < max_len) {
                uint32_t cc = get_char_at(encoding, start, i);
                if (cc == '\r' || cc == '\n') break;
                i++;
            }
            if (offset) *offset = pos;
            if (length) *length = i - pos;
            return true;
        }
    }
    return false;
}

static const char *const asm_instructions[] = {
    "section", "global", "extern", "cmovnz", "cmovne", "cmovnc", "cmovbe", "cmovae",
    "cmovpe", "cmovpo", "cmovge", "cmovle", "cmovno", "cmovns", "cmovnp", "cmovz",
    "cmove", "cmovc", "cmovb", "cmova", "cmovp", "cmovg", "cmovl", "cmovo", "cmovs",
    "movsx", "movzx", "movsxd", "cmpxchg", "syscall", "sysret", "setnz", "setne",
    "setnc", "setbe", "setae", "setpe", "setpo", "setge", "setle", "setno", "setns",
    "setnp", "setz", "sete", "setc", "setb", "seta", "setp", "setg", "setl", "seto",
    "sets", "pushf", "popfq", "pushfq", "popfd", "pushfd", "clflush", "rdtsc", "rdtscp",
    "cpuid", "stosq", "stosd", "stosw", "stosb", "movsq", "movsd", "movsw", "movsb",
    "scasq", "scasd", "scasw", "scasb", "cmpsq", "cmpsd", "cmpsw", "cmpsb", "lodsq",
    "lodsd", "lodsw", "lodsb", "loopne", "loopnz", "loope", "loopz", "pause", "enter",
    "leave", "imul", "idiv", "xchg", "test", "push", "call", "retq", "retf", "iret",
    "iretd", "iretq", "int3", "into", "into", "bswap", "bt", "bts", "btr", "btc", "bsf",
    "bsr", "jnz", "jne", "jnc", "jbe", "jae", "jpe", "jpo", "jge", "jle", "jno", "jns",
    "jnp", "jz", "je", "jc", "jb", "ja", "jp", "jg", "jl", "jo", "js", "jmp", "mov",
    "add", "sub", "mul", "div", "inc", "dec", "and", "xor", "not", "neg", "shl", "shr",
    "sar", "rol", "ror", "rcl", "rcr", "pop", "ret", "nop", "hlt", "int", "cli", "sti",
    "clc", "stc", "cld", "std", "loop", "cmp", "lea", "or", "in", "out", NULL
};

static size_t asm_instruction_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; asm_instructions[k] != NULL; k++) {
        size_t kw_len = strlen(asm_instructions[k]);
        if (str_match_at(encoding, start, pos, max_len, asm_instructions[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_asm_Instruction_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = asm_instruction_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = asm_instruction_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static const char *const asm_registers[] = {
    "r10w", "r10b", "r10d", "r11w", "r11b", "r11d", "r12w", "r12b", "r12d",
    "r13w", "r13b", "r13d", "r14w", "r14b", "r14d", "r15w", "r15b", "r15d",
    "r8w", "r8b", "r8d", "r9w", "r9b", "r9d", "rax", "rbx", "rcx", "rdx",
    "rsi", "rdi", "rbp", "rsp", "r8", "r9", "r10", "r11", "r12", "r13",
    "r14", "r15", "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
    "ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "al", "bl", "cl", "dl",
    "ah", "bh", "ch", "dh", "sil", "dil", "bpl", "spl", "cs", "ds", "ss",
    "es", "fs", "gs", "cr0", "cr2", "cr3", "cr4", "cr8", "dr0", "dr1", "dr2",
    "dr3", "dr6", "dr7", "rip", "eip", "ip", NULL
};

static size_t asm_register_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; asm_registers[k] != NULL; k++) {
        size_t kw_len = strlen(asm_registers[k]);
        if (str_match_at(encoding, start, pos, max_len, asm_registers[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_asm_Register_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = asm_register_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = asm_register_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static const char *const asm_directives[] = {
    "align", "ascii", "asciz", "byte", "data", "def", "double", "dword",
    "eject", "else", "elseif", "end", "endef", "endif", "equ", "equiv",
    "err", "exitm", "extern", "fail", "file", "fill", "float", "global",
    "globl", "hidden", "ident", "if", "incbin", "include", "int", "internal",
    "irp", "irpc", "lcomm", "line", "linkonce", "list", "ln", "loc", "local",
    "long", "macro", "mexit", "noformat", "nolist", "nopage", "octa", "org",
    "p2align", "page", "popsection", "previous", "print", "protected", "psize",
    "purgem", "pushsection", "quad", "rep", "rept", "sbttl", "scl", "section",
    "set", "short", "single", "size", "skip", "sleb128", "space", "stabd",
    "stabn", "stabs", "string", "struct", "subsection", "symver", "tag", "text",
    "title", "type", "uleb128", "val", "version", "vtable", "warning", "weak",
    "word", "xdef", "xref", "zero", "db", "dw", "dd", "dq", "dt", "resb",
    "resw", "resd", "resq", "rest", "ptr", "qword", NULL
};

static size_t asm_directive_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    size_t start_p = pos;
    if (c == '.') {
        start_p++;
        if (start_p >= max_len) return 0;
        c = get_char_at(encoding, start, start_p);
    }
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; asm_directives[k] != NULL; k++) {
        size_t kw_len = strlen(asm_directives[k]);
        if (str_match_at(encoding, start, start_p, max_len, asm_directives[k], caseless)) {
            if (start_p + kw_len == max_len) return (start_p + kw_len) - pos;
            uint32_t after = get_char_at(encoding, start, start_p + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return (start_p + kw_len) - pos;
            }
        }
    }
    return 0;
}

bool _gen_asm_Directive_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = asm_directive_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = asm_directive_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t asm_label_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    uint32_t c0 = get_char_at(encoding, start, pos);
    size_t i = pos;
    if (c0 == '.') i++;
    if (i >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, i);
    if (!is_alpha_codepoint(c) && c != '_' && c != '@' && c != '$') return 0;
    i++;
    while (i < max_len) {
        uint32_t cc = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(cc) || cc == '_' || cc == '@' || cc == '$' || cc == '?') i++;
        else break;
    }
    if (i < max_len && get_char_at(encoding, start, i) == ':') {
        return (i + 1) - pos;
    }
    return 0;
}

bool _gen_asm_Label_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = asm_label_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = asm_label_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_asm_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_asm_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_asm_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_asm_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_asm_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_StringEscape_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t asm_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 < max_len && get_char_at(encoding, start, pos) == '0') {
        uint32_t c1 = get_char_at(encoding, start, pos + 1);
        if (c1 == 'x' || c1 == 'X') {
            size_t i = pos + 2;
            while (i < max_len && is_xdigit_codepoint(get_char_at(encoding, start, i))) i++;
            if (i > pos + 2) return i - pos;
        }
        if (c1 == 'b' || c1 == 'B') {
            size_t i = pos + 2;
            while (i < max_len) {
                uint32_t bc = get_char_at(encoding, start, i);
                if (bc == '0' || bc == '1') i++;
                else break;
            }
            if (i > pos + 2) return i - pos;
        }
    }
    if (is_digit_codepoint(get_char_at(encoding, start, pos))) {
        size_t i = pos + 1;
        while (i < max_len && is_xdigit_codepoint(get_char_at(encoding, start, i))) i++;
        if (i < max_len) {
            uint32_t suf = get_char_at(encoding, start, i);
            if (suf == 'h' || suf == 'H' || suf == 'b' || suf == 'B' || suf == 'd' || suf == 'D' || suf == 'o' || suf == 'O' || suf == 'q' || suf == 'Q') {
                return (i + 1) - pos;
            }
        }
        // check standard decimal/float
        size_t j = pos + 1;
        while (j < max_len && is_digit_codepoint(get_char_at(encoding, start, j))) j++;
        if (j < max_len && get_char_at(encoding, start, j) == '.') {
            j++;
            while (j < max_len && is_digit_codepoint(get_char_at(encoding, start, j))) j++;
        }
        return j - pos;
    }
    return 0;
}

bool _gen_asm_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = asm_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = asm_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t asm_var_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_' && c != '@' && c != '$' && c != '.' && c != '?') return 0;
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t cc = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(cc) || cc == '_' || cc == '@' || cc == '$' || cc == '.' || cc == '?') i++;
        else break;
    }
    return i - pos;
}

bool _gen_asm_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = asm_var_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = asm_var_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t asm_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 <= max_len) {
        static const char *const aops2[] = { "<<", ">>", "==", "!=", "<=", ">=", NULL };
        for (int k = 0; aops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, aops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
        c == '&' || c == '|' || c == '^' || c == '~' || c == '=' ||
        c == '<' || c == '>' || c == '!' || c == ':' || c == ',' ||
        c == '$') {
        return 1;
    }
    return 0;
}

bool _gen_asm_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = asm_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = asm_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_asm_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_asm_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_asm_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_asm_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === MATLAB Matchers Implementation ===                                    */
/* ========================================================================= */

static size_t matlab_line_comment_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len || get_char_at(encoding, start, pos) != '%') return 0;
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (c == '\r' || c == '\n') break;
        i++;
    }
    return i - pos;
}

bool _gen_matlab_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = matlab_line_comment_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = matlab_line_comment_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_matlab_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "%{", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "%{", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

bool _gen_matlab_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "%}", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "%}", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

static const char *const matlab_keywords[] = {
    "arguments", "properties", "classdef", "methods", "events", "function",
    "enumeration", "continue", "persistent", "otherwise", "return", "global",
    "switch", "catch", "break", "while", "case", "else", "elseif", "for",
    "try", "end", "if", "spmd", "parfor", NULL
};

static size_t matlab_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; matlab_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(matlab_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, matlab_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_matlab_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = matlab_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = matlab_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_matlab_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_matlab_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_matlab_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_matlab_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_matlab_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_matlab_EscapedQuote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_pascal_EscapedQuote_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_matlab_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Number_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_matlab_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t matlab_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        if (str_match_at(encoding, start, pos, max_len, "...", false)) return 3;
    }
    if (pos + 2 <= max_len) {
        static const char *const mops2[] = {
            "==", "~=", "<=", ">=", "&&", "||", ".+", ".-", ".*", "./", ".\\", ".^", ".'", NULL
        };
        for (int k = 0; mops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, mops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '~' || c == '&' ||
        c == '|' || c == '+' || c == '-' || c == '*' || c == '/' ||
        c == '\\' || c == '^' || c == '@' || c == ':' || c == '.' ||
        c == ',' || c == ';') {
        return 1;
    }
    return 0;
}

bool _gen_matlab_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = matlab_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = matlab_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_matlab_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_matlab_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_matlab_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_matlab_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_matlab_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_matlab_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === R Matchers Implementation ===                                         */
/* ========================================================================= */

bool _gen_r_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_python_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static const char *const r_keywords[] = {
    "function", "continue", "return", "repeat", "while", "break", "else",
    "next", "for", "if", "in", NULL
};

static size_t r_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; r_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(r_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, r_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_' && after != '.') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_r_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = r_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = r_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t r_bool_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    static const char *const rbools[] = {
        "TRUE", "FALSE", "NULL", "NA", "Inf", "NaN", "NA_integer_",
        "NA_real_", "NA_complex_", "NA_character_", "T", "F", NULL
    };
    for (int k = 0; rbools[k] != NULL; k++) {
        size_t blen = strlen(rbools[k]);
        if (str_match_at(encoding, start, pos, max_len, rbools[k], false)) {
            if (pos + blen == max_len) return blen;
            uint32_t after = get_char_at(encoding, start, pos + blen);
            if (!is_alnum_codepoint(after) && after != '_' && after != '.') return blen;
        }
    }
    return 0;
}

bool _gen_r_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = r_bool_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = r_bool_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_r_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_r_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_SingleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_r_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_r_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_r_BacktickIdentifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_TemplateString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_r_BacktickIdentifier_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_TemplateString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_r_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_StringEscape_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t r_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 < max_len && get_char_at(encoding, start, pos) == '0') {
        uint32_t c1 = get_char_at(encoding, start, pos + 1);
        if (c1 == 'x' || c1 == 'X') {
            size_t i = pos + 2;
            while (i < max_len && (is_xdigit_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '.')) i++;
            if (i < max_len && (get_char_at(encoding, start, i) == 'L' || get_char_at(encoding, start, i) == 'i')) i++;
            if (i > pos + 2) return i - pos;
        }
    }
    if (is_digit_codepoint(get_char_at(encoding, start, pos)) ||
        (get_char_at(encoding, start, pos) == '.' && pos + 1 < max_len && is_digit_codepoint(get_char_at(encoding, start, pos + 1)))) {
        size_t i = pos;
        while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
        if (i < max_len && get_char_at(encoding, start, i) == '.') {
            i++;
            while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
        }
        if (i < max_len && (get_char_at(encoding, start, i) == 'e' || get_char_at(encoding, start, i) == 'E')) {
            size_t e_pos = i++;
            if (i < max_len && (get_char_at(encoding, start, i) == '+' || get_char_at(encoding, start, i) == '-')) i++;
            size_t ed = 0;
            while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) { i++; ed++; }
            if (ed == 0) i = e_pos;
        }
        if (i < max_len && (get_char_at(encoding, start, i) == 'L' || get_char_at(encoding, start, i) == 'i')) i++;
        return i - pos;
    }
    return 0;
}

bool _gen_r_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = r_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = r_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t r_var_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '.') {
        if (pos + 1 >= max_len) return 0;
        uint32_t c1 = get_char_at(encoding, start, pos + 1);
        if (!is_alpha_codepoint(c1) && c1 != '.' && c1 != '_') return 0;
    } else if (!is_alpha_codepoint(c)) {
        return 0;
    }
    size_t i = pos + 1;
    while (i < max_len) {
        uint32_t cc = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(cc) || cc == '.' || cc == '_') i++;
        else break;
    }
    return i - pos;
}

bool _gen_r_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = r_var_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = r_var_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t r_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    // check infix %...%
    if (get_char_at(encoding, start, pos) == '%') {
        size_t i = pos + 1;
        while (i < max_len) {
            uint32_t cc = get_char_at(encoding, start, i);
            if (cc == '%') return (i + 1) - pos;
            if (cc == '\r' || cc == '\n') break;
            i++;
        }
    }
    if (pos + 3 <= max_len) {
        if (str_match_at(encoding, start, pos, max_len, "<<-", false) ||
            str_match_at(encoding, start, pos, max_len, "->>", false)) return 3;
    }
    if (pos + 2 <= max_len) {
        static const char *const rops2[] = {
            ":::", "::", ":=", "->", "<-", "==", "!=", "<=", ">=", "&&", "||", NULL
        };
        for (int k = 0; rops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, rops2[k], false)) return strlen(rops2[k]);
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '&' ||
        c == '|' || c == '+' || c == '-' || c == '*' || c == '/' ||
        c == '^' || c == '~' || c == '$' || c == '@' || c == ':' ||
        c == '?' || c == ',' || c == ';') {
        return 1;
    }
    return 0;
}

bool _gen_r_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = r_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = r_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_r_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_r_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_r_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_r_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_r_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_r_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === JAI Matchers Implementation ===                                       */
/* ========================================================================= */

bool _gen_jai_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_jai_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_jai_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t jai_directive_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos + 1 >= max_len || get_char_at(encoding, start, pos) != '#') return 0;
    uint32_t c1 = get_char_at(encoding, start, pos + 1);
    if (!is_alpha_codepoint(c1) && c1 != '_') return 0;
    size_t i = pos + 2;
    while (i < max_len) {
        uint32_t c = get_char_at(encoding, start, i);
        if (is_alnum_codepoint(c) || c == '_') i++;
        else break;
    }
    return i - pos;
}

bool _gen_jai_Directive_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = jai_directive_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = jai_directive_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static const char *const jai_keywords[] = {
    "push_context", "no_check", "float64", "float32", "context", "continue",
    "inline", "return", "remove", "struct", "string", "defer", "float", "union",
    "using", "while", "break", "case", "cast", "enum", "else", "then", "temp",
    "trunc", "bool", "void", "Code", "Type", "for", "ifx", "int", "u16", "u32",
    "u64", "s16", "s32", "s64", "if", "xx", "u8", "s8", NULL
};

static size_t jai_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; jai_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(jai_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, jai_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_jai_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = jai_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = jai_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t jai_bool_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    static const char *const jbools[] = { "true", "false", "null", NULL };
    for (int k = 0; jbools[k] != NULL; k++) {
        size_t blen = strlen(jbools[k]);
        if (str_match_at(encoding, start, pos, max_len, jbools[k], false)) {
            if (pos + blen == max_len) return blen;
            uint32_t after = get_char_at(encoding, start, pos + blen);
            if (!is_alnum_codepoint(after) && after != '_') return blen;
        }
    }
    return 0;
}

bool _gen_jai_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = jai_bool_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = jai_bool_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_jai_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_jai_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_jai_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t jai_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 3 <= max_len) {
        static const char *const jops3[] = { "===", "<<=", ">>=", NULL };
        for (int k = 0; jops3[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, jops3[k], false)) return 3;
        }
    }
    if (pos + 2 <= max_len) {
        static const char *const jops2[] = {
            "::", ":=", "==", "!=", "<=", ">=", "&&", "||", "+=", "-=",
            "*=", "/=", "%=", "&=", "^=", "|=", "->", "<<", ">>", NULL
        };
        for (int k = 0; jops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, jops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '!' || c == '&' ||
        c == '|' || c == '^' || c == '~' || c == '+' || c == '-' ||
        c == '*' || c == '/' || c == '%' || c == '?' || c == ':' ||
        c == ';' || c == '.' || c == ',') {
        return 1;
    }
    return 0;
}

bool _gen_jai_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = jai_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = jai_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_jai_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_jai_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_jai_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_StringEscape_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_jai_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_Number_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_jai_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_jai_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_jai_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_jai_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === VB Matchers Implementation ===                                        */
/* ========================================================================= */

static size_t vb_line_comment_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '\'') {
        size_t i = pos + 1;
        while (i < max_len) {
            uint32_t cc = get_char_at(encoding, start, i);
            if (cc == '\r' || cc == '\n') break;
            i++;
        }
        return i - pos;
    }
    if (str_match_at(encoding, start, pos, max_len, "rem", true)) {
        if (pos + 3 < max_len) {
            uint32_t sp = get_char_at(encoding, start, pos + 3);
            if (is_space_codepoint(sp)) {
                size_t i = pos + 4;
                while (i < max_len) {
                    uint32_t cc = get_char_at(encoding, start, i);
                    if (cc == '\r' || cc == '\n') break;
                    i++;
                }
                return i - pos;
            }
        }
    }
    return 0;
}

bool _gen_vb_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = vb_line_comment_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = vb_line_comment_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static const char *const vb_keywords[] = {
    "notinheritable", "notoverridable", "removehandler", "mustoverride", "mustinherit",
    "addhandler", "addressof", "synclock", "widening", "narrowing", "paramarray",
    "withevents", "writeonly", "implements", "overloads", "overridable", "overrides",
    "structure", "protected", "continue", "delegate", "function", "inherits",
    "interface", "namespace", "readonly", "property", "raiseevent", "synclock",
    "andalso", "byref", "byval", "catch", "class", "const", "declare", "default",
    "elseif", "endif", "erase", "error", "event", "finally", "friend", "global",
    "gosub", "handles", "imports", "isnot", "mybase", "myclass", "nothing",
    "operator", "option", "optional", "orelse", "partial", "preserve", "private",
    "public", "redim", "resume", "return", "select", "shadows", "shared", "static",
    "typeof", "unicode", "alias", "call", "case", "each", "else", "exit", "goto",
    "like", "loop", "next", "step", "stop", "then", "throw", "when", "with", "and",
    "dim", "end", "for", "get", "lib", "mod", "new", "not", "off", "out", "rem",
    "set", "sub", "try", "xor", "as", "do", "if", "in", "is", "me", "of", "on",
    "or", "to", NULL
};

static size_t vb_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; vb_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(vb_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, vb_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_vb_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = vb_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = vb_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static const char *const vb_types[] = {
    "uinteger", "boolean", "decimal", "integer", "ushort", "double", "object",
    "single", "string", "ulong", "sbyte", "short", "cdate", "byte", "char",
    "date", "long", NULL
};

static size_t vb_datatype_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; vb_types[k] != NULL; k++) {
        size_t kw_len = strlen(vb_types[k]);
        if (str_match_at(encoding, start, pos, max_len, vb_types[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_vb_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = vb_datatype_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = vb_datatype_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_vb_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_ada_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

bool _gen_vb_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_vb_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_vb_EscapedQuote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_fortran_EscapedDoubleQuote_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t vb_number_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 < max_len && get_char_at(encoding, start, pos) == '&') {
        uint32_t c1 = get_char_at(encoding, start, pos + 1);
        if (c1 == 'h' || c1 == 'H') {
            size_t i = pos + 2;
            while (i < max_len && is_xdigit_codepoint(get_char_at(encoding, start, i))) i++;
            if (i > pos + 2) return i - pos;
        }
        if (c1 == 'o' || c1 == 'O') {
            size_t i = pos + 2;
            while (i < max_len) {
                uint32_t oc = get_char_at(encoding, start, i);
                if (oc >= '0' && oc <= '7') i++;
                else break;
            }
            if (i > pos + 2) return i - pos;
        }
    }
    if (is_digit_codepoint(get_char_at(encoding, start, pos))) {
        size_t i = pos + 1;
        while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
        if (i < max_len && get_char_at(encoding, start, i) == '.') {
            if (i + 1 < max_len && is_digit_codepoint(get_char_at(encoding, start, i + 1))) {
                i += 2;
                while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) i++;
            }
        }
        if (i < max_len && (get_char_at(encoding, start, i) == 'e' || get_char_at(encoding, start, i) == 'E')) {
            size_t e_pos = i++;
            if (i < max_len && (get_char_at(encoding, start, i) == '+' || get_char_at(encoding, start, i) == '-')) i++;
            size_t ed = 0;
            while (i < max_len && is_digit_codepoint(get_char_at(encoding, start, i))) { i++; ed++; }
            if (ed == 0) i = e_pos;
        }
        if (i < max_len) {
            uint32_t suf = get_char_at(encoding, start, i);
            if (suf == '!' || suf == '#' || suf == '%' || suf == '&' || suf == '@') i++;
        }
        return i - pos;
    }
    return 0;
}

bool _gen_vb_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = vb_number_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = vb_number_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_vb_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Variable_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t vb_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (pos + 2 <= max_len) {
        static const char *const vbops2[] = {
            "<<", ">>", "<=", ">=", "<>", ":=", "+=", "-=", NULL
        };
        for (int k = 0; vbops2[k] != NULL; k++) {
            if (str_match_at(encoding, start, pos, max_len, vbops2[k], false)) return 2;
        }
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '+' || c == '-' ||
        c == '*' || c == '/' || c == '\\' || c == '^' || c == '&' ||
        c == '.' || c == ',' || c == ';' || c == ':') {
        return 1;
    }
    return 0;
}

bool _gen_vb_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = vb_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = vb_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_vb_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_vb_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_vb_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_vb_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_vb_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_vb_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* ========================================================================= */
/* === SCRATCH Matchers Implementation ===                                   */
/* ========================================================================= */

bool _gen_scratch_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_php_LineComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static const char *const scratch_keywords[] = {
    "direction", "broadcast", "message1", "touching", "distance", "contains",
    "backdrop", "clicked", "degrees", "towards", "graphic", "seconds",
    "costume", "message", "loudness", "between", "replace", "counter",
    "pressed", "effect", "forever", "answer", "random", "length", "sprite",
    "script", "create", "delete", "repeat", "switch", "insert", "green",
    "space", "steps", "glide", "point", "clear", "sound", "until", "timer",
    "reset", "round", "color", "when", "flag", "turn", "secs", "size",
    "show", "hide", "think", "wait", "next", "play", "stop", "then", "else",
    "join", "sqrt", "item", "list", "key", "any", "move", "set", "say",
    "all", "this", "for", "new", "ask", "end", "not", "mod", "add",
    "cw", "ccw", "go", "to", "if", "or", "of", "x", "y", "down", "pick",
    "letter", "abs", "and", NULL
};

static size_t scratch_keyword_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len, bool caseless) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    for (int k = 0; scratch_keywords[k] != NULL; k++) {
        size_t kw_len = strlen(scratch_keywords[k]);
        if (str_match_at(encoding, start, pos, max_len, scratch_keywords[k], caseless)) {
            if (pos + kw_len == max_len) return kw_len;
            uint32_t after = get_char_at(encoding, start, pos + kw_len);
            if (!is_alnum_codepoint(after) && after != '_') {
                return kw_len;
            }
        }
    }
    return 0;
}

bool _gen_scratch_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    if (only_at_start) {
        size_t m = scratch_keyword_match_at(encoding, start, 0, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = scratch_keyword_match_at(encoding, start, pos, max_len, is_caseless);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_scratch_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Boolean_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_scratch_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_scratch_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_DoubleString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_scratch_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_php_Number_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

static size_t scratch_var_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    if (get_char_at(encoding, start, pos) == '[') {
        if (pos + 2 >= max_len) return 0;
        uint32_t c1 = get_char_at(encoding, start, pos + 1);
        if (!is_alpha_codepoint(c1) && c1 != '_') return 0;
        size_t i = pos + 2;
        while (i < max_len && (is_alnum_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) i++;
        if (i < max_len && get_char_at(encoding, start, i) == ']') return (i + 1) - pos;
        return 0;
    }
    uint32_t c = get_char_at(encoding, start, pos);
    if (!is_alpha_codepoint(c) && c != '_') return 0;
    size_t i = pos + 1;
    while (i < max_len && (is_alnum_codepoint(get_char_at(encoding, start, i)) || get_char_at(encoding, start, i) == '_')) i++;
    return i - pos;
}

bool _gen_scratch_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = scratch_var_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = scratch_var_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

static size_t scratch_op_match_at(enum textparser_encoding encoding, const void *start, size_t pos, size_t max_len) {
    if (pos >= max_len) return 0;
    uint32_t c = get_char_at(encoding, start, pos);
    if (c == '=' || c == '<' || c == '>' || c == '+' || c == '-' || c == '*' || c == '/') {
        return 1;
    }
    return 0;
}

bool _gen_scratch_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        size_t m = scratch_op_match_at(encoding, start, 0, max_len);
        if (m > 0) {
            if (offset) *offset = 0;
            if (length) *length = m;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos < max_len; pos++) {
        size_t m = scratch_op_match_at(encoding, start, pos, max_len);
        if (m > 0) {
            if (offset) *offset = pos;
            if (length) *length = m;
            return true;
        }
    }
    return false;
}

bool _gen_scratch_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_scratch_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_Parenthesis_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_scratch_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_scratch_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_ArrayIndex_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_scratch_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_scratch_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* === Missing Batch 4 Matchers === */

// r BacktickString
bool _gen_r_BacktickString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_TemplateString_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_r_BacktickString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_javascript_TemplateString_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// matlab StringEscape: ''|""
bool _gen_matlab_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    (void)is_caseless;
    if (only_at_start) {
        if (str_match_at(encoding, start, 0, max_len, "''", false) ||
            str_match_at(encoding, start, 0, max_len, "\"\"", false)) {
            if (offset) *offset = 0;
            if (length) *length = 2;
            return true;
        }
        return false;
    }
    for (size_t pos = 0; pos + 1 < max_len; pos++) {
        if (str_match_at(encoding, start, pos, max_len, "''", false) ||
            str_match_at(encoding, start, pos, max_len, "\"\"", false)) {
            if (offset) *offset = pos;
            if (length) *length = 2;
            return true;
        }
    }
    return false;
}

// asm BlockComment
bool _gen_asm_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_asm_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_BlockComment_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// asm Keyword
bool _gen_asm_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_asm_Instruction_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

// asm CodeBlock
bool _gen_asm_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_start(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
bool _gen_asm_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start)
{
    return _gen_c_CodeBlock_end(encoding, start, max_len, offset, length, is_caseless, only_at_start);
}
