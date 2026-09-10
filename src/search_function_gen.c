#include "search_function_gen.h"
#include "adv_regex.h"

static bool native_regex_match(const char *pattern, enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start, adv_regex_context **ctx_slot, void **regex_slot, int *encoding_slot) {
    if (!*ctx_slot) *ctx_slot = adv_regex_context_create();
    if (!*ctx_slot) return false;
    if (*encoding_slot != (int)encoding) {
        if (*regex_slot) adv_regex_free(*ctx_slot, regex_slot, (enum textparser_encoding)*encoding_slot);
        *encoding_slot = (int)encoding;
    }
    return adv_regex_find_pattern_ctx(*ctx_slot, pattern, regex_slot, encoding, start, max_len, offset, length, is_caseless, only_at_start);
}

/* _gen_ada_LineComment_start */
bool _gen_ada_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("--[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_Keyword_start */
bool _gen_ada_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("abort\\b|abs\\b|abstract\\b|accept\\b|access\\b|aliased\\b|all\\b|and\\b|array\\b|at\\b|begin\\b|body\\b|case\\b|constant\\b|declare\\b|delay\\b|delta\\b|digits\\b|do\\b|else\\b|elsif\\b|end\\b|entry\\b|exception\\b|exit\\b|for\\b|function\\b|generic\\b|goto\\b|if\\b|in\\b|interface\\b|is\\b|limited\\b|loop\\b|mod\\b|new\\b|not\\b|null\\b|of\\b|or\\b|others\\b|out\\b|overriding\\b|package\\b|pragma\\b|private\\b|procedure\\b|protected\\b|raise\\b|range\\b|record\\b|rem\\b|renames\\b|requeue\\b|return\\b|reverse\\b|select\\b|separate\\b|some\\b|subtype\\b|synchronized\\b|tagged\\b|task\\b|terminate\\b|then\\b|type\\b|until\\b|use\\b|when\\b|while\\b|with\\b|xor\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_DataType_start */
bool _gen_ada_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("integer\\b|float\\b|character\\b|boolean\\b|string\\b|natural\\b|positive\\b|long_integer\\b|long_float\\b|short_integer\\b|short_float\\b|wide_character\\b|wide_string\\b|duration\\b|address\\b|count\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_Boolean_start */
bool _gen_ada_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_CharLiteral_start */
bool _gen_ada_CharLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'.'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_SingleString_start */
bool _gen_ada_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_SingleString_end */
bool _gen_ada_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_EscapedQuote_start */
bool _gen_ada_EscapedQuote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_Number_start */
bool _gen_ada_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[0-9]+(?:#[0-9a-fA-F]+(?:\\.[0-9a-fA-F]+)?#)?(?:\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_Attribute_start */
bool _gen_ada_Attribute_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_Variable_start */
bool _gen_ada_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_Operator_start */
bool _gen_ada_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(":=|=>|\\.\\.|<>|>=|<=|\\*\\*|/=|[=<>+\\-*/&.,;:]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_Parenthesis_start */
bool _gen_ada_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_Parenthesis_end */
bool _gen_ada_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_ArrayIndex_start */
bool _gen_ada_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_ArrayIndex_end */
bool _gen_ada_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_CodeBlock_start */
bool _gen_ada_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_ada_CodeBlock_end */
bool _gen_ada_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_LineComment_start */
bool _gen_asm_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(";[^\\r\\n]*|\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_BlockComment_start */
bool _gen_asm_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_BlockComment_end */
bool _gen_asm_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_Keyword_start */
bool _gen_asm_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("mov\\b|add\\b|sub\\b|mul\\b|div\\b|imul\\b|idiv\\b|inc\\b|dec\\b|neg\\b|cmp\\b|test\\b|and\\b|or\\b|xor\\b|not\\b|shl\\b|shr\\b|sal\\b|sar\\b|rol\\b|ror\\b|rcl\\b|rcr\\b|push\\b|pop\\b|pushf\\b|popf\\b|pusha\\b|popa\\b|call\\b|ret\\b|retf\\b|iretd\\b|int\\b|into\\b|syscall\\b|sysenter\\b|jmp\\b|je\\b|jne\\b|jz\\b|jnz\\b|jg\\b|jl\\b|jge\\b|jle\\b|ja\\b|jb\\b|jae\\b|jbe\\b|jo\\b|jno\\b|js\\b|jns\\b|jp\\b|jnp\\b|jcxz\\b|jecxz\\b|loop\\b|loope\\b|loopne\\b|nop\\b|hlt\\b|cli\\b|sti\\b|cld\\b|std\\b|cmc\\b|clc\\b|stc\\b|movsb\\b|movsw\\b|movsd\\b|cmpsb\\b|cmpsw\\b|cmpsd\\b|scasb\\b|scasw\\b|scasd\\b|lodsb\\b|lodsw\\b|lodsd\\b|stosb\\b|stosw\\b|stosd\\b|lea\\b|movzx\\b|movsx\\b|cwde\\b|cdq\\b|cqo\\b|xchg\\b|bswap\\b|bt\\b|bts\\b|btr\\b|btc\\b|bsf\\b|bsr\\b|sete\\b|setne\\b|setg\\b|setl\\b|setge\\b|setle\\b|seta\\b|setb\\b|setae\\b|setbe\\b|cpuid\\b|rdtsc\\b|rdmsr\\b|wrmsr\\b|lgdt\\b|lidt\\b|sgdt\\b|sidt\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_Directive_start */
bool _gen_asm_Directive_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\.[a-zA-Z_][a-zA-Z0-9_]*|section\\b|segment\\b|proc\\b|endp\\b|assume\\b|model\\b|stack\\b|data\\b|code\\b|public\\b|extern\\b|global\\b|align\\b|org\\b|equ\\b|=\\b|db\\b|dw\\b|dd\\b|dq\\b|dt\\b|resb\\b|resw\\b|resd\\b|resq\\b|incbin\\b|macro\\b|endm\\b|local\\b|rept\\b|irp\\b|irpc\\b|exitm\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_Register_start */
bool _gen_asm_Register_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[re]?[abcd]x\\b|[re]?[bs]p\\b|[re]?[bs]i\\b|[re]?[di]p\\b|r[0-9]+\\b|e?[abcd]x\\b|e?[bs]p\\b|e?[bs]i\\b|e?[di]p\\b|r[0-9]+[dw]?\\b|st\\([0-9]+\\)\\b|mm[0-7]\\b|xmm[0-9]+\\b|ymm[0-9]+\\b|zmm[0-9]+\\b|cr[0-8]\\b|dr[0-7]\\b|cs\\b|ds\\b|es\\b|fs\\b|gs\\b|ss\\b|ax\\b|bx\\b|cx\\b|dx\\b|si\\b|di\\b|bp\\b|sp\\b|ip\\b|ah\\b|al\\b|bh\\b|bl\\b|ch\\b|cl\\b|dh\\b|dl\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_Number_start */
bool _gen_asm_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("0[xX][0-9a-fA-F]+\\b|0[oOqQ][0-7]+\\b|0[bByY][01]+\\b|[0-9]+(?:\\.[0-9]+)?\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_SingleString_start */
bool _gen_asm_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_SingleString_end */
bool _gen_asm_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_Variable_start */
bool _gen_asm_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_Operator_start */
bool _gen_asm_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\+|-|\\*|\\/|,|:|;", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_Parenthesis_start */
bool _gen_asm_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_Parenthesis_end */
bool _gen_asm_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_ArrayIndex_start */
bool _gen_asm_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_ArrayIndex_end */
bool _gen_asm_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_CodeBlock_start */
bool _gen_asm_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_asm_CodeBlock_end */
bool _gen_asm_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_LineComment_start */
bool _gen_bash_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_Keyword_start */
bool _gen_bash_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("if\\b|then\\b|else\\b|elif\\b|fi\\b|for\\b|while\\b|until\\b|do\\b|done\\b|in\\b|case\\b|esac\\b|select\\b|function\\b|time\\b|coproc\\b|alias\\b|bg\\b|bind\\b|break\\b|builtin\\b|caller\\b|cd\\b|command\\b|compgen\\b|complete\\b|compopt\\b|continue\\b|declare\\b|dirs\\b|disown\\b|echo\\b|enable\\b|eval\\b|exec\\b|exit\\b|export\\b|fc\\b|fg\\b|getopts\\b|hash\\b|help\\b|history\\b|jobs\\b|kill\\b|let\\b|local\\b|mapfile\\b|popd\\b|printf\\b|pushd\\b|pwd\\b|read\\b|readarray\\b|readonly\\b|return\\b|set\\b|shift\\b|shopt\\b|source\\b|suspend\\b|test\\b|times\\b|trap\\b|type\\b|typeset\\b|ulimit\\b|umask\\b|unalias\\b|unset\\b|wait\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_Boolean_start */
bool _gen_bash_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_ArithmeticExpression_start */
bool _gen_bash_ArithmeticExpression_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\$\\(\\(|\\(\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_ArithmeticExpression_end */
bool _gen_bash_ArithmeticExpression_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_CommandSubstitution_start */
bool _gen_bash_CommandSubstitution_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\$\\((?!\\()", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_CommandSubstitution_end */
bool _gen_bash_CommandSubstitution_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_ProcessSubstitution_start */
bool _gen_bash_ProcessSubstitution_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<\\(|>\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_ProcessSubstitution_end */
bool _gen_bash_ProcessSubstitution_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_BacktickSubstitution_start */
bool _gen_bash_BacktickSubstitution_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_BacktickSubstitution_end */
bool _gen_bash_BacktickSubstitution_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_TestExpression_start */
bool _gen_bash_TestExpression_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_TestExpression_end */
bool _gen_bash_TestExpression_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_Variable_start */
bool _gen_bash_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\$[a-zA-Z_][a-zA-Z0-9_]*|\\$[@*#?$!_0-9-]|[a-zA-Z_][a-zA-Z0-9_]*(?=\\+=|=|:-|:=|:\\+|:\\?)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_Path_start */
bool _gen_bash_Path_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z0-9_.~-]*\\/[a-zA-Z0-9_.~/-]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_Identifier_start */
bool _gen_bash_Identifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_ParameterExpansion_start */
bool _gen_bash_ParameterExpansion_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\$\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_ParameterExpansion_end */
bool _gen_bash_ParameterExpansion_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_CodeBlock_start */
bool _gen_bash_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_CodeBlock_end */
bool _gen_bash_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_Operator_start */
bool _gen_bash_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("&&|\\|\\||>>|<<|==|!=|\\+=|\\-=|\\*=|/=|%=|&=|\\|=|\\^=|<<=|>>=|\\+\\+|\\-\\-|;;|;&|;;&|\\|&|<<<|<<-|>&|<&|&>>|&>|>\\||<>|<=|>=|=~|:-|:=|:\\+|:\\?|//|##|%%|\\^\\^|,,|[-+*/%&|^~<>!=;.,|:]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_ArithmeticOperator_start */
bool _gen_bash_ArithmeticOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\*|<<=|>>=|\\+=|\\-=|\\*=|/=|%=|&=|\\|=|\\^=|\\+\\+|\\-\\-|<<|>>|<=|>=|==|!=|&&|\\|\\||[-+*/%&|^~<>!=?:=]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_SingleString_start */
bool _gen_bash_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\$?'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_SingleString_end */
bool _gen_bash_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_DoubleString_start */
bool _gen_bash_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\$?\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_DoubleString_end */
bool _gen_bash_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_StringEscape_start */
bool _gen_bash_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t|\\\\\\$|\\\\\\`|\\\\a|\\\\b|\\\\[eE]|\\\\f|\\\\v|\\\\\\?|\\\\x[0-9a-fA-F]{1,2}|\\\\u[0-9a-fA-F]{1,4}|\\\\U[0-9a-fA-F]{1,8}|\\\\c[a-zA-Z]|\\\\[0-7]{1,3}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_Number_start */
bool _gen_bash_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("0[xX][0-9a-fA-F]+|0[oO][0-7]+|0[bB][01]+|[0-9]+#[0-9a-zA-Z@_]+|[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_Parenthesis_start */
bool _gen_bash_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_Parenthesis_end */
bool _gen_bash_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_ArrayIndex_start */
bool _gen_bash_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_bash_ArrayIndex_end */
bool _gen_bash_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_LineComment_start */
bool _gen_c3_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_BlockComment_start */
bool _gen_c3_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_BlockComment_end */
bool _gen_c3_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_DocComment_start */
bool _gen_c3_DocComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\<\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_DocComment_end */
bool _gen_c3_DocComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_Directive_start */
bool _gen_c3_Directive_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\$[a-zA-Z_][a-zA-Z0-9_]*\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_Annotation_start */
bool _gen_c3_Annotation_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("@[a-zA-Z_][a-zA-Z0-9_]*\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_Keyword_start */
bool _gen_c3_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("break\\b|case\\b|continue\\b|default\\b|defer\\b|do\\b|else\\b|for\\b|foreach\\b|foreach_r\\b|if\\b|nextcase\\b|return\\b|switch\\b|while\\b|assert\\b|asm\\b|catch\\b|inline\\b|import\\b|module\\b|interface\\b|try\\b|var\\b|const\\b|extern\\b|static\\b|tlocal\\b|fn\\b|typedef\\b|struct\\b|union\\b|enum\\b|faultdef\\b|attrdef\\b|constdef\\b|void\\b|bool\\b|char\\b|double\\b|float\\b|float16\\b|bfloat\\b|int128\\b|ichar\\b|int\\b|iptr\\b|isz\\b|sz\\b|long\\b|short\\b|uint128\\b|uint\\b|ulong\\b|uptr\\b|ushort\\b|usz\\b|float128\\b|any\\b|fault\\b|typeid\\b|untypedlist\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_Boolean_start */
bool _gen_c3_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b|null\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_Variable_start */
bool _gen_c3_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_CodeBlock_start */
bool _gen_c3_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_CodeBlock_end */
bool _gen_c3_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_Operator_start */
bool _gen_c3_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("::|:=|:|===|==|!=|<=|>=|&&|\\|\\||\\+=|\\-=|\\*=|\\/=|%=|&=|\\^=|\\|=|<<=|>>=|->|<<|>>|\\.\\.|[=<>!&|^~+\\-*/%\\?:;.,]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_DoubleString_start */
bool _gen_c3_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_DoubleString_end */
bool _gen_c3_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_SingleString_start */
bool _gen_c3_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_SingleString_end */
bool _gen_c3_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_StringEscape_start */
bool _gen_c3_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t|\\\\u[0-9a-fA-F]{4}|\\\\x[0-9a-fA-F]{2}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_Number_start */
bool _gen_c3_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("0[xX][0-9a-fA-F_]+\\b|0[bB][01_]+\\b|0[oO][0-7_]+\\b|[0-9_]*\\.?[0-9_]+(?:[eE][-+]?[0-9_]+)?\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_Parenthesis_start */
bool _gen_c3_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_Parenthesis_end */
bool _gen_c3_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_ArrayIndex_start */
bool _gen_c3_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c3_ArrayIndex_end */
bool _gen_c3_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_LineComment_start */
bool _gen_c_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_BlockComment_start */
bool _gen_c_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_BlockComment_end */
bool _gen_c_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_Preprocessor_start */
bool _gen_c_Preprocessor_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#[ \\t]*[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_Attribute_start */
bool _gen_c_Attribute_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_Attribute_end */
bool _gen_c_Attribute_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_Keyword_start */
bool _gen_c_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("typedef\\b|const\\b|static\\b|extern\\b|volatile\\b|if\\b|else\\b|for\\b|while\\b|do\\b|switch\\b|case\\b|default\\b|break\\b|continue\\b|return\\b|sizeof\\b|goto\\b|register\\b|auto\\b|inline\\b|restrict\\b|constexpr\\b|nullptr\\b|typeof\\b|typeof_unqual\\b|static_assert\\b|thread_local\\b|alignas\\b|alignof\\b|_Alignas\\b|_Alignof\\b|_Atomic\\b|_Generic\\b|_Noreturn\\b|_Static_assert\\b|_Thread_local\\b|_Decimal32\\b|_Decimal64\\b|_Decimal128\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_Function_start */
bool _gen_c_Function_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("([a-zA-Z_][a-zA-Z0-9_]*)[\\s]*\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_Variable_start */
bool _gen_c_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_CodeBlock_start */
bool _gen_c_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_CodeBlock_end */
bool _gen_c_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_Operator_start */
bool _gen_c_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\+=|\\-=|\\*=|\\/=|%=|&=|\\^=|\\|=|<{2}=|>{2}=|<{2}|>{2}|\\+\\+|\\-\\-|&&|\\|\\||<=|>=|==|!=|->|\\.|[=<>!&|^~+\\-*/%\\?:;.,]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_SingleString_start */
bool _gen_c_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(?:u8|u|U|L)?'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_SingleString_end */
bool _gen_c_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_DoubleString_start */
bool _gen_c_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(?:u8|u|U|L)?\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_DoubleString_end */
bool _gen_c_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_FormatSpecifier_start */
bool _gen_c_FormatSpecifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("%(?:%|(?:[1-9][0-9]*\\$)?[-+ #0']*(?:\\*(?:[1-9][0-9]*\\$)?|[0-9]+)?(?:\\.(?:\\*(?:[1-9][0-9]*\\$)?|[0-9]*))?(?:hh|ll|[hljztL])?[diouxXfFeEgGaAcspnbB])", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_StringEscape_start */
bool _gen_c_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t|\\\\v|\\\\f|\\\\a|\\\\b|\\\\e|\\\\0|\\\\u[0-9a-fA-F]{4}|\\\\U[0-9a-fA-F]{8}|\\\\x[0-9a-fA-F]+|\\[0-7]{1,3}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_Number_start */
bool _gen_c_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("0[xX][0-9a-fA-F](?:'?[0-9a-fA-F])*(?:\\.[0-9a-fA-F](?:'?[0-9a-fA-F])*)?(?:[pP][+-]?[0-9](?:'?[0-9])*)?[uUlLzZwWbBfFdD]*|0[bB][01](?:'?[01])*[uUlLzZwWbB]*|[0-9](?:'?[0-9])*(?:\\.[0-9](?:'?[0-9])*)?(?:[eE][+-]?[0-9](?:'?[0-9])*)?[uUlLzZwWbBfFdD]*|\\.[0-9](?:'?[0-9])*(?:[eE][+-]?[0-9](?:'?[0-9])*)?[uUlLzZwWbBfFdD]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_Boolean_start */
bool _gen_c_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_Parenthesis_start */
bool _gen_c_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_Parenthesis_end */
bool _gen_c_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_ArrayIndex_start */
bool _gen_c_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_ArrayIndex_end */
bool _gen_c_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_TypeCast_start */
bool _gen_c_TypeCast_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_TypeCast_end */
bool _gen_c_TypeCast_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_DataType_start */
bool _gen_c_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("int\\b|char\\b|double\\b|float\\b|void\\b|short\\b|long\\b|unsigned\\b|signed\\b|_Bool\\b|bool\\b|nullptr_t\\b|_BitInt\\b|size_t\\b|ssize_t\\b|uint8_t\\b|uint16_t\\b|uint32_t\\b|uint64_t\\b|int8_t\\b|int16_t\\b|int32_t\\b|int64_t\\b|uintptr_t\\b|intptr_t\\b|ptrdiff_t\\b|intmax_t\\b|uintmax_t\\b|char8_t\\b|char16_t\\b|char32_t\\b|wchar_t\\b|_Complex\\b|_Imaginary\\b|[a-zA-Z_][a-zA-Z0-9_]*_(?:t|type|enum|e|s)\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_TypeName_start */
bool _gen_c_TypeName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_c_TagSpecifier_start */
bool _gen_c_TagSpecifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("struct\\b|union\\b|enum\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_ScriptStartTag_start */
bool _gen_cfml_ScriptStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<cfscript(?=[\\>\\s])", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_ScriptStartTag_end */
bool _gen_cfml_ScriptStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/?>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_ScriptEndTag_start */
bool _gen_cfml_ScriptEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<\\/cfscript>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_OutputStartTag_start */
bool _gen_cfml_OutputStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<cfoutput(?=[\\>\\s])", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_OutputStartTag_end */
bool _gen_cfml_OutputStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/?>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_OutputEndTag_start */
bool _gen_cfml_OutputEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<\\/cfoutput>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_QueryStartTag_start */
bool _gen_cfml_QueryStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<cfquery(?=[\\>\\s])", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_QueryStartTag_end */
bool _gen_cfml_QueryStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/?>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_QueryEndTag_start */
bool _gen_cfml_QueryEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<\\/cfquery>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_LoopStartTag_start */
bool _gen_cfml_LoopStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<cfloop(?=[\\>\\s])", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_LoopStartTag_end */
bool _gen_cfml_LoopStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/?>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_LoopEndTag_start */
bool _gen_cfml_LoopEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<\\/cfloop>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_MailStartTag_start */
bool _gen_cfml_MailStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<cfmail(?=[\\>\\s])", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_MailStartTag_end */
bool _gen_cfml_MailStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/?>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_MailEndTag_start */
bool _gen_cfml_MailEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<\\/cfmail>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_SavecontentStartTag_start */
bool _gen_cfml_SavecontentStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<cfsavecontent(?=[\\>\\s])", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_SavecontentStartTag_end */
bool _gen_cfml_SavecontentStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/?>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_SavecontentEndTag_start */
bool _gen_cfml_SavecontentEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<\\/cfsavecontent>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_StartTag_start */
bool _gen_cfml_StartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<(?:cf[a-z0-9_]*|[a-z0-9_]+:[a-z0-9_]+)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_StartTag_end */
bool _gen_cfml_StartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/?>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_EndTag_start */
bool _gen_cfml_EndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<\\/(?:cf(?!(?:output|script|query|loop|mail|savecontent)\\b)[a-z0-9_]*|[a-z0-9_]+:[a-z0-9_]+)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_EndTag_end */
bool _gen_cfml_EndTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/?>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_Comment_start */
bool _gen_cfml_Comment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<!---", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_Comment_end */
bool _gen_cfml_Comment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("--->", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_SingleString_start */
bool _gen_cfml_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_SingleString_end */
bool _gen_cfml_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_DoubleString_start */
bool _gen_cfml_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_DoubleString_end */
bool _gen_cfml_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_SingleChar_start */
bool _gen_cfml_SingleChar_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("''", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_DoubleChar_start */
bool _gen_cfml_DoubleChar_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_SharpChar_start */
bool _gen_cfml_SharpChar_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("##", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_SharpExpression_start */
bool _gen_cfml_SharpExpression_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_SharpExpression_end */
bool _gen_cfml_SharpExpression_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_ScriptBlockComment_start */
bool _gen_cfml_ScriptBlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_ScriptBlockComment_end */
bool _gen_cfml_ScriptBlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_ScriptLineComment_start */
bool _gen_cfml_ScriptLineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_ExpressionEnd_start */
bool _gen_cfml_ExpressionEnd_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(";", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_Number_start */
bool _gen_cfml_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("([0-9]+\\.?[0-9]*(?:e[-+]?[0-9]+)?|[0-9]*\\.[0-9]+(?:e[-+]?[0-9]+)?)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_Boolean_start */
bool _gen_cfml_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b|yes\\b|no\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_ObjectMember_start */
bool _gen_cfml_ObjectMember_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\?\\.|\\.", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_Function_start */
bool _gen_cfml_Function_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("([a-z_]+[a-z0-9_]*)[\\s]*\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_Separator_start */
bool _gen_cfml_Separator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(",", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_SpreadOperator_start */
bool _gen_cfml_SpreadOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\.\\.\\.", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_Variable_start */
bool _gen_cfml_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-z_\\$]+[a-z0-9_\\$]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_LambdaOperator_start */
bool _gen_cfml_LambdaOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("=>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_AssignOperator_start */
bool _gen_cfml_AssignOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\+=|\\-=|\\*=|\\/=|%=|&=|=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_TernaryOperator_start */
bool _gen_cfml_TernaryOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\?\\?|\\?:|\\?|\\:", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_LogicalImpOperator_start */
bool _gen_cfml_LogicalImpOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("imp\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_LogicalEqvOperator_start */
bool _gen_cfml_LogicalEqvOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("eqv\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_LogicalXorOperator_start */
bool _gen_cfml_LogicalXorOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("xor\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_LogicalOrOperator_start */
bool _gen_cfml_LogicalOrOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\|\\||or\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_LogicalAndOperator_start */
bool _gen_cfml_LogicalAndOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\&\\&|and\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_LogicalNotOperator_start */
bool _gen_cfml_LogicalNotOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("not\\b|!", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_CompareOperator_start */
bool _gen_cfml_CompareOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("greater\\s+than\\s+or\\s+equal\\s+to\\b|less\\s+than\\s+or\\s+equal\\s+to\\b|does\\s+not\\s+contain\\b|is\\s+not\\b|contains\\b|less\\s+than\\b|greater\\s+than\\b|not\\s+equal\\b|equal\\b|neq\\b|lte\\b|gte\\b|eq\\b|===|!==|==|>=|<=|!=|ge\\b|lt\\b|gt\\b|le\\b|\\bis\\b|>|<", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_ConcatOperator_start */
bool _gen_cfml_ConcatOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\&", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_IncDecOperator_start */
bool _gen_cfml_IncDecOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\+\\+|\\-\\-", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_AddOperator_start */
bool _gen_cfml_AddOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\+|-", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_MulOperator_start */
bool _gen_cfml_MulOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*|\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_IntegerDivOperator_start */
bool _gen_cfml_IntegerDivOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_ModOperator_start */
bool _gen_cfml_ModOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("mod\\b|\\%", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_PowerOperator_start */
bool _gen_cfml_PowerOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\^", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_CodeBlock_start */
bool _gen_cfml_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_CodeBlock_end */
bool _gen_cfml_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_Keyword_start */
bool _gen_cfml_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("var\\b|function\\b|new\\b|this\\b|super\\b|null\\b|try\\b|catch\\b|finally\\b|if\\b|then\\b|else\\b|switch\\b|case\\b|default\\b|break\\b|continue\\b|while\\b|do\\b|for\\b|in\\b|return\\b|throw\\b|rethrow\\b|retry\\b|component\\b|interface\\b|property\\b|pageencoding\\b|import\\b|include\\b|param\\b|lock\\b|transaction\\b|thread\\b|public\\b|private\\b|remote\\b|package\\b|static\\b|final\\b|abstract\\b|required\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_Parenthesis_start */
bool _gen_cfml_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_Parenthesis_end */
bool _gen_cfml_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_ArrayIndex_start */
bool _gen_cfml_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cfml_ArrayIndex_end */
bool _gen_cfml_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_LineComment_start */
bool _gen_cpp_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_BlockComment_start */
bool _gen_cpp_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_BlockComment_end */
bool _gen_cpp_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_Preprocessor_start */
bool _gen_cpp_Preprocessor_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#[ \\t]*[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_Keyword_start */
bool _gen_cpp_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("namespace\\b|template\\b|typename\\b|using\\b|public\\b|private\\b|protected\\b|virtual\\b|override\\b|final\\b|friend\\b|operator\\b|this\\b|new\\b|delete\\b|throw\\b|catch\\b|try\\b|constexpr\\b|consteval\\b|constinit\\b|decltype\\b|explicit\\b|export\\b|import\\b|module\\b|mutable\\b|noexcept\\b|nullptr\\b|static_cast\\b|dynamic_cast\\b|const_cast\\b|reinterpret_cast\\b|thread_local\\b|concept\\b|requires\\b|co_await\\b|co_return\\b|co_yield\\b|typedef\\b|const\\b|static\\b|extern\\b|volatile\\b|if\\b|else\\b|for\\b|while\\b|do\\b|switch\\b|case\\b|default\\b|break\\b|continue\\b|return\\b|sizeof\\b|goto\\b|register\\b|auto\\b|inline\\b|restrict\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_Variable_start */
bool _gen_cpp_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_CodeBlock_start */
bool _gen_cpp_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_CodeBlock_end */
bool _gen_cpp_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_ScopeResolution_start */
bool _gen_cpp_ScopeResolution_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("::", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_Operator_start */
bool _gen_cpp_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\+=|\\-=|\\*=|\\/=|%=|&=|\\^=|\\|=|<{2}=|>{2}=|<{2}|>{2}|\\+\\+|\\-\\-|&&|\\|\\||<=>|<=|>=|==|!=|->\\*|->|\\.\\*|\\.|[=!|^~+\\-/%\\?:;.]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_PointerOrRef_start */
bool _gen_cpp_PointerOrRef_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[*&]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_TemplateOpen_start */
bool _gen_cpp_TemplateOpen_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_TemplateClose_start */
bool _gen_cpp_TemplateClose_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(">", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_Comma_start */
bool _gen_cpp_Comma_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(",", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_SingleString_start */
bool _gen_cpp_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_SingleString_end */
bool _gen_cpp_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_DoubleString_start */
bool _gen_cpp_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_DoubleString_end */
bool _gen_cpp_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_StringEscape_start */
bool _gen_cpp_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t|\\\\u[0-9a-fA-F]{4}|\\\\x[0-9a-fA-F]{2}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_Number_start */
bool _gen_cpp_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(?:0[xX][0-9a-fA-F]+[uUlL]*|0[bB][01]+[uUlL]*|[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?[fFdDlL]?)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_Boolean_start */
bool _gen_cpp_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_Parenthesis_start */
bool _gen_cpp_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_Parenthesis_end */
bool _gen_cpp_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_ArrayIndex_start */
bool _gen_cpp_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_ArrayIndex_end */
bool _gen_cpp_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_TemplateGroup_start */
bool _gen_cpp_TemplateGroup_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_TemplateGroup_end */
bool _gen_cpp_TemplateGroup_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(">", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_TypeCast_start */
bool _gen_cpp_TypeCast_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_TypeCast_end */
bool _gen_cpp_TypeCast_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_DataType_start */
bool _gen_cpp_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("char8_t\\b|char16_t\\b|char32_t\\b|wchar_t\\b|int\\b|char\\b|double\\b|float\\b|void\\b|short\\b|long\\b|unsigned\\b|signed\\b|_Bool\\b|bool\\b|size_t\\b|ssize_t\\b|uint8_t\\b|uint16_t\\b|uint32_t\\b|uint64_t\\b|int8_t\\b|int16_t\\b|int32_t\\b|int64_t\\b|uintptr_t\\b|intptr_t\\b|ptrdiff_t\\b|[a-zA-Z_][a-zA-Z0-9_]*_(?:t|type|enum|e|s)\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_TagSpecifier_start */
bool _gen_cpp_TagSpecifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("struct\\b|union\\b|enum\\b|class\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_cpp_TypeName_start */
bool _gen_cpp_TypeName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_LineComment_start */
bool _gen_csharp_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_BlockComment_start */
bool _gen_csharp_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_BlockComment_end */
bool _gen_csharp_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_Keyword_start */
bool _gen_csharp_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("using\\b|namespace\\b|class\\b|interface\\b|struct\\b|record\\b|enum\\b|public\\b|private\\b|protected\\b|internal\\b|static\\b|readonly\\b|volatile\\b|virtual\\b|override\\b|abstract\\b|sealed\\b|partial\\b|async\\b|await\\b|var\\b|int\\b|double\\b|float\\b|long\\b|short\\b|byte\\b|char\\b|bool\\b|string\\b|object\\b|decimal\\b|void\\b|if\\b|else\\b|for\\b|foreach\\b|while\\b|do\\b|switch\\b|case\\b|default\\b|break\\b|continue\\b|return\\b|new\\b|this\\b|base\\b|try\\b|catch\\b|finally\\b|throw\\b|in\\b|out\\b|ref\\b|get\\b|set\\b|value\\b|add\\b|remove\\b|delegate\\b|event\\b|lock\\b|implicit\\b|explicit\\b|operator\\b|params\\b|null\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_Variable_start */
bool _gen_csharp_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_CodeBlock_start */
bool _gen_csharp_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_CodeBlock_end */
bool _gen_csharp_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_Operator_start */
bool _gen_csharp_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\?\\?=|\\?\\.|\\?\\?|=>|\\+=|\\-=|\\*=|\\/=|%=|&=|\\^=|\\|=|<{2}=|>{2}=|\\+\\+|\\-\\-|&&|\\|\\||<{2}|>{2}|<=|>=|==|!=|->|::|\\.|[=<>!&|^~+\\-*/%\\?:;.,]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_SingleString_start */
bool _gen_csharp_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_SingleString_end */
bool _gen_csharp_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_DoubleString_start */
bool _gen_csharp_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_DoubleString_end */
bool _gen_csharp_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_StringEscape_start */
bool _gen_csharp_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t|\\\\u[0-9a-fA-F]{4}|\\\\x[0-9a-fA-F]{2}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_Number_start */
bool _gen_csharp_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(?:0[xX][0-9a-fA-F][0-9a-fA-F_]*[uUlL]*|0[bB][01][01_]*[uUlL]*|[0-9][0-9_]*(?:\\.[0-9][0-9_]*)?(?:[eE][-+]?[0-9_]+)?[fFdDlLmM]?)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_Boolean_start */
bool _gen_csharp_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_Parenthesis_start */
bool _gen_csharp_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_Parenthesis_end */
bool _gen_csharp_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_ArrayIndex_start */
bool _gen_csharp_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_csharp_ArrayIndex_end */
bool _gen_csharp_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_BlockComment_start */
bool _gen_css_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_BlockComment_end */
bool _gen_css_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_AtRule_start */
bool _gen_css_AtRule_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("@[a-zA-Z-]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_ClassName_start */
bool _gen_css_ClassName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\.[a-zA-Z_-][a-zA-Z0-9_-]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_IdName_start */
bool _gen_css_IdName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#[a-zA-Z_-][a-zA-Z0-9_-]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_PseudoClass_start */
bool _gen_css_PseudoClass_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(":{1,2}[a-zA-Z-]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_TagName_start */
bool _gen_css_TagName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z-][a-zA-Z0-9-]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_Operator_start */
bool _gen_css_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[,>+~*]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_CodeBlock_start */
bool _gen_css_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_CodeBlock_end */
bool _gen_css_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_Declaration_start */
bool _gen_css_Declaration_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z-][a-zA-Z0-9-]*\\s*:", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_Declaration_end */
bool _gen_css_Declaration_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(";", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_HexColor_start */
bool _gen_css_HexColor_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#[0-9a-fA-F]{3,8}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_Number_start */
bool _gen_css_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[0-9]*\\.?[0-9]+(?:[a-zA-Z%]+)?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_FunctionCall_start */
bool _gen_css_FunctionCall_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("url\\([^)]*\\)|rgb\\([^)]*\\)|rgba\\([^)]*\\)|hsl\\([^)]*\\)|hsla\\([^)]*\\)|var\\([^)]*\\)|calc\\([^)]*\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_SingleString_start */
bool _gen_css_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_SingleString_end */
bool _gen_css_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_DoubleString_start */
bool _gen_css_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_DoubleString_end */
bool _gen_css_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_StringEscape_start */
bool _gen_css_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_Important_start */
bool _gen_css_Important_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("!important\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_Value_start */
bool _gen_css_Value_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z-][a-zA-Z0-9-]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_DeclOperator_start */
bool _gen_css_DeclOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[,/!]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_Parenthesis_start */
bool _gen_css_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_Parenthesis_end */
bool _gen_css_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_ArrayIndex_start */
bool _gen_css_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_css_ArrayIndex_end */
bool _gen_css_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_LineComment_start */
bool _gen_fortran_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("![^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_Keyword_start */
bool _gen_fortran_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("abstract\\b|allocatable\\b|allocate\\b|assignment\\b|associated\\b|bind\\b|block\\b|call\\b|case\\b|class\\b|close\\b|common\\b|contains\\b|continue\\b|critical\\b|cycle\\b|data\\b|deallocate\\b|default\\b|deferred\\b|dimension\\b|do\\b|elemental\\b|else\\b|elseif\\b|elsewhere\\b|end\\b|enddo\\b|endif\\b|enum\\b|equivalence\\b|error\\b|exit\\b|extends\\b|external\\b|final\\b|forall\\b|format\\b|function\\b|generic\\b|go\\b|if\\b|implicit\\b|import\\b|in\\b|include\\b|inout\\b|intent\\b|interface\\b|intrinsic\\b|module\\b|new\\b|none\\b|nopass\\b|null\\b|nullify\\b|only\\b|open\\b|operator\\b|optional\\b|out\\b|parameter\\b|pass\\b|pointer\\b|print\\b|private\\b|procedure\\b|program\\b|protected\\b|public\\b|pure\\b|read\\b|recursive\\b|return\\b|save\\b|select\\b|stop\\b|subroutine\\b|target\\b|then\\b|to\\b|type\\b|use\\b|wait\\b|where\\b|while\\b|write\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_Boolean_start */
bool _gen_fortran_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\.true\\.|\\.false\\.", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_SingleString_start */
bool _gen_fortran_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_SingleString_end */
bool _gen_fortran_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_DoubleString_start */
bool _gen_fortran_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_DoubleString_end */
bool _gen_fortran_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_EscapedApostrophe_start */
bool _gen_fortran_EscapedApostrophe_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("''", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_EscapedDoubleQuote_start */
bool _gen_fortran_EscapedDoubleQuote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_Number_start */
bool _gen_fortran_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\b[0-9]+(?:\\.[0-9]+)?(?:[dDeE][-+]?[0-9]+)?\\b|\\.[0-9]+(?:[dDeE][-+]?[0-9]+)?\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_Variable_start */
bool _gen_fortran_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_Operator_start */
bool _gen_fortran_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("::|==|\\/=|>=|<=|=>|\\*\\*|\\+|-|\\*|\\/|=|<|>|,|:|%|;", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_Parenthesis_start */
bool _gen_fortran_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_Parenthesis_end */
bool _gen_fortran_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_ArrayIndex_start */
bool _gen_fortran_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_ArrayIndex_end */
bool _gen_fortran_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_CodeBlock_start */
bool _gen_fortran_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_fortran_CodeBlock_end */
bool _gen_fortran_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_LineComment_start */
bool _gen_go_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_BlockComment_start */
bool _gen_go_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_BlockComment_end */
bool _gen_go_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_Keyword_start */
bool _gen_go_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("break\\b|case\\b|chan\\b|const\\b|continue\\b|default\\b|defer\\b|else\\b|fallthrough\\b|for\\b|func\\b|go\\b|goto\\b|if\\b|import\\b|interface\\b|map\\b|package\\b|range\\b|return\\b|select\\b|struct\\b|switch\\b|type\\b|var\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_Boolean_start */
bool _gen_go_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b|nil\\b|iota\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_Variable_start */
bool _gen_go_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_CodeBlock_start */
bool _gen_go_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_CodeBlock_end */
bool _gen_go_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_Operator_start */
bool _gen_go_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\.\\.\\.|<<=|>>=|&\\^=|\\+\\+|\\-\\-|\\+=|\\-=|\\*=|\\/=|%=|&=|\\|=|\\^=|&&|\\|\\||<=|>=|==|!=|:=|<-|<<|>>|&\\^|=|<|>|!|\\+|-|\\*|\\/|%|&|\\||\\^|:|;|\\.|,", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_SingleString_start */
bool _gen_go_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_SingleString_end */
bool _gen_go_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_DoubleString_start */
bool _gen_go_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_DoubleString_end */
bool _gen_go_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_BacktickString_start */
bool _gen_go_BacktickString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_BacktickString_end */
bool _gen_go_BacktickString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_StringEscape_start */
bool _gen_go_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\[abfnrtv\\\\\\\"']|\\\\x[0-9a-fA-F]{2}|\\\\u[0-9a-fA-F]{4}|\\\\U[0-9a-fA-F]{8}|\\\\[0-7]{3}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_Number_start */
bool _gen_go_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("0[xX][0-9a-fA-F]+|0[oO][0-7]+|0[bB][01]+|[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_Parenthesis_start */
bool _gen_go_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_Parenthesis_end */
bool _gen_go_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_ArrayIndex_start */
bool _gen_go_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_go_ArrayIndex_end */
bool _gen_go_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_html_Comment_start */
bool _gen_html_Comment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<!--", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_html_Comment_end */
bool _gen_html_Comment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("-->", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_html_Doctype_start */
bool _gen_html_Doctype_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<!doctype\\b[^>]*>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_html_ClosingTag_start */
bool _gen_html_ClosingTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<\\/[a-zA-Z0-9:-]+\\s*>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_html_Tag_start */
bool _gen_html_Tag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<[a-zA-Z0-9:-]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_html_Tag_end */
bool _gen_html_Tag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/?>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_html_DoubleString_start */
bool _gen_html_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_html_DoubleString_end */
bool _gen_html_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_html_SingleString_start */
bool _gen_html_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_html_SingleString_end */
bool _gen_html_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_html_StringEscape_start */
bool _gen_html_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_html_Equal_start */
bool _gen_html_Equal_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_html_AttributeName_start */
bool _gen_html_AttributeName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z0-9:-]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_LineComment_start */
bool _gen_jai_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_BlockComment_start */
bool _gen_jai_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_BlockComment_end */
bool _gen_jai_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_Directive_start */
bool _gen_jai_Directive_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#(?:import|add_context|align|as|assert|bake_arguments|bake_constants|bake|bytes|caller_code|caller_location|char|code|compile_time|complete|dump|elsewhere|expand|file|filepath|ifx|if|insert|intrinsic|modify|module_parameters|must|no_abc|no_alias|no_padding|no_reset|place|placeholder|procedure_name|procedure_of_call|program_export|run|specified|symmetric|this|through|type_info|type|unshared|scope_export|scope_file|scope_module|string)\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_Keyword_start */
bool _gen_jai_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("if\\b|ifx\\b|then\\b|else\\b|case\\b|return\\b|break\\b|continue\\b|while\\b|for\\b|remove\\b|defer\\b|context\\b|push_context\\b|using\\b|temp\\b|struct\\b|enum\\b|union\\b|cast\\b|trunc\\b|no_check\\b|xx\\b|inline\\b|int\\b|u8\\b|u16\\b|u32\\b|u64\\b|s8\\b|s16\\b|s32\\b|s64\\b|float\\b|float32\\b|float64\\b|bool\\b|string\\b|void\\b|Code\\b|Type\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_Boolean_start */
bool _gen_jai_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b|null\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_Variable_start */
bool _gen_jai_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_CodeBlock_start */
bool _gen_jai_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_CodeBlock_end */
bool _gen_jai_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_Operator_start */
bool _gen_jai_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("::|:=|:|===|==|!=|<=|>=|&&|\\|\\||\\+=|\\-=|\\*=|\\/=|%=|&=|\\^=|\\|=|<<=|>>=|->|<<|>>|[=<>!&|^~+\\-*/%\\?:;.,]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_DoubleString_start */
bool _gen_jai_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_DoubleString_end */
bool _gen_jai_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_StringEscape_start */
bool _gen_jai_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t|\\\\u[0-9a-fA-F]{4}|\\\\x[0-9a-fA-F]{2}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_Number_start */
bool _gen_jai_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("0[xX][0-9a-fA-F_]+\\b|0[bB][01_]+\\b|0[hH][0-9a-fA-F_]+\\b|[0-9_]*\\.?[0-9_]+(?:[eE][-+]?[0-9_]+)?\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_Parenthesis_start */
bool _gen_jai_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_Parenthesis_end */
bool _gen_jai_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_ArrayIndex_start */
bool _gen_jai_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_jai_ArrayIndex_end */
bool _gen_jai_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_LineComment_start */
bool _gen_java_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_BlockComment_start */
bool _gen_java_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_BlockComment_end */
bool _gen_java_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_Annotation_start */
bool _gen_java_Annotation_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("@[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_Keyword_start */
bool _gen_java_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("class\\b|interface\\b|enum\\b|extends\\b|implements\\b|package\\b|import\\b|public\\b|private\\b|protected\\b|static\\b|final\\b|void\\b|int\\b|double\\b|float\\b|long\\b|short\\b|byte\\b|char\\b|boolean\\b|if\\b|else\\b|for\\b|while\\b|do\\b|switch\\b|case\\b|default\\b|break\\b|continue\\b|return\\b|new\\b|this\\b|super\\b|try\\b|catch\\b|finally\\b|throw\\b|throws\\b|instanceof\\b|synchronized\\b|volatile\\b|transient\\b|abstract\\b|native\\b|strictfp\\b|assert\\b|null\\b|var\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_Variable_start */
bool _gen_java_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_CodeBlock_start */
bool _gen_java_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_CodeBlock_end */
bool _gen_java_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_Operator_start */
bool _gen_java_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\+=|\\-=|\\*=|\\/=|%=|&=|\\^=|\\|=|<{2}=|>{2,3}=|\\+\\+|\\-\\-|&&|\\|\\||<=|>=|==|!=|->|::|[=<>!&|^~+\\-*/%\\?:;.,]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_SingleString_start */
bool _gen_java_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_SingleString_end */
bool _gen_java_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_DoubleString_start */
bool _gen_java_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_DoubleString_end */
bool _gen_java_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_StringEscape_start */
bool _gen_java_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t|\\\\u[0-9a-fA-F]{4}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_Number_start */
bool _gen_java_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(?:0[xX][0-9a-fA-F][0-9a-fA-F_]*[lL]*|0[bB][01][01_]*[lL]*|[0-9][0-9_]*(?:\\.[0-9][0-9_]*)?(?:[eE][-+]?[0-9_]+)?[fFdDlL]?)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_Boolean_start */
bool _gen_java_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_Parenthesis_start */
bool _gen_java_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_Parenthesis_end */
bool _gen_java_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_ArrayIndex_start */
bool _gen_java_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_java_ArrayIndex_end */
bool _gen_java_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_LineComment_start */
bool _gen_javascript_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_BlockComment_start */
bool _gen_javascript_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_BlockComment_end */
bool _gen_javascript_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_Keyword_start */
bool _gen_javascript_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("async\\b|await\\b|class\\b|const\\b|let\\b|var\\b|function\\b|import\\b|export\\b|from\\b|default\\b|extends\\b|super\\b|this\\b|new\\b|return\\b|try\\b|catch\\b|finally\\b|throw\\b|if\\b|else\\b|switch\\b|case\\b|break\\b|continue\\b|do\\b|while\\b|for\\b|in\\b|of\\b|typeof\\b|instanceof\\b|yield\\b|debugger\\b|delete\\b|void\\b|with\\b|null\\b|undefined\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_Boolean_start */
bool _gen_javascript_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_Variable_start */
bool _gen_javascript_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_$][a-zA-Z0-9_$]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_CodeBlock_start */
bool _gen_javascript_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_CodeBlock_end */
bool _gen_javascript_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_Regex_start */
bool _gen_javascript_Regex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/(?:[^\\/\\\\\r\n]|\\\\.)+\\/[a-zA-Z]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_Operator_start */
bool _gen_javascript_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("===|==|!==|!=|=>|\\+=|\\-=|\\*=|\\/=|%=|\\+\\+|\\-\\-|&&|\\|\\||<=|>=|\\?\\.|\\?\\?|[=<>!&|^~+\\-*/%\\?:;.,]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_SingleString_start */
bool _gen_javascript_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_SingleString_end */
bool _gen_javascript_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_DoubleString_start */
bool _gen_javascript_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_DoubleString_end */
bool _gen_javascript_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_TemplateString_start */
bool _gen_javascript_TemplateString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_TemplateString_end */
bool _gen_javascript_TemplateString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_StringEscape_start */
bool _gen_javascript_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t|\\\\\\`|\\\\u[0-9a-fA-F]{4}|\\\\x[0-9a-fA-F]{2}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_Number_start */
bool _gen_javascript_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("0[xX][0-9a-fA-F]+|0[oO][0-7]+|0[bB][01]+|[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_Parenthesis_start */
bool _gen_javascript_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_Parenthesis_end */
bool _gen_javascript_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_ArrayIndex_start */
bool _gen_javascript_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_javascript_ArrayIndex_end */
bool _gen_javascript_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_Object_start */
bool _gen_json_Object_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_Object_end */
bool _gen_json_Object_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_Array_start */
bool _gen_json_Array_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_Array_end */
bool _gen_json_Array_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_Key_start */
bool _gen_json_Key_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\"(?=[^\\\"\\\\\\\\]*(?:\\\\\\\\.[^\\\"\\\\\\\\]*)*\\\"\\s*:)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_Key_end */
bool _gen_json_Key_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(\\\")\\s*:", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_String_start */
bool _gen_json_String_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_String_end */
bool _gen_json_String_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_Number_start */
bool _gen_json_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\d+(?:\\.\\d+)?(?:e[+-]?\\d+)?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_StringEscape_start */
bool _gen_json_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\/|\\\\b|\\\\f|\\\\n|\\\\r|\\\\t|\\\\u[0-9a-f]{4}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_Bool_start */
bool _gen_json_Bool_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true|false", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_Null_start */
bool _gen_json_Null_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("null", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_ValueSeparator_start */
bool _gen_json_ValueSeparator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(",", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_json_KeyValueSeparator_start */
bool _gen_json_KeyValueSeparator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(":", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_LineComment_start */
bool _gen_matlab_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("%[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_Keyword_start */
bool _gen_matlab_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("break\\b|case\\b|catch\\b|classdef\\b|continue\\b|else\\b|elseif\\b|end\\b|enumeration\\b|events\\b|for\\b|function\\b|global\\b|if\\b|methods\\b|otherwise\\b|parfor\\b|persistent\\b|properties\\b|return\\b|spmd\\b|switch\\b|try\\b|while\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_Boolean_start */
bool _gen_matlab_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_SingleString_start */
bool _gen_matlab_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_SingleString_end */
bool _gen_matlab_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_DoubleString_start */
bool _gen_matlab_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_DoubleString_end */
bool _gen_matlab_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_StringEscape_start */
bool _gen_matlab_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("''|\"\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_Number_start */
bool _gen_matlab_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?[iIjJ]?|\\.[0-9]+(?:[eE][-+]?[0-9]+)?[iIjJ]?|0[xX][0-9a-fA-F]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_Variable_start */
bool _gen_matlab_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_Operator_start */
bool _gen_matlab_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\.\\*|\\.\\/|\\.\\\\|\\.\\^|==|~=|\\.'|&&|\\|\\||<<|>>|\\+=|\\-=|\\*=|\\/=|[=~&|<>=+\\-*/\\\\^'.:;,@]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_Parenthesis_start */
bool _gen_matlab_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_Parenthesis_end */
bool _gen_matlab_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_ArrayIndex_start */
bool _gen_matlab_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_ArrayIndex_end */
bool _gen_matlab_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_CodeBlock_start */
bool _gen_matlab_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_matlab_CodeBlock_end */
bool _gen_matlab_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_HtmlComment_start */
bool _gen_md_HtmlComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<!--", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_HtmlComment_end */
bool _gen_md_HtmlComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("-->", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_FencedCodeBlock_start */
bool _gen_md_FencedCodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("```|~~~", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_FencedCodeBlock_end */
bool _gen_md_FencedCodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("```|~~~", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Heading_start */
bool _gen_md_Heading_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#{1,6}[ \\t]+[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_HorizontalRule_start */
bool _gen_md_HorizontalRule_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("---[ \\t]*|\\*\\*\\*[ \\t]*|___[ \\t]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Blockquote_start */
bool _gen_md_Blockquote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(">[ \\t]*[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_HtmlTag_start */
bool _gen_md_HtmlTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<\\/?[a-zA-Z0-9:-]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_HtmlTag_end */
bool _gen_md_HtmlTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/?>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_DoubleString_start */
bool _gen_md_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_DoubleString_end */
bool _gen_md_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_SingleString_start */
bool _gen_md_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_SingleString_end */
bool _gen_md_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Equal_start */
bool _gen_md_Equal_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_AttributeName_start */
bool _gen_md_AttributeName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z0-9:-]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_TaskCheckbox_start */
bool _gen_md_TaskCheckbox_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[[ xX]\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Footnote_start */
bool _gen_md_Footnote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[\\^[a-zA-Z0-9_-]+\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Image_start */
bool _gen_md_Image_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("!\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Image_end */
bool _gen_md_Image_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\](?:\\([^)]*\\)|\\[[^\\]]*\\])?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Link_start */
bool _gen_md_Link_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Link_end */
bool _gen_md_Link_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\](?:\\([^)]*\\)|\\[[^\\]]*\\])?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_InlineCode_start */
bool _gen_md_InlineCode_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_InlineCode_end */
bool _gen_md_InlineCode_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Bold_start */
bool _gen_md_Bold_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\*|__", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Bold_end */
bool _gen_md_Bold_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\*|__", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Italic_start */
bool _gen_md_Italic_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*|_", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Italic_end */
bool _gen_md_Italic_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*|_", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Strikethrough_start */
bool _gen_md_Strikethrough_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("~~", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_Strikethrough_end */
bool _gen_md_Strikethrough_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("~~", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_UnorderedList_start */
bool _gen_md_UnorderedList_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[-*+][ \\t]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_OrderedList_start */
bool _gen_md_OrderedList_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[0-9]+[.)][ \\t]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_TablePipe_start */
bool _gen_md_TablePipe_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\|", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_md_BackslashEscape_start */
bool _gen_md_BackslashEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\[\\\\`*_{}\\[\\]()#+\\-.!|~>\"']", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_LineComment_start */
bool _gen_pascal_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_CompilerDirective_start */
bool _gen_pascal_CompilerDirective_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{\\$", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_CompilerDirective_end */
bool _gen_pascal_CompilerDirective_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_BlockCommentCurly_start */
bool _gen_pascal_BlockCommentCurly_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_BlockCommentCurly_end */
bool _gen_pascal_BlockCommentCurly_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_BlockCommentParen_start */
bool _gen_pascal_BlockCommentParen_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_BlockCommentParen_end */
bool _gen_pascal_BlockCommentParen_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_Keyword_start */
bool _gen_pascal_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("and\\b|or\\b|not\\b|xor\\b|shl\\b|shr\\b|div\\b|mod\\b|as\\b|is\\b|program\\b|unit\\b|interface\\b|implementation\\b|uses\\b|var\\b|type\\b|const\\b|resourcestring\\b|threadvar\\b|begin\\b|end\\b|procedure\\b|function\\b|constructor\\b|destructor\\b|property\\b|class\\b|record\\b|object\\b|helper\\b|strict\\b|private\\b|protected\\b|public\\b|published\\b|initialization\\b|finalization\\b|if\\b|then\\b|else\\b|case\\b|of\\b|for\\b|to\\b|downto\\b|do\\b|while\\b|repeat\\b|until\\b|with\\b|try\\b|except\\b|finally\\b|raise\\b|at\\b|on\\b|inherited\\b|inline\\b|overload\\b|override\\b|virtual\\b|abstract\\b|reintroduce\\b|nil\\b|out\\b|label\\b|goto\\b|exports\\b|library\\b|package\\b|requires\\b|contains\\b|absolute\\b|assembler\\b|cdecl\\b|pascal\\b|register\\b|safecall\\b|stdcall\\b|reference\\b|operator\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_DataType_start */
bool _gen_pascal_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("integer\\b|cardinal\\b|shortint\\b|smallint\\b|longint\\b|int64\\b|byte\\b|word\\b|fixedint\\b|fixeduint\\b|real\\b|single\\b|double\\b|extended\\b|comp\\b|currency\\b|char\\b|ansichar\\b|widechar\\b|boolean\\b|bytebool\\b|wordbool\\b|longbool\\b|string\\b|ansistring\\b|widestring\\b|unicodestring\\b|shortstring\\b|pointer\\b|variant\\b|tobject\\b|tclass\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_Boolean_start */
bool _gen_pascal_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_CharLiteral_start */
bool _gen_pascal_CharLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#[0-9]+|#\\$[0-9a-fA-F]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_SingleString_start */
bool _gen_pascal_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_SingleString_end */
bool _gen_pascal_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_EscapedQuote_start */
bool _gen_pascal_EscapedQuote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("''", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_Number_start */
bool _gen_pascal_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\$[0-9a-fA-F]+\\b|\\b[0-9]+(?:\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_Variable_start */
bool _gen_pascal_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_Operator_start */
bool _gen_pascal_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(":=|==|<=|>=|<>|\\+=|\\-=|\\*=|\\/=|[@\\^.\\,:;\\+\\-\\*\\/=<>]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_Parenthesis_start */
bool _gen_pascal_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_Parenthesis_end */
bool _gen_pascal_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_ArrayIndex_start */
bool _gen_pascal_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_ArrayIndex_end */
bool _gen_pascal_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_CodeBlock_start */
bool _gen_pascal_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_pascal_CodeBlock_end */
bool _gen_pascal_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_LineComment_start */
bool _gen_perl_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_PODBlock_start */
bool _gen_perl_PODBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("^=\\w+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_PODBlock_end */
bool _gen_perl_PODBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("^=cut", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_Keyword_start */
bool _gen_perl_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("package\\b|use\\b|require\\b|sub\\b|my\\b|our\\b|local\\b|state\\b|if\\b|unless\\b|else\\b|elsif\\b|given\\b|when\\b|default\\b|for\\b|foreach\\b|while\\b|until\\b|do\\b|continue\\b|last\\b|next\\b|redo\\b|goto\\b|return\\b|die\\b|warn\\b|exit\\b|eval\\b|try\\b|catch\\b|finally\\b|throw\\b|bless\\b|ref\\b|tie\\b|untie\\b|defined\\b|exists\\b|delete\\b|shift\\b|unshift\\b|push\\b|pop\\b|splice\\b|split\\b|join\\b|keys\\b|values\\b|each\\b|map\\b|grep\\b|sort\\b|print\\b|say\\b|printf\\b|sprintf\\b|open\\b|close\\b|read\\b|write\\b|sysopen\\b|sysread\\b|syswrite\\b|seek\\b|tell\\b|truncate\\b|flock\\b|chdir\\b|mkdir\\b|rmdir\\b|opendir\\b|closedir\\b|readdir\\b|glob\\b|unlink\\b|rename\\b|chmod\\b|chown\\b|umask\\b|link\\b|symlink\\b|readlink\\b|stat\\b|lstat\\b|fcntl\\b|ioctl\\b|select\\b|socket\\b|connect\\b|bind\\b|listen\\b|accept\\b|send\\b|recv\\b|shutdown\\b|setsockopt\\b|getsockopt\\b|fork\\b|exec\\b|system\\b|qx\\b|pipe\\b|wait\\b|waitpid\\b|times\\b|alarm\\b|sleep\\b|gmtime\\b|localtime\\b|time\\b|caller\\b|wantarray\\b|prototype\\b|__FILE__\\b|__LINE__\\b|__PACKAGE__\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_Boolean_start */
bool _gen_perl_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\b(?:undef)\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_SingleString_start */
bool _gen_perl_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_SingleString_end */
bool _gen_perl_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_DoubleString_start */
bool _gen_perl_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_DoubleString_end */
bool _gen_perl_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_StringEscape_start */
bool _gen_perl_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\[\\\\\\\"'nrtfeab]|\\\\x[0-9a-fA-F]{1,2}|\\\\[0-7]{1,3}|\\\\c.", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_Number_start */
bool _gen_perl_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("0[xX][0-9a-fA-F]+|0[oO][0-7]+|0[bB][01]+|[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_Variable_start */
bool _gen_perl_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[$@%*][a-zA-Z_][a-zA-Z0-9_]*|[$@%*]\\^[a-zA-Z]|[$@%*]\\$|[$@%*]\\d+|[$@%*][:]{2}[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_Operator_start */
bool _gen_perl_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\.\\.\\.|<=>|\\|\\||=|\\|\\||=>|->|==|!=|<=|>=|=~|!~|&&|//|\\.\\.|\\+\\+|\\-\\-|\\*\\*|\\+=|\\-=|\\*=|\\/=|%=|&=|\\^=|<<|>>|=|<|>|!|\\||\\^|&|\\*|/|%|\\+|-|\\.|~|,|;", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_Parenthesis_start */
bool _gen_perl_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_Parenthesis_end */
bool _gen_perl_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_ArrayIndex_start */
bool _gen_perl_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_ArrayIndex_end */
bool _gen_perl_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_CodeBlock_start */
bool _gen_perl_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_perl_CodeBlock_end */
bool _gen_perl_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_Tag_start */
bool _gen_php_Tag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<\\?php|<\\?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_Tag_end */
bool _gen_php_Tag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\?>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_LineComment_start */
bool _gen_php_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*|#[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_BlockComment_start */
bool _gen_php_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_BlockComment_end */
bool _gen_php_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_ArrayKeyValue_start */
bool _gen_php_ArrayKeyValue_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("=>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_MemberAccess_start */
bool _gen_php_MemberAccess_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("->", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_Variable_start */
bool _gen_php_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\$[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_CodeBlock_start */
bool _gen_php_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_CodeBlock_end */
bool _gen_php_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_Operator_start */
bool _gen_php_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("===|!==|==|!=|\\+=|\\-=|\\*=|\\/=|\\.\\=|%=|\\+\\+|\\-\\-|<=|>=|&&|\\|\\||[=<>!&|^~+\\-*/%\\?:;]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_SingleString_start */
bool _gen_php_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_SingleString_end */
bool _gen_php_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_DoubleString_start */
bool _gen_php_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_DoubleString_end */
bool _gen_php_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_StringEscape_start */
bool _gen_php_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\$|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_Number_start */
bool _gen_php_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[0-9]+\\.?[0-9]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_Boolean_start */
bool _gen_php_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_Keyword_start */
bool _gen_php_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("echo\\b|print\\b|function\\b|class\\b|public\\b|private\\b|protected\\b|static\\b|if\\b|else\\b|elseif\\b|foreach\\b|for\\b|while\\b|do\\b|switch\\b|case\\b|default\\b|return\\b|require\\b|require_once\\b|include\\b|include_once\\b|new\\b|use\\b|namespace\\b|try\\b|catch\\b|throw\\b|finally\\b|global\\b|as\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_Parenthesis_start */
bool _gen_php_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_Parenthesis_end */
bool _gen_php_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_ArrayIndex_start */
bool _gen_php_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_php_ArrayIndex_end */
bool _gen_php_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_LineComment_start */
bool _gen_python_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_TripleSingleString_start */
bool _gen_python_TripleSingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'''", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_TripleSingleString_end */
bool _gen_python_TripleSingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'''", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_TripleDoubleString_start */
bool _gen_python_TripleDoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"\"\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_TripleDoubleString_end */
bool _gen_python_TripleDoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"\"\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_SingleString_start */
bool _gen_python_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_SingleString_end */
bool _gen_python_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_DoubleString_start */
bool _gen_python_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_DoubleString_end */
bool _gen_python_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_FString_start */
bool _gen_python_FString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[fF](?:\"\"\"|'''|\"|')", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_FString_end */
bool _gen_python_FString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(?:\"\"\"|'''|\"|')", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_FStringInterpolation_start */
bool _gen_python_FStringInterpolation_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_FStringInterpolation_end */
bool _gen_python_FStringInterpolation_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_StringEscape_start */
bool _gen_python_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\[\\\\\\\"'nrt0]|\\\\x[0-9a-fA-F]{2}|\\\\u[0-9a-fA-F]{4}|\\\\U[0-9a-fA-F]{8}|\\\\N\\{[a-zA-Z_][a-zA-Z0-9_]*\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_Keyword_start */
bool _gen_python_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("None\\b|and\\b|as\\b|assert\\b|async\\b|await\\b|break\\b|class\\b|continue\\b|def\\b|del\\b|elif\\b|else\\b|except\\b|finally\\b|for\\b|from\\b|global\\b|if\\b|import\\b|in\\b|is\\b|lambda\\b|nonlocal\\b|not\\b|or\\b|pass\\b|raise\\b|return\\b|try\\b|while\\b|with\\b|yield\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_Boolean_start */
bool _gen_python_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("True\\b|False\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_Number_start */
bool _gen_python_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("0[xX][0-9a-fA-F][0-9a-fA-F_]*|0[oO][0-7][0-7_]*|0[bB][01][01_]*|[0-9][0-9_]*(?:\\.[0-9][0-9_]*)?(?:[eE][-+]?[0-9][0-9_]*)?[jJ]?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_Variable_start */
bool _gen_python_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("@?[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_Operator_start */
bool _gen_python_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(":=|\\.\\.\\.|->|//|\\*\\*|<<|>>|==|!=|<=|>=|\\+=|\\-=|\\*=|\\/=|//=|%=|@=|&=|\\|=|\\^=|<<=|>>=|\\*\\*=|[=<>!&|^~+\\-*/%@.,;:]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_Parenthesis_start */
bool _gen_python_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_Parenthesis_end */
bool _gen_python_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_ArrayIndex_start */
bool _gen_python_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_ArrayIndex_end */
bool _gen_python_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_CodeBlock_start */
bool _gen_python_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_python_CodeBlock_end */
bool _gen_python_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_LineComment_start */
bool _gen_r_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_Keyword_start */
bool _gen_r_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(?:break|else|for|function|if|in|next|repeat|return|switch|while)\\b(?!\\.)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_Boolean_start */
bool _gen_r_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(?:TRUE|FALSE|NULL|NA|Inf|NaN|NA_integer_|NA_real_|NA_complex_|NA_character_)\\b(?!\\.)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_SingleString_start */
bool _gen_r_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_SingleString_end */
bool _gen_r_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_DoubleString_start */
bool _gen_r_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_DoubleString_end */
bool _gen_r_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_BacktickString_start */
bool _gen_r_BacktickString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_BacktickString_end */
bool _gen_r_BacktickString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_StringEscape_start */
bool _gen_r_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\[\\\\\\\"'nrt0]|\\\\x[0-9a-fA-F]{1,2}|\\\\u[0-9a-fA-F]{4}|\\\\U[0-9a-fA-F]{8}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_Number_start */
bool _gen_r_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("0[xX][0-9a-fA-F]+[lL]?|[0-9]+(?:\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?[lLi]?|\\.[0-9]+(?:[eE][-+]?[0-9]+)?[lLi]?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_Variable_start */
bool _gen_r_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z.][a-zA-Z0-9_.]*|\\.[a-zA-Z.][a-zA-Z0-9_.]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_Operator_start */
bool _gen_r_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<-|<<-|->|->>|:::|::|%[^%\\r\\n]*%|\\*\\*|~|\\$|@|\\|\\||&&|\\||&|!=|<=|>=|==|[=<>!+\\-*/^:,;]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_Parenthesis_start */
bool _gen_r_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_Parenthesis_end */
bool _gen_r_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_ArrayIndex_start */
bool _gen_r_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_ArrayIndex_end */
bool _gen_r_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_CodeBlock_start */
bool _gen_r_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_r_CodeBlock_end */
bool _gen_r_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_LineComment_start */
bool _gen_rust_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_BlockComment_start */
bool _gen_rust_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_BlockComment_end */
bool _gen_rust_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_Attribute_start */
bool _gen_rust_Attribute_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#!?\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_Attribute_end */
bool _gen_rust_Attribute_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_Macro_start */
bool _gen_rust_Macro_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*!", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_Keyword_start */
bool _gen_rust_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("as\\b|async\\b|await\\b|break\\b|const\\b|continue\\b|crate\\b|dyn\\b|else\\b|enum\\b|extern\\b|fn\\b|for\\b|if\\b|impl\\b|in\\b|let\\b|loop\\b|match\\b|mod\\b|move\\b|mut\\b|pub\\b|ref\\b|return\\b|Self\\b|self\\b|static\\b|struct\\b|super\\b|trait\\b|type\\b|unsafe\\b|use\\b|where\\b|while\\b|yield\\b|macro\\b|bool\\b|char\\b|str\\b|i8\\b|u8\\b|i16\\b|u16\\b|i32\\b|u32\\b|i64\\b|u64\\b|i128\\b|u128\\b|isize\\b|usize\\b|f32\\b|f64\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_Boolean_start */
bool _gen_rust_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b|Some\\b|None\\b|Ok\\b|Err\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_CharLiteral_start */
bool _gen_rust_CharLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'(?:[^\\\\']|\\\\\\\\.)'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_Lifetime_start */
bool _gen_rust_Lifetime_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'[a-zA-Z_][a-zA-Z0-9_]*\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_Variable_start */
bool _gen_rust_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_CodeBlock_start */
bool _gen_rust_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_CodeBlock_end */
bool _gen_rust_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_Operator_start */
bool _gen_rust_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("::|:=|:|===|==|!=|<=|>=|&&|\\|\\||\\+=|\\-=|\\*=|\\/=|%=|&=|\\^=|\\|=|<<=|>>=|->|<<|>>|=>|[=<>!&|^~+\\-*/%\\?:;.,]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_DoubleString_start */
bool _gen_rust_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_DoubleString_end */
bool _gen_rust_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_StringEscape_start */
bool _gen_rust_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t|\\\\u\\{[0-9a-fA-F]{1,6}\\}|\\\\x[0-9a-fA-F]{2}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_Number_start */
bool _gen_rust_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("0[xX][0-9a-fA-F_]+\\b|0[bB][01_]+\\b|0[oO][0-7_]+\\b|[0-9_]*\\.?[0-9_]+(?:[eE][-+]?[0-9_]+)?\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_Parenthesis_start */
bool _gen_rust_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_Parenthesis_end */
bool _gen_rust_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_ArrayIndex_start */
bool _gen_rust_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_rust_ArrayIndex_end */
bool _gen_rust_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_LineComment_start */
bool _gen_scratch_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#[^\\r\\n]*|\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_Keyword_start */
bool _gen_scratch_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("when\\b|green\\b|flag\\b|clicked\\b|key\\b|pressed\\b|space\\b|any\\b|move\\b|steps\\b|turn\\b|cw\\b|ccw\\b|degrees\\b|go\\b|to\\b|glide\\b|secs\\b|point\\b|towards\\b|direction\\b|change\\b|set\\b|effect\\b|size\\b|graphic\\b|clear\\b|show\\b|hide\\b|say\\b|think\\b|seconds\\b|wait\\b|costume\\b|backdrop\\b|switch\\b|next\\b|sound\\b|play\\b|until\\b|stop\\b|all\\b|this\\b|sprite\\b|script\\b|create\\b|clone\\b|delete\\b|forever\\b|repeat\\b|if\\b|then\\b|else\\b|end\\b|for\\b|broadcast\\b|message\\b|message1\\b|new\\b|ask\\b|answer\\b|mouse\\b|x\\b|y\\b|down\\b|loudness\\b|timer\\b|reset\\b|random\\b|pick\\b|between\\b|join\\b|letter\\b|of\\b|length\\b|contains\\b|round\\b|sqrt\\b|abs\\b|and\\b|or\\b|not\\b|touching\\b|color\\b|distance\\b|item\\b|list\\b|add\\b|insert\\b|replace\\b|counter\\b|mod\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_Boolean_start */
bool _gen_scratch_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_DoubleString_start */
bool _gen_scratch_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_DoubleString_end */
bool _gen_scratch_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_Number_start */
bool _gen_scratch_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[0-9]*\\.?[0-9]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_Variable_start */
bool _gen_scratch_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*|\\[[a-zA-Z_][a-zA-Z0-9_]*\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_Operator_start */
bool _gen_scratch_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[=<>+\\-*/]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_Parenthesis_start */
bool _gen_scratch_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_Parenthesis_end */
bool _gen_scratch_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_ArrayIndex_start */
bool _gen_scratch_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_ArrayIndex_end */
bool _gen_scratch_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_CodeBlock_start */
bool _gen_scratch_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_scratch_CodeBlock_end */
bool _gen_scratch_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_LineComment_start */
bool _gen_sql_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("--[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_BlockComment_start */
bool _gen_sql_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_BlockComment_end */
bool _gen_sql_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_Keyword_start */
bool _gen_sql_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("select\\b|insert\\b|update\\b|delete\\b|from\\b|where\\b|join\\b|left\\b|right\\b|inner\\b|outer\\b|on\\b|group\\b|by\\b|having\\b|order\\b|limit\\b|offset\\b|create\\b|table\\b|drop\\b|alter\\b|add\\b|column\\b|index\\b|primary\\b|key\\b|foreign\\b|references\\b|into\\b|values\\b|set\\b|and\\b|or\\b|not\\b|in\\b|is\\b|null\\b|like\\b|between\\b|exists\\b|any\\b|all\\b|as\\b|distinct\\b|union\\b|intersect\\b|except\\b|with\\b|recursive\\b|database\\b|use\\b|view\\b|trigger\\b|procedure\\b|function\\b|returns\\b|declare\\b|begin\\b|end\\b|if\\b|else\\b|then\\b|commit\\b|rollback\\b|transaction\\b|grant\\b|revoke\\b|constraint\\b|default\\b|unique\\b|check\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_DataType_start */
bool _gen_sql_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("int\\b|integer\\b|varchar\\b|char\\b|text\\b|boolean\\b|bool\\b|date\\b|time\\b|timestamp\\b|datetime\\b|decimal\\b|numeric\\b|float\\b|double\\b|real\\b|blob\\b|clob\\b|json\\b|uuid\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_Boolean_start */
bool _gen_sql_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_BacktickIdentifier_start */
bool _gen_sql_BacktickIdentifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_BacktickIdentifier_end */
bool _gen_sql_BacktickIdentifier_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_BracketIdentifier_start */
bool _gen_sql_BracketIdentifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_BracketIdentifier_end */
bool _gen_sql_BracketIdentifier_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_Variable_start */
bool _gen_sql_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_Operator_start */
bool _gen_sql_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<>|<=|>=|!=|:=|[-+*/%&|^~<>!=;.,|=]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_SingleString_start */
bool _gen_sql_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_SingleString_end */
bool _gen_sql_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_DoubleString_start */
bool _gen_sql_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_DoubleString_end */
bool _gen_sql_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_StringEscape_start */
bool _gen_sql_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_Number_start */
bool _gen_sql_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\b[0-9]+(?:\\.[0-9]+)?\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_Parenthesis_start */
bool _gen_sql_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_Parenthesis_end */
bool _gen_sql_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_ArrayIndex_start */
bool _gen_sql_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_ArrayIndex_end */
bool _gen_sql_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_CodeBlock_start */
bool _gen_sql_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_sql_CodeBlock_end */
bool _gen_sql_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_LineComment_start */
bool _gen_swift_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_BlockComment_start */
bool _gen_swift_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_BlockComment_end */
bool _gen_swift_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_Keyword_start */
bool _gen_swift_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("actor\\b|associatedtype\\b|async\\b|await\\b|class\\b|convenience\\b|deinit\\b|distributed\\b|dynamic\\b|enum\\b|extension\\b|fileprivate\\b|final\\b|func\\b|get\\b|guard\\b|import\\b|indirect\\b|infix\\b|init\\b|inout\\b|internal\\b|isolated\\b|lazy\\b|let\\b|macro\\b|mutating\\b|nonisolated\\b|nonmutating\\b|open\\b|operator\\b|optional\\b|override\\b|package\\b|postfix\\b|precedencegroup\\b|prefix\\b|private\\b|protocol\\b|public\\b|required\\b|rethrows\\b|set\\b|some\\b|static\\b|struct\\b|subscript\\b|typealias\\b|unowned\\b|var\\b|weak\\b|willSet\\b|didSet\\b|where\\b|any\\b|as\\b|break\\b|case\\b|catch\\b|continue\\b|default\\b|defer\\b|do\\b|else\\b|fallthrough\\b|for\\b|if\\b|in\\b|is\\b|repeat\\b|return\\b|switch\\b|throw\\b|throws\\b|try\\b|while\\b|#available\\b|#selector\\b|#keyPath\\b|#file\\b|#line\\b|#column\\b|#function\\b|#dsohandle\\b|#warning\\b|#error\\b|#if\\b|#else\\b|#elseif\\b|#endif\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_Boolean_start */
bool _gen_swift_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b|nil\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_Variable_start */
bool _gen_swift_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_CodeBlock_start */
bool _gen_swift_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_CodeBlock_end */
bool _gen_swift_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_Operator_start */
bool _gen_swift_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\?\\?|===|!==|\\.\\.<|\\.\\.\\.|->|\\+=|\\-=|\\*=|\\/=|%=|&=|\\^=|\\|=|<<=|>>=|&&|\\|\\||<=|>=|==|!=|<<|>>|[=<>!&|^~+\\-*/%\\?:;.,]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_DoubleString_start */
bool _gen_swift_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_DoubleString_end */
bool _gen_swift_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_MultiLineString_start */
bool _gen_swift_MultiLineString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"\"\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_MultiLineString_end */
bool _gen_swift_MultiLineString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"\"\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_StringInterpolation_start */
bool _gen_swift_StringInterpolation_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_StringInterpolation_end */
bool _gen_swift_StringInterpolation_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_StringEscape_start */
bool _gen_swift_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\[\\\\\\\"'nrt0]|\\\\u\\{[0-9a-fA-F]{1,8}\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_Number_start */
bool _gen_swift_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("0[xX][0-9a-fA-F]+(?:\\.[0-9a-fA-F]+)?(?:[pP][-+]?[0-9]+)?\\b|0[oO][0-7]+\\b|0[bB][01]+\\b|[0-9]+(?:\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_Parenthesis_start */
bool _gen_swift_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_Parenthesis_end */
bool _gen_swift_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_ArrayIndex_start */
bool _gen_swift_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_swift_ArrayIndex_end */
bool _gen_swift_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_TypeofKeyword_start */
bool _gen_typescript_TypeofKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("typeof\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_VoidKeyword_start */
bool _gen_typescript_VoidKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("void\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_DeleteKeyword_start */
bool _gen_typescript_DeleteKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("delete\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_AwaitKeyword_start */
bool _gen_typescript_AwaitKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("await\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_YieldKeyword_start */
bool _gen_typescript_YieldKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("yield\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_InstanceofKeyword_start */
bool _gen_typescript_InstanceofKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("instanceof\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_InKeyword_start */
bool _gen_typescript_InKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("in\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_OutKeyword_start */
bool _gen_typescript_OutKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("out\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ThisKeyword_start */
bool _gen_typescript_ThisKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("this\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_TrueKeyword_start */
bool _gen_typescript_TrueKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_FalseKeyword_start */
bool _gen_typescript_FalseKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_NullKeyword_start */
bool _gen_typescript_NullKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("null\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_UndefinedKeyword_start */
bool _gen_typescript_UndefinedKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("undefined\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_AnyKeyword_start */
bool _gen_typescript_AnyKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("any\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_UnknownKeyword_start */
bool _gen_typescript_UnknownKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("unknown\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_NeverKeyword_start */
bool _gen_typescript_NeverKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("never\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_NumberKeyword_start */
bool _gen_typescript_NumberKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("number\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_BigIntKeyword_start */
bool _gen_typescript_BigIntKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("bigint\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_BooleanKeyword_start */
bool _gen_typescript_BooleanKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("boolean\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_StringKeyword_start */
bool _gen_typescript_StringKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("string\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_SymbolKeyword_start */
bool _gen_typescript_SymbolKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("symbol\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ObjectKeyword_start */
bool _gen_typescript_ObjectKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("object\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_KeyofKeyword_start */
bool _gen_typescript_KeyofKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("keyof\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ReadonlyKeyword_start */
bool _gen_typescript_ReadonlyKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("readonly\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_UniqueKeyword_start */
bool _gen_typescript_UniqueKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("unique\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_InferKeyword_start */
bool _gen_typescript_InferKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("infer\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ExtendsKeyword_start */
bool _gen_typescript_ExtendsKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("extends\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_NewKeyword_start */
bool _gen_typescript_NewKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("new\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_AbstractKeyword_start */
bool _gen_typescript_AbstractKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("abstract\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_IsKeyword_start */
bool _gen_typescript_IsKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("is\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_AssertsKeyword_start */
bool _gen_typescript_AssertsKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("asserts\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_AsKeyword_start */
bool _gen_typescript_AsKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("as\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_SatisfiesKeyword_start */
bool _gen_typescript_SatisfiesKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("satisfies\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ImportKeyword_start */
bool _gen_typescript_ImportKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("import\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_IfKeyword_start */
bool _gen_typescript_IfKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("if\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ElseKeyword_start */
bool _gen_typescript_ElseKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("else\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_WhileKeyword_start */
bool _gen_typescript_WhileKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("while\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_DoKeyword_start */
bool _gen_typescript_DoKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("do\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ForKeyword_start */
bool _gen_typescript_ForKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("for\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_OfKeyword_start */
bool _gen_typescript_OfKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("of\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ContinueKeyword_start */
bool _gen_typescript_ContinueKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("continue\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_BreakKeyword_start */
bool _gen_typescript_BreakKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("break\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ReturnKeyword_start */
bool _gen_typescript_ReturnKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("return\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ThrowKeyword_start */
bool _gen_typescript_ThrowKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("throw\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_SwitchKeyword_start */
bool _gen_typescript_SwitchKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("switch\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_CaseKeyword_start */
bool _gen_typescript_CaseKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("case\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_DefaultKeyword_start */
bool _gen_typescript_DefaultKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("default\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_TryKeyword_start */
bool _gen_typescript_TryKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("try\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_CatchKeyword_start */
bool _gen_typescript_CatchKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("catch\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_FinallyKeyword_start */
bool _gen_typescript_FinallyKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("finally\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_WithKeyword_start */
bool _gen_typescript_WithKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("with\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_DebuggerKeyword_start */
bool _gen_typescript_DebuggerKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("debugger\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_VarKeyword_start */
bool _gen_typescript_VarKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("var\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LetKeyword_start */
bool _gen_typescript_LetKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("let\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ConstKeyword_start */
bool _gen_typescript_ConstKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("const\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_FunctionKeyword_start */
bool _gen_typescript_FunctionKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("function\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ClassKeyword_start */
bool _gen_typescript_ClassKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("class\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_InterfaceKeyword_start */
bool _gen_typescript_InterfaceKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("interface\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_TypeKeyword_start */
bool _gen_typescript_TypeKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("type\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_EnumKeyword_start */
bool _gen_typescript_EnumKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("enum\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_NamespaceKeyword_start */
bool _gen_typescript_NamespaceKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("namespace\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ModuleKeyword_start */
bool _gen_typescript_ModuleKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("module\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_DeclareKeyword_start */
bool _gen_typescript_DeclareKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("declare\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_AsyncKeyword_start */
bool _gen_typescript_AsyncKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("async\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_PublicKeyword_start */
bool _gen_typescript_PublicKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("public\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_PrivateKeyword_start */
bool _gen_typescript_PrivateKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("private\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ProtectedKeyword_start */
bool _gen_typescript_ProtectedKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("protected\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_StaticKeyword_start */
bool _gen_typescript_StaticKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("static\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_OverrideKeyword_start */
bool _gen_typescript_OverrideKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("override\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_AccessorKeyword_start */
bool _gen_typescript_AccessorKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("accessor\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_GetKeyword_start */
bool _gen_typescript_GetKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("get\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_SetKeyword_start */
bool _gen_typescript_SetKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("set\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ConstructorKeyword_start */
bool _gen_typescript_ConstructorKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("constructor\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ImplementsKeyword_start */
bool _gen_typescript_ImplementsKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("implements\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ExportKeyword_start */
bool _gen_typescript_ExportKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("export\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_FromKeyword_start */
bool _gen_typescript_FromKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("from\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_RequireKeyword_start */
bool _gen_typescript_RequireKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("require\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_GlobalKeyword_start */
bool _gen_typescript_GlobalKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("global\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_AssertKeyword_start */
bool _gen_typescript_AssertKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("assert\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_DeferKeyword_start */
bool _gen_typescript_DeferKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("defer\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_UsingKeyword_start */
bool _gen_typescript_UsingKeyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("using\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Hashbang_start */
bool _gen_typescript_Hashbang_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#![^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_InvalidHashbang_start */
bool _gen_typescript_InvalidHashbang_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#![^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_BigIntLiteral_start */
bool _gen_typescript_BigIntLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(?:0[xX][0-9a-fA-F](?:_?[0-9a-fA-F])*|0[oO][0-7](?:_?[0-7])*|0[bB][01](?:_?[01])*|[0-9](?:_?[0-9])*)n\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_NumericLiteral_start */
bool _gen_typescript_NumericLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(?:0[xX][0-9a-fA-F](?:_?[0-9a-fA-F])*|0[oO][0-7](?:_?[0-7])*|0[bB][01](?:_?[01])*|(?:[0-9](?:_?[0-9])*)?\\.[0-9](?:_?[0-9])*(?:[eE][+-]?[0-9](?:_?[0-9])*)?|[0-9](?:_?[0-9])*\\.(?!\\.)|[0-9](?:_?[0-9])*(?:\\.[0-9](?:_?[0-9])*)?(?:[eE][+-]?[0-9](?:_?[0-9])*)?)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_StringLiteral_start */
bool _gen_typescript_StringLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'(?:[^'\\\\\\r\\n]|\\\\(?:\\r\\n|[\\s\\S]))*'|\"(?:[^\"\\\\\\r\\n]|\\\\(?:\\r\\n|[\\s\\S]))*\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_NoSubstitutionTemplateLiteral_start */
bool _gen_typescript_NoSubstitutionTemplateLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`(?:[^`\\\\$]|\\\\(?:\\r\\n|[\\s\\S])|\\$(?!\\{))*`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_TemplateHead_start */
bool _gen_typescript_TemplateHead_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`(?:[^`\\\\$]|\\\\(?:\\r\\n|[\\s\\S])|\\$(?!\\{))*\\$\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_TemplateMiddle_start */
bool _gen_typescript_TemplateMiddle_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}(?:[^`\\\\$]|\\\\(?:\\r\\n|[\\s\\S])|\\$(?!\\{))*\\$\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_TemplateTail_start */
bool _gen_typescript_TemplateTail_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}(?:[^`\\\\$]|\\\\(?:\\r\\n|[\\s\\S])|\\$(?!\\{))*`", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_InvalidNumericLiteral_start */
bool _gen_typescript_InvalidNumericLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(?:0[xX][0-9A-Za-z_]*|0[oO][0-9A-Za-z_]*|0[bB][0-9A-Za-z_]*|[0-9]+(?:__+[0-9_]*|_))(?:n)?\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_UnterminatedStringLiteral_start */
bool _gen_typescript_UnterminatedStringLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'(?:[^'\\\\\\r\\n]|\\\\[^\\r\\n])*(?=\\r|\\n|$)|\"(?:[^\"\\\\\\r\\n]|\\\\[^\\r\\n])*(?=\\r|\\n|$)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_UnterminatedTemplateLiteral_start */
bool _gen_typescript_UnterminatedTemplateLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("`(?:[^`\\\\$]|\\\\(?:\\r\\n|[\\s\\S])|\\$(?!\\{))*(?=$)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_RegularExpressionLiteral_start */
bool _gen_typescript_RegularExpressionLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("/(?:[^/\\\\\\r\\n\\[]|\\\\.|\\[(?:[^\\]\\\\\\r\\n]|\\\\.)*\\])+/[a-zA-Z]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_StrictEqual_start */
bool _gen_typescript_StrictEqual_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("===", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_StrictNotEqual_start */
bool _gen_typescript_StrictNotEqual_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("!==", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_UnsignedRightShift_start */
bool _gen_typescript_UnsignedRightShift_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(">>>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Exponent_start */
bool _gen_typescript_Exponent_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Increment_start */
bool _gen_typescript_Increment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\+\\+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Decrement_start */
bool _gen_typescript_Decrement_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("--", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Equal_start */
bool _gen_typescript_Equal_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("==", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_NotEqual_start */
bool _gen_typescript_NotEqual_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("!=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LessEqual_start */
bool _gen_typescript_LessEqual_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_GreaterEqual_start */
bool _gen_typescript_GreaterEqual_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(">=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LeftShift_start */
bool _gen_typescript_LeftShift_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<<", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_RightShift_start */
bool _gen_typescript_RightShift_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(">>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LogicalAnd_start */
bool _gen_typescript_LogicalAnd_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("&&", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LogicalOr_start */
bool _gen_typescript_LogicalOr_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\|\\|", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_NullishCoalesce_start */
bool _gen_typescript_NullishCoalesce_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\?\\?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_OptionalChain_start */
bool _gen_typescript_OptionalChain_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\?\\.(?![0-9])", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_PlusAssign_start */
bool _gen_typescript_PlusAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\+=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_MinusAssign_start */
bool _gen_typescript_MinusAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("-=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_MultiplyAssign_start */
bool _gen_typescript_MultiplyAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_DivideAssign_start */
bool _gen_typescript_DivideAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("/=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_RemainderAssign_start */
bool _gen_typescript_RemainderAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("%=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_ExponentAssign_start */
bool _gen_typescript_ExponentAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*\\*=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LeftShiftAssign_start */
bool _gen_typescript_LeftShiftAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<<=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_RightShiftAssign_start */
bool _gen_typescript_RightShiftAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(">>=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_UnsignedRightShiftAssign_start */
bool _gen_typescript_UnsignedRightShiftAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(">>>=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_BitAndAssign_start */
bool _gen_typescript_BitAndAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("&=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_BitOrAssign_start */
bool _gen_typescript_BitOrAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\|=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_BitXorAssign_start */
bool _gen_typescript_BitXorAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\^=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_NullishCoalesceAssign_start */
bool _gen_typescript_NullishCoalesceAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\?\\?=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LogicalAndAssign_start */
bool _gen_typescript_LogicalAndAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("&&=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LogicalOrAssign_start */
bool _gen_typescript_LogicalOrAssign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\|\\|=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Arrow_start */
bool _gen_typescript_Arrow_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("=>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Ellipsis_start */
bool _gen_typescript_Ellipsis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\.\\.\\.", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LParen_start */
bool _gen_typescript_LParen_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_RParen_start */
bool _gen_typescript_RParen_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LBracket_start */
bool _gen_typescript_LBracket_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_RBracket_start */
bool _gen_typescript_RBracket_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LBrace_start */
bool _gen_typescript_LBrace_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_RBrace_start */
bool _gen_typescript_RBrace_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Dot_start */
bool _gen_typescript_Dot_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\.", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Semicolon_start */
bool _gen_typescript_Semicolon_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(";", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Question_start */
bool _gen_typescript_Question_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Colon_start */
bool _gen_typescript_Colon_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(":", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Comma_start */
bool _gen_typescript_Comma_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(",", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Assign_start */
bool _gen_typescript_Assign_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("=", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Plus_start */
bool _gen_typescript_Plus_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Minus_start */
bool _gen_typescript_Minus_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("-", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Multiply_start */
bool _gen_typescript_Multiply_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Slash_start */
bool _gen_typescript_Slash_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Remainder_start */
bool _gen_typescript_Remainder_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("%", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LessThan_start */
bool _gen_typescript_LessThan_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_GreaterThan_start */
bool _gen_typescript_GreaterThan_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(">", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_BitAnd_start */
bool _gen_typescript_BitAnd_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("&", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_BitOr_start */
bool _gen_typescript_BitOr_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\|", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_BitXor_start */
bool _gen_typescript_BitXor_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\^", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LogicalNot_start */
bool _gen_typescript_LogicalNot_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("!", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_BitNot_start */
bool _gen_typescript_BitNot_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("~", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_At_start */
bool _gen_typescript_At_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("@", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_Identifier_start */
bool _gen_typescript_Identifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(?:[$_\\p{ID_Start}]|\\\\u(?:[0-9a-fA-F]{4}|\\{[0-9a-fA-F]{1,6}\\}))(?:[$\\x{200C}\\x{200D}\\p{ID_Continue}]|\\\\u(?:[0-9a-fA-F]{4}|\\{[0-9a-fA-F]{1,6}\\}))*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_PrivateIdentifier_start */
bool _gen_typescript_PrivateIdentifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("#(?:[$_\\p{ID_Start}]|\\\\u(?:[0-9a-fA-F]{4}|\\{[0-9a-fA-F]{1,6}\\}))(?:[$\\x{200C}\\x{200D}\\p{ID_Continue}]|\\\\u(?:[0-9a-fA-F]{4}|\\{[0-9a-fA-F]{1,6}\\}))*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_InvalidIdentifier_start */
bool _gen_typescript_InvalidIdentifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("(?:#)?\\\\u(?:[0-9a-fA-F]{4}|\\{[0-9a-fA-F]{1,6}\\})(?:[$\\x{200C}\\x{200D}\\p{ID_Continue}]|\\\\u(?:[0-9a-fA-F]{4}|\\{[0-9a-fA-F]{1,6}\\}))*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_JSXOpen_start */
bool _gen_typescript_JSXOpen_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_JSXCloseStart_start */
bool _gen_typescript_JSXCloseStart_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("</", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_JSXTagEnd_start */
bool _gen_typescript_JSXTagEnd_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(">", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_JSXClosingTagEnd_start */
bool _gen_typescript_JSXClosingTagEnd_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match(">", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_JSXSelfClosingEnd_start */
bool _gen_typescript_JSXSelfClosingEnd_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("/>", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_JSXExpressionStart_start */
bool _gen_typescript_JSXExpressionStart_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_JSXExpressionEnd_start */
bool _gen_typescript_JSXExpressionEnd_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_JSXExpressionLBrace_start */
bool _gen_typescript_JSXExpressionLBrace_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_JSXExpressionRBrace_start */
bool _gen_typescript_JSXExpressionRBrace_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_JSXIdentifier_start */
bool _gen_typescript_JSXIdentifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[$_\\p{L}\\p{Nl}][$_\\p{L}\\p{Nl}\\p{Mn}\\p{Mc}\\p{Nd}\\p{Pc}\\x{200C}\\x{200D}-]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_JSXText_start */
bool _gen_typescript_JSXText_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[^<{]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_TemplateLBrace_start */
bool _gen_typescript_TemplateLBrace_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_TemplateNestedRBrace_start */
bool _gen_typescript_TemplateNestedRBrace_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_WhitespaceTrivia_start */
bool _gen_typescript_WhitespaceTrivia_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[ \\t\\r\\n]+", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_LineCommentTrivia_start */
bool _gen_typescript_LineCommentTrivia_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("//[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_typescript_BlockCommentTrivia_start */
bool _gen_typescript_BlockCommentTrivia_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("/\\*(?:[^*]|\\*(?!/))*\\*/", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_LineComment_start */
bool _gen_vb_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'[^\\r\\n]*|rem\\s.*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_Keyword_start */
bool _gen_vb_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("addhandler\\b|addressof\\b|alias\\b|and\\b|andalso\\b|as\\b|byref\\b|byval\\b|call\\b|case\\b|catch\\b|class\\b|const\\b|continue\\b|declare\\b|default\\b|delegate\\b|dim\\b|do\\b|each\\b|else\\b|elseif\\b|end\\b|endif\\b|enum\\b|erase\\b|error\\b|event\\b|exit\\b|finally\\b|for\\b|friend\\b|function\\b|get\\b|global\\b|gosub\\b|goto\\b|handles\\b|if\\b|implements\\b|imports\\b|in\\b|inherits\\b|interface\\b|is\\b|isnot\\b|lib\\b|like\\b|loop\\b|me\\b|mod\\b|module\\b|mustinherit\\b|mustoverride\\b|mybase\\b|myclass\\b|namespace\\b|narrowing\\b|next\\b|new\\b|not\\b|nothing\\b|notinheritable\\b|notoverridable\\b|of\\b|off\\b|on\\b|operator\\b|option\\b|optional\\b|or\\b|orelse\\b|overloads\\b|overridable\\b|overrides\\b|paramarray\\b|partial\\b|preserve\\b|private\\b|property\\b|protected\\b|public\\b|raiseevent\\b|readonly\\b|redim\\b|rem\\b|removehandler\\b|resume\\b|return\\b|select\\b|set\\b|shadows\\b|shared\\b|static\\b|step\\b|stop\\b|structure\\b|sub\\b|synclock\\b|then\\b|throw\\b|to\\b|try\\b|typeof\\b|unicode\\b|until\\b|using\\b|when\\b|while\\b|widening\\b|with\\b|withevents\\b|writeonly\\b|xor\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_DataType_start */
bool _gen_vb_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("boolean\\b|byte\\b|char\\b|cdate\\b|date\\b|decimal\\b|double\\b|integer\\b|long\\b|object\\b|sbyte\\b|short\\b|single\\b|string\\b|uinteger\\b|ulong\\b|ushort\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_Boolean_start */
bool _gen_vb_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_DoubleString_start */
bool _gen_vb_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_DoubleString_end */
bool _gen_vb_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_EscapedQuote_start */
bool _gen_vb_EscapedQuote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_Number_start */
bool _gen_vb_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("&H[0-9a-fA-F]+|&O[0-7]+|[0-9]+(?:\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?[!#%&@]?", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_Variable_start */
bool _gen_vb_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_Operator_start */
bool _gen_vb_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("<<|>>|<=|>=|<>|:=|\\+=|\\-=|[=<>+\\-*/\\\\^&.,;:]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_Parenthesis_start */
bool _gen_vb_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_Parenthesis_end */
bool _gen_vb_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_ArrayIndex_start */
bool _gen_vb_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_ArrayIndex_end */
bool _gen_vb_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_CodeBlock_start */
bool _gen_vb_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_vb_CodeBlock_end */
bool _gen_vb_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_LineComment_start */
bool _gen_zig_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\/\\/[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_MultiLineString_start */
bool _gen_zig_MultiLineString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\[^\\r\\n]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_Directive_start */
bool _gen_zig_Directive_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("@[a-zA-Z_][a-zA-Z0-9_]*\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_Keyword_start */
bool _gen_zig_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("fn\\b|const\\b|var\\b|pub\\b|usingnamespace\\b|align\\b|allowzero\\b|and\\b|anyframe\\b|anytype\\b|asm\\b|async\\b|await\\b|break\\b|cancel\\b|catch\\b|comptime\\b|defer\\b|errdefer\\b|enum\\b|export\\b|extern\\b|for\\b|if\\b|inline\\b|noalias\\b|noinline\\b|nosuspend\\b|or\\b|opaque\\b|packed\\b|resume\\b|return\\b|linksection\\b|struct\\b|suspend\\b|switch\\b|test\\b|threadlocal\\b|try\\b|union\\b|unreachable\\b|volatile\\b|while\\b|orelse\\b|void\\b|noreturn\\b|type\\b|anyerror\\b|bool\\b|f16\\b|f32\\b|f64\\b|f128\\b|c_short\\b|c_ushort\\b|c_int\\b|c_uint\\b|c_long\\b|c_ulong\\b|c_longlong\\b|c_ulonglong\\b|c_longdouble\\b|c_void\\b|isize\\b|usize\\b|i8\\b|u8\\b|i16\\b|u16\\b|i32\\b|u32\\b|i64\\b|u64\\b|i128\\b|u128\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_Boolean_start */
bool _gen_zig_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("true\\b|false\\b|null\\b|undefined\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_Variable_start */
bool _gen_zig_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("[a-zA-Z_][a-zA-Z0-9_]*", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_CodeBlock_start */
bool _gen_zig_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\{", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_CodeBlock_end */
bool _gen_zig_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_Operator_start */
bool _gen_zig_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("::|:=|:|===|==|!=|<=|>=|&&|\\|\\||\\+=|\\-=|\\*=|\\/=|%=|&=|\\^=|\\|=|<<=|>>=|->|<<|>>|[=<>!&|^~+\\-*/%\\?:;.,]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_DoubleString_start */
bool _gen_zig_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_DoubleString_end */
bool _gen_zig_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\"", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_SingleString_start */
bool _gen_zig_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_SingleString_end */
bool _gen_zig_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("'", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_StringEscape_start */
bool _gen_zig_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\\\\\\\|\\\\\\\"|\\\\\\'|\\\\n|\\\\r|\\\\t|\\\\u[0-9a-fA-F]{4}|\\\\x[0-9a-fA-F]{2}", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_Number_start */
bool _gen_zig_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("0[xX][0-9a-fA-F_]+\\b|0[bB][01_]+\\b|0[oO][0-7_]+\\b|[0-9_]*\\.?[0-9_]+(?:[eE][-+]?[0-9_]+)?\\b", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_Parenthesis_start */
bool _gen_zig_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\(", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_Parenthesis_end */
bool _gen_zig_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\)", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_ArrayIndex_start */
bool _gen_zig_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\[", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}

/* _gen_zig_ArrayIndex_end */
bool _gen_zig_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start) {
    static _Thread_local adv_regex_context *ctx;
    static _Thread_local void *compiled;
    static _Thread_local int compiled_encoding = -1;
    return native_regex_match("\\]", encoding, start, max_len, offset, length, is_caseless, only_at_start, &ctx, &compiled, &compiled_encoding);
}
