#pragma once

#include "textparser.h"

#ifdef __cplusplus
extern "C" {
#endif

/* C Grammar Matchers */
/* _gen_c_LineComment_start_XC9cL1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_c_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_c_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_c_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_Preprocessor_start_I1sgXHRdKlthLXpBLVpfXVthLXpBLVowLTlfXSo= */
EXPORT_TEXTPARSER bool _gen_c_Preprocessor_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_Keyword_start_aW50XGJ8Y2hhclxifGRvdWJsZVxifGZsb2F0XGJ8dm9pZFxifHNob3J0XGJ8bG9uZ1xifHVuc2lnbmVkXGJ8c2lnbmVkXGJ8c3RydWN0XGJ8dW5pb25cYnxlbnVtXGJ8dHlwZWRlZlxifGNvbnN0XGJ8c3RhdGljXGJ8ZXh0ZXJuXGJ8dm9sYXRpbGVcYnxpZlxifGVsc2VcYnxmb3JcYnx3aGlsZVxifGRvXGJ8c3dpdGNoXGJ8Y2FzZVxifGRlZmF1bHRcYnxicmVha1xifGNvbnRpbnVlXGJ8cmV0dXJuXGJ8c2l6ZW9mXGJ8Z290b1xifHJlZ2lzdGVyXGJ8YXV0b1xifGlubGluZVxifHJlc3RyaWN0XGJ8X0Jvb2xcYnxib29sXGJ8c2l6ZV90XGJ8c3NpemVfdFxifHVpbnQ4X3RcYnx1aW50MTZfdFxifHVpbnQzMl90XGJ8dWludDY0X3RcYnxpbnQ4X3RcYnxpbnQxNl90XGJ8aW50MzJfdFxifGludDY0X3RcYnx1aW50cHRyX3RcYnxpbnRwdHJfdFxifHB0cmRpZmZfdFxi */
EXPORT_TEXTPARSER bool _gen_c_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_c_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_c_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_c_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_Operator_start_XCs9fFwtPXxcKj18XC89fCU9fCY9fFxePXxcfD18PHsyfT18PnsyfT18PHsyfXw+ezJ9fFwrXCt8XC1cLXwmJnxcfFx8fDw9fD49fD09fCE9fC0+fFwufFs9PD4hJnxefitcLSovJVw/OjsuLF0= */
EXPORT_TEXTPARSER bool _gen_c_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_c_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_c_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_c_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_c_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHR8XFx1WzAtOWEtZkEtRl17NH18XFx4WzAtOWEtZkEtRl17Mn0= */
EXPORT_TEXTPARSER bool _gen_c_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_Number_start_WzAtOV0qXC4/WzAtOV0rKD86W2VFXVstK10/WzAtOV0rKT9bZkZkRGxMXT8= */
EXPORT_TEXTPARSER bool _gen_c_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_c_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_c_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_c_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_c_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_c_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_TypeCast_start_XCg= */
EXPORT_TEXTPARSER bool _gen_c_TypeCast_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* _gen_c_TypeCast_end_XCk= */
EXPORT_TEXTPARSER bool _gen_c_TypeCast_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* === JSON Grammar Matchers === */
/* _gen_json_Object_start_ew== */
EXPORT_TEXTPARSER bool _gen_json_Object_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_json_Object_end_fQ== */
EXPORT_TEXTPARSER bool _gen_json_Object_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_json_Array_start_XFs= */
EXPORT_TEXTPARSER bool _gen_json_Array_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_json_Array_end_XF0= */
EXPORT_TEXTPARSER bool _gen_json_Array_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_json_Key_start_XCIoPz1bXlwiXFxcXF0qKD86XFxcXC5bXlwiXFxcXF0qKSpcIlxzKjop */
EXPORT_TEXTPARSER bool _gen_json_Key_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_json_Key_end_KFwiKVxzKjo= */
EXPORT_TEXTPARSER bool _gen_json_Key_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_json_String_start_XCI= */
EXPORT_TEXTPARSER bool _gen_json_String_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_json_String_end_XCI= */
EXPORT_TEXTPARSER bool _gen_json_String_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_json_Number_start_XGQrKD86XC5cZCspPyg/OmVbKy1dP1xkKyk/ */
EXPORT_TEXTPARSER bool _gen_json_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_json_StringEscape_start_XFxcXHxcXFwvfFxcYnxcXGZ8XFxufFxccnxcXHR8XFx1WzAtOWEtZl17NH0= */
EXPORT_TEXTPARSER bool _gen_json_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_json_Bool_start_dHJ1ZXxmYWxzZQ== */
EXPORT_TEXTPARSER bool _gen_json_Bool_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_json_Null_start_bnVsbA== */
EXPORT_TEXTPARSER bool _gen_json_Null_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_json_ValueSeparator_start_LA== */
EXPORT_TEXTPARSER bool _gen_json_ValueSeparator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_json_KeyValueSeparator_start_Og== */
EXPORT_TEXTPARSER bool _gen_json_KeyValueSeparator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* === HTML Grammar Matchers === */
/* _gen_html_Comment_start_PCEtLQ== */
EXPORT_TEXTPARSER bool _gen_html_Comment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_html_Comment_end_LS0+ */
EXPORT_TEXTPARSER bool _gen_html_Comment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_html_Doctype_start_PCFkb2N0eXBlXGJbXj5dKj4= */
EXPORT_TEXTPARSER bool _gen_html_Doctype_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_html_ClosingTag_start_PFwvW2EtekEtWjAtOTotXStccyo+ */
EXPORT_TEXTPARSER bool _gen_html_ClosingTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_html_Tag_start_PFthLXpBLVowLTk6LV0r */
EXPORT_TEXTPARSER bool _gen_html_Tag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_html_Tag_end_XC8/Pg== */
EXPORT_TEXTPARSER bool _gen_html_Tag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_html_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_html_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_html_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_html_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_html_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_html_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_html_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_html_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_html_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHQ= */
EXPORT_TEXTPARSER bool _gen_html_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_html_Equal_start_PQ== */
EXPORT_TEXTPARSER bool _gen_html_Equal_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_html_AttributeName_start_W2EtekEtWjAtOTotXSs= */
EXPORT_TEXTPARSER bool _gen_html_AttributeName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* === CSS Grammar Matchers === */
/* _gen_css_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_css_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_css_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_AtRule_start_QFthLXpBLVotXSs= */
EXPORT_TEXTPARSER bool _gen_css_AtRule_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_ClassName_start_XC5bYS16QS1aXy1dW2EtekEtWjAtOV8tXSo= */
EXPORT_TEXTPARSER bool _gen_css_ClassName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_IdName_start_I1thLXpBLVpfLV1bYS16QS1aMC05Xy1dKg== */
EXPORT_TEXTPARSER bool _gen_css_IdName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_PseudoClass_start_OnsxLDJ9W2EtekEtWi1dKw== */
EXPORT_TEXTPARSER bool _gen_css_PseudoClass_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_TagName_start_W2EtekEtWi1dW2EtekEtWjAtOS1dKg== */
EXPORT_TEXTPARSER bool _gen_css_TagName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_Operator_start_Wyw+K34qXQ== */
EXPORT_TEXTPARSER bool _gen_css_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_css_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_css_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_Declaration_start_W2EtekEtWi1dW2EtekEtWjAtOS1dKlxzKjo= */
EXPORT_TEXTPARSER bool _gen_css_Declaration_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_Declaration_end_Ow== */
EXPORT_TEXTPARSER bool _gen_css_Declaration_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_HexColor_start_I1swLTlhLWZBLUZdezMsOH0= */
EXPORT_TEXTPARSER bool _gen_css_HexColor_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_Number_start_WzAtOV0qXC4/WzAtOV0rKD86W2EtekEtWiVdKyk/ */
EXPORT_TEXTPARSER bool _gen_css_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_FunctionCall_start_dXJsXChbXildKlwpfHJnYlwoW14pXSpcKXxyZ2JhXChbXildKlwpfGhzbFwoW14pXSpcKXxoc2xhXChbXildKlwpfHZhclwoW14pXSpcKXxjYWxjXChbXildKlwp */
EXPORT_TEXTPARSER bool _gen_css_FunctionCall_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_css_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_css_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_css_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_css_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHQ= */
EXPORT_TEXTPARSER bool _gen_css_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_Important_start_IWltcG9ydGFudFxi */
EXPORT_TEXTPARSER bool _gen_css_Important_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_Value_start_W2EtekEtWi1dW2EtekEtWjAtOS1dKg== */
EXPORT_TEXTPARSER bool _gen_css_Value_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_DeclOperator_start_WywvIV0= */
EXPORT_TEXTPARSER bool _gen_css_DeclOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_css_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_css_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_css_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_css_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_css_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);


/* === CFML Grammar Matchers === */
/* _gen_cfml_ScriptStartTag_start_PGNmc2NyaXB0KD89W1w+XHNdKQ== */
EXPORT_TEXTPARSER bool _gen_cfml_ScriptStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_ScriptStartTag_end_XC8/Pg== */
EXPORT_TEXTPARSER bool _gen_cfml_ScriptStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_ScriptEndTag_start_PFwvY2ZzY3JpcHQ+ */
EXPORT_TEXTPARSER bool _gen_cfml_ScriptEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_OutputStartTag_start_PGNmb3V0cHV0KD89W1w+XHNdKQ== */
EXPORT_TEXTPARSER bool _gen_cfml_OutputStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_OutputStartTag_end_XC8/Pg== */
EXPORT_TEXTPARSER bool _gen_cfml_OutputStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_OutputEndTag_start_PFwvY2ZvdXRwdXQ+ */
EXPORT_TEXTPARSER bool _gen_cfml_OutputEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_QueryStartTag_start_PGNmcXVlcnkoPz1bXD5cc10p */
EXPORT_TEXTPARSER bool _gen_cfml_QueryStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_QueryStartTag_end_XC8/Pg== */
EXPORT_TEXTPARSER bool _gen_cfml_QueryStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_QueryEndTag_start_PFwvY2ZxdWVyeT4= */
EXPORT_TEXTPARSER bool _gen_cfml_QueryEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_LoopStartTag_start_PGNmbG9vcCg/PVtcPlxzXSk= */
EXPORT_TEXTPARSER bool _gen_cfml_LoopStartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_LoopStartTag_end_XC8/Pg== */
EXPORT_TEXTPARSER bool _gen_cfml_LoopStartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_LoopEndTag_start_PFwvY2Zsb29wPg== */
EXPORT_TEXTPARSER bool _gen_cfml_LoopEndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_StartTag_start_PGNmW2EtejAtOV9dKw== */
EXPORT_TEXTPARSER bool _gen_cfml_StartTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_StartTag_end_XC8/Pg== */
EXPORT_TEXTPARSER bool _gen_cfml_StartTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_EndTag_start_PFwvY2YoPyFvdXRwdXQpKD8hc2NyaXB0KSg/IXF1ZXJ5KSg/IWxvb3ApW2EtejAtOV9dKw== */
EXPORT_TEXTPARSER bool _gen_cfml_EndTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_EndTag_end_XC8/Pg== */
EXPORT_TEXTPARSER bool _gen_cfml_EndTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_Comment_start_PCEtLS0= */
EXPORT_TEXTPARSER bool _gen_cfml_Comment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_Comment_end_LS0tPg== */
EXPORT_TEXTPARSER bool _gen_cfml_Comment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_cfml_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_cfml_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_cfml_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_cfml_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_SingleChar_start_Jyc= */
EXPORT_TEXTPARSER bool _gen_cfml_SingleChar_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_DoubleChar_start_IiI= */
EXPORT_TEXTPARSER bool _gen_cfml_DoubleChar_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_SharpChar_start_IyM= */
EXPORT_TEXTPARSER bool _gen_cfml_SharpChar_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_SharpExpression_start_Iw== */
EXPORT_TEXTPARSER bool _gen_cfml_SharpExpression_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_SharpExpression_end_Iw== */
EXPORT_TEXTPARSER bool _gen_cfml_SharpExpression_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_ScriptBlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_cfml_ScriptBlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_ScriptBlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_cfml_ScriptBlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_ScriptLineComment_start_XC9cLy4qW15cclxuXQ== */
EXPORT_TEXTPARSER bool _gen_cfml_ScriptLineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_ExpressionEnd_start_Ow== */
EXPORT_TEXTPARSER bool _gen_cfml_ExpressionEnd_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_Number_start_KFswLTldK1wuP1swLTldKig/OltlRV1bLStdP1swLTldKyk/fFswLTldKlwuWzAtOV0rKD86W2VFXVstK10/WzAtOV0rKT8p */
EXPORT_TEXTPARSER bool _gen_cfml_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_Boolean_start_dHJ1ZVxifGZhbHNlXGJ8eWVzXGJ8bm9cYg== */
EXPORT_TEXTPARSER bool _gen_cfml_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_ObjectMember_start_XC4= */
EXPORT_TEXTPARSER bool _gen_cfml_ObjectMember_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_Function_start_KFthLXpfXStbYS16MC05X10qKVtcc10qXCg= */
EXPORT_TEXTPARSER bool _gen_cfml_Function_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_Separator_start_LA== */
EXPORT_TEXTPARSER bool _gen_cfml_Separator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_Variable_start_W2Etel9cJF0rW2EtejAtOV9cJF0q */
EXPORT_TEXTPARSER bool _gen_cfml_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_AssignOperator_start_XCs9fFwtPXxcKj18XC89fCU9fCY9fD0= */
EXPORT_TEXTPARSER bool _gen_cfml_AssignOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_TernaryOperator_start_XD86fFw/fFw6 */
EXPORT_TEXTPARSER bool _gen_cfml_TernaryOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_LogicalImpOperator_start_aW1wXGI= */
EXPORT_TEXTPARSER bool _gen_cfml_LogicalImpOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_LogicalEqvOperator_start_ZXF2XGI= */
EXPORT_TEXTPARSER bool _gen_cfml_LogicalEqvOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_LogicalXorOperator_start_eG9yXGI= */
EXPORT_TEXTPARSER bool _gen_cfml_LogicalXorOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_LogicalOrOperator_start_XHxcfHxvclxi */
EXPORT_TEXTPARSER bool _gen_cfml_LogicalOrOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_LogicalAndOperator_start_XCZcJnxhbmRcYg== */
EXPORT_TEXTPARSER bool _gen_cfml_LogicalAndOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_LogicalNotOperator_start_bm90XGJ8IQ== */
EXPORT_TEXTPARSER bool _gen_cfml_LogicalNotOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_CompareOperator_start_Z3JlYXRlclxzK3RoYW5ccytvclxzK2VxdWFsXHMrdG9cYnxsZXNzXHMrdGhhblxzK29yXHMrZXF1YWxccyt0b1xifGRvZXNccytub3Rccytjb250YWluXGJ8aXNccytub3RcYnxjb250YWluc1xifGxlc3Nccyt0aGFuXGJ8Z3JlYXRlclxzK3RoYW5cYnxub3RccytlcXVhbFxifGVxdWFsXGJ8bmVxXGJ8bHRlXGJ8Z3RlXGJ8ZXFcYnw9PXw+PXw8PXwhPXxnZVxifGx0XGJ8Z3RcYnxsZVxifFxiaXNcYnw+fDw= */
EXPORT_TEXTPARSER bool _gen_cfml_CompareOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_ConcatOperator_start_XCY= */
EXPORT_TEXTPARSER bool _gen_cfml_ConcatOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_AddOperator_start_XCt8LQ== */
EXPORT_TEXTPARSER bool _gen_cfml_AddOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_MulOperator_start_XCp8XC98XFx8bW9kXGJ8XCU= */
EXPORT_TEXTPARSER bool _gen_cfml_MulOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_PowerOperator_start_XF4= */
EXPORT_TEXTPARSER bool _gen_cfml_PowerOperator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_cfml_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_cfml_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_Keyword_start_dmFyXGJ8ZnVuY3Rpb25cYnx0aGlzXGJ8dHJ5XGJ8Y2F0Y2hcYnxpZlxifHRoZW5cYnxlbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_cfml_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_cfml_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_cfml_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_cfml_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cfml_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_cfml_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);


/* === CPP Grammar Matchers === */
/* _gen_cpp_LineComment_start_XC9cL1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_cpp_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_cpp_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_cpp_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_Preprocessor_start_I1sgXHRdKlthLXpBLVpfXVthLXpBLVowLTlfXSo= */
EXPORT_TEXTPARSER bool _gen_cpp_Preprocessor_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_Keyword_start_Y2xhc3NcYnxuYW1lc3BhY2VcYnx0ZW1wbGF0ZVxifHR5cGVuYW1lXGJ8dXNpbmdcYnxwdWJsaWNcYnxwcml2YXRlXGJ8cHJvdGVjdGVkXGJ8dmlydHVhbFxifG92ZXJyaWRlXGJ8ZmluYWxcYnxmcmllbmRcYnxvcGVyYXRvclxifHRoaXNcYnxuZXdcYnxkZWxldGVcYnx0aHJvd1xifGNhdGNoXGJ8dHJ5XGJ8Y29uc3RleHByXGJ8Y29uc3RldmFsXGJ8Y29uc3Rpbml0XGJ8ZGVjbHR5cGVcYnxleHBsaWNpdFxifGV4cG9ydFxifGltcG9ydFxifG1vZHVsZVxifG11dGFibGVcYnxub2V4Y2VwdFxifG51bGxwdHJcYnxzdGF0aWNfY2FzdFxifGR5bmFtaWNfY2FzdFxifGNvbnN0X2Nhc3RcYnxyZWludGVycHJldF9jYXN0XGJ8dGhyZWFkX2xvY2FsXGJ8Y29uY2VwdFxifHJlcXVpcmVzXGJ8Y29fYXdhaXRcYnxjb19yZXR1cm5cYnxjb195aWVsZFxifGNoYXI4X3RcYnxjaGFyMTZfdFxifGNoYXIzMl90XGJ8d2NoYXJfdFxifGludFxifGNoYXJcYnxkb3VibGVcYnxmbG9hdFxifHZvaWRcYnxzaG9ydFxifGxvbmdcYnx1bnNpZ25lZFxifHNpZ25lZFxifHN0cnVjdFxifHVuaW9uXGJ8ZW51bVxifHR5cGVkZWZcYnxjb25zdFxifHN0YXRpY1xifGV4dGVyblxifHZvbGF0aWxlXGJ8aWZcYnxlbHNlXGJ8Zm9yXGJ8d2hpbGVcYnxkb1xifHN3aXRjaFxifGNhc2VcYnxkZWZhdWx0XGJ8YnJlYWtcYnxjb250aW51ZVxifHJldHVyblxifHNpemVvZlxifGdvdG9cYnxyZWdpc3RlclxifGF1dG9cYnxpbmxpbmVcYnxyZXN0cmljdFxifF9Cb29sXGJ8Ym9vbFxifHNpemVfdFxifHNzaXplX3RcYnx1aW50OF90XGJ8dWludDE2X3RcYnx1aW50MzJfdFxifHVpbnQ2NF90XGJ8aW50OF90XGJ8aW50MTZfdFxifGludDMyX3RcYnxpbnQ2NF90XGJ8dWludHB0cl90XGJ8aW50cHRyX3RcYnxwdHJkaWZmX3RcYg== */
EXPORT_TEXTPARSER bool _gen_cpp_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_cpp_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_cpp_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_cpp_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_ScopeResolution_start_Ojo= */
EXPORT_TEXTPARSER bool _gen_cpp_ScopeResolution_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_Operator_start_XCs9fFwtPXxcKj18XC89fCU9fCY9fFxePXxcfD18PHsyfT18PnsyfT18PHsyfXw+ezJ9fFwrXCt8XC1cLXwmJnxcfFx8fDw9Pnw8PXw+PXw9PXwhPXwtPlwqfC0+fFwuXCp8XC58Wz0hfF5+K1wtLyVcPzo7Ll0= */
EXPORT_TEXTPARSER bool _gen_cpp_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_PointerOrRef_start_WyomXQ== */
EXPORT_TEXTPARSER bool _gen_cpp_PointerOrRef_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_TemplateOpen_start_PA== */
EXPORT_TEXTPARSER bool _gen_cpp_TemplateOpen_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_TemplateClose_start_Pg== */
EXPORT_TEXTPARSER bool _gen_cpp_TemplateClose_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_Comma_start_LA== */
EXPORT_TEXTPARSER bool _gen_cpp_Comma_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_cpp_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_cpp_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_cpp_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_cpp_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHR8XFx1WzAtOWEtZkEtRl17NH18XFx4WzAtOWEtZkEtRl17Mn0= */
EXPORT_TEXTPARSER bool _gen_cpp_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_Number_start_KD86MFt4WF1bMC05YS1mQS1GXStbdVVsTF0qfDBbYkJdWzAxXStbdVVsTF0qfFswLTldKlwuP1swLTldKyg/OltlRV1bLStdP1swLTldKyk/W2ZGZERsTF0/KQ== */
EXPORT_TEXTPARSER bool _gen_cpp_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_cpp_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_cpp_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_cpp_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_cpp_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_cpp_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_TemplateGroup_start_PA== */
EXPORT_TEXTPARSER bool _gen_cpp_TemplateGroup_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_TemplateGroup_end_Pg== */
EXPORT_TEXTPARSER bool _gen_cpp_TemplateGroup_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_TypeCast_start_XCg= */
EXPORT_TEXTPARSER bool _gen_cpp_TypeCast_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_cpp_TypeCast_end_XCk= */
EXPORT_TEXTPARSER bool _gen_cpp_TypeCast_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);


/* === PYTHON Grammar Matchers === */
/* _gen_python_LineComment_start_I1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_python_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_TripleSingleString_start_Jycn */
EXPORT_TEXTPARSER bool _gen_python_TripleSingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_TripleSingleString_end_Jycn */
EXPORT_TEXTPARSER bool _gen_python_TripleSingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_TripleDoubleString_start_IiIi */
EXPORT_TEXTPARSER bool _gen_python_TripleDoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_TripleDoubleString_end_IiIi */
EXPORT_TEXTPARSER bool _gen_python_TripleDoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_python_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_python_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_python_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_python_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_FString_start_W2ZGXSg/OiIiInwnJyd8InwnKQ== */
EXPORT_TEXTPARSER bool _gen_python_FString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_FString_end_KD86IiIifCcnJ3wifCcp */
EXPORT_TEXTPARSER bool _gen_python_FString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_FStringInterpolation_start_XHs= */
EXPORT_TEXTPARSER bool _gen_python_FStringInterpolation_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_FStringInterpolation_end_XH0= */
EXPORT_TEXTPARSER bool _gen_python_FStringInterpolation_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_StringEscape_start_XFxbXFxcIiducnQwXXxcXHhbMC05YS1mQS1GXXsyfXxcXHVbMC05YS1mQS1GXXs0fXxcXFVbMC05YS1mQS1GXXs4fXxcXE5ce1thLXpBLVpfXVthLXpBLVowLTlfXSpcfQ== */
EXPORT_TEXTPARSER bool _gen_python_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_Keyword_start_Tm9uZVxifGFuZFxifGFzXGJ8YXNzZXJ0XGJ8YXN5bmNcYnxhd2FpdFxifGJyZWFrXGJ8Y2xhc3NcYnxjb250aW51ZVxifGRlZlxifGRlbFxifGVsaWZcYnxlbHNlXGJ8ZXhjZXB0XGJ8ZmluYWxseVxifGZvclxifGZyb21cYnxnbG9iYWxcYnxpZlxifGltcG9ydFxifGluXGJ8aXNcYnxsYW1iZGFcYnxub25sb2NhbFxifG5vdFxifG9yXGJ8cGFzc1xifHJhaXNlXGJ8cmV0dXJuXGJ8dHJ5XGJ8d2hpbGVcYnx3aXRoXGJ8eWllbGRcYg== */
EXPORT_TEXTPARSER bool _gen_python_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_Boolean_start_VHJ1ZVxifEZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_python_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_Number_start_MFt4WF1bMC05YS1mQS1GXVswLTlhLWZBLUZfXSp8MFtvT11bMC03XVswLTdfXSp8MFtiQl1bMDFdWzAxX10qfFswLTldWzAtOV9dKig/OlwuWzAtOV1bMC05X10qKT8oPzpbZUVdWy0rXT9bMC05XVswLTlfXSopP1tqSl0/ */
EXPORT_TEXTPARSER bool _gen_python_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_Variable_start_QD9bYS16QS1aX11bYS16QS1aMC05X10q */
EXPORT_TEXTPARSER bool _gen_python_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_Operator_start_Oj18XC5cLlwufC0+fC8vfFwqXCp8PDx8Pj58PT18IT18PD18Pj18XCs9fFwtPXxcKj18XC89fC8vPXwlPXxAPXwmPXxcfD18XF49fDw8PXw+Pj18XCpcKj18Wz08PiEmfF5+K1wtKi8lQC4sOzpd */
EXPORT_TEXTPARSER bool _gen_python_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_python_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_python_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_python_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_python_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_python_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_python_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_python_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);


/* === JAVASCRIPT Grammar Matchers === */
/* _gen_javascript_LineComment_start_XC9cL1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_javascript_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_javascript_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_javascript_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_Keyword_start_YXN5bmNcYnxhd2FpdFxifGNsYXNzXGJ8Y29uc3RcYnxsZXRcYnx2YXJcYnxmdW5jdGlvblxifGltcG9ydFxifGV4cG9ydFxifGZyb21cYnxkZWZhdWx0XGJ8ZXh0ZW5kc1xifHN1cGVyXGJ8dGhpc1xifG5ld1xifHJldHVyblxifHRyeVxifGNhdGNoXGJ8ZmluYWxseVxifHRocm93XGJ8aWZcYnxlbHNlXGJ8c3dpdGNoXGJ8Y2FzZVxifGJyZWFrXGJ8Y29udGludWVcYnxkb1xifHdoaWxlXGJ8Zm9yXGJ8aW5cYnxvZlxifHR5cGVvZlxifGluc3RhbmNlb2ZcYnx5aWVsZFxifGRlYnVnZ2VyXGJ8ZGVsZXRlXGJ8dm9pZFxifHdpdGhcYnxudWxsXGJ8dW5kZWZpbmVkXGI= */
EXPORT_TEXTPARSER bool _gen_javascript_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_javascript_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_Variable_start_W2EtekEtWl8kXVthLXpBLVowLTlfJF0q */
EXPORT_TEXTPARSER bool _gen_javascript_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_javascript_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_javascript_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_Regex_start_XC8oPzpbXlwvXFwNCl18XFwuKStcL1thLXpBLVpdKg== */
EXPORT_TEXTPARSER bool _gen_javascript_Regex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_Operator_start_PT09fD09fCE9PXwhPXw9PnxcKz18XC09fFwqPXxcLz18JT18XCtcK3xcLVwtfCYmfFx8XHx8PD18Pj18XD9cLnxcP1w/fFs9PD4hJnxefitcLSovJVw/OjsuLF0= */
EXPORT_TEXTPARSER bool _gen_javascript_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_javascript_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_javascript_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_javascript_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_javascript_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_TemplateString_start_YA== */
EXPORT_TEXTPARSER bool _gen_javascript_TemplateString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_TemplateString_end_YA== */
EXPORT_TEXTPARSER bool _gen_javascript_TemplateString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHR8XFxcYHxcXHVbMC05YS1mQS1GXXs0fXxcXHhbMC05YS1mQS1GXXsyfQ== */
EXPORT_TEXTPARSER bool _gen_javascript_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_Number_start_MFt4WF1bMC05YS1mQS1GXSt8MFtvT11bMC03XSt8MFtiQl1bMDFdK3xbMC05XSpcLj9bMC05XSsoPzpbZUVdWy0rXT9bMC05XSspPw== */
EXPORT_TEXTPARSER bool _gen_javascript_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_javascript_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_javascript_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_javascript_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_javascript_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_javascript_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);


/* === RUST Grammar Matchers === */
/* _gen_rust_LineComment_start_XC9cL1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_rust_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_rust_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_rust_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_Attribute_start_IyE/XFs= */
EXPORT_TEXTPARSER bool _gen_rust_Attribute_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_Attribute_end_XF0= */
EXPORT_TEXTPARSER bool _gen_rust_Attribute_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_Macro_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKiE= */
EXPORT_TEXTPARSER bool _gen_rust_Macro_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_Keyword_start_YXNcYnxhc3luY1xifGF3YWl0XGJ8YnJlYWtcYnxjb25zdFxifGNvbnRpbnVlXGJ8Y3JhdGVcYnxkeW5cYnxlbHNlXGJ8ZW51bVxifGV4dGVyblxifGZuXGJ8Zm9yXGJ8aWZcYnxpbXBsXGJ8aW5cYnxsZXRcYnxsb29wXGJ8bWF0Y2hcYnxtb2RcYnxtb3ZlXGJ8bXV0XGJ8cHViXGJ8cmVmXGJ8cmV0dXJuXGJ8U2VsZlxifHNlbGZcYnxzdGF0aWNcYnxzdHJ1Y3RcYnxzdXBlclxifHRyYWl0XGJ8dHlwZVxifHVuc2FmZVxifHVzZVxifHdoZXJlXGJ8d2hpbGVcYnx5aWVsZFxifG1hY3JvXGJ8Ym9vbFxifGNoYXJcYnxzdHJcYnxpOFxifHU4XGJ8aTE2XGJ8dTE2XGJ8aTMyXGJ8dTMyXGJ8aTY0XGJ8dTY0XGJ8aTEyOFxifHUxMjhcYnxpc2l6ZVxifHVzaXplXGJ8ZjMyXGJ8ZjY0XGI= */
EXPORT_TEXTPARSER bool _gen_rust_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_Boolean_start_dHJ1ZVxifGZhbHNlXGJ8U29tZVxifE5vbmVcYnxPa1xifEVyclxi */
EXPORT_TEXTPARSER bool _gen_rust_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_CharLiteral_start_Jyg/OlteXFwnXXxcXFxcLikn */
EXPORT_TEXTPARSER bool _gen_rust_CharLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_Lifetime_start_J1thLXpBLVpfXVthLXpBLVowLTlfXSpcYg== */
EXPORT_TEXTPARSER bool _gen_rust_Lifetime_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_rust_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_rust_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_rust_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_Operator_start_Ojp8Oj18Onw9PT18PT18IT18PD18Pj18JiZ8XHxcfHxcKz18XC09fFwqPXxcLz18JT18Jj18XF49fFx8PXw8PD18Pj49fC0+fDw8fD4+fD0+fFs9PD4hJnxefitcLSovJVw/OjsuLF0= */
EXPORT_TEXTPARSER bool _gen_rust_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_rust_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_rust_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHR8XFx1XHtbMC05YS1mQS1GXXsxLDZ9XH18XFx4WzAtOWEtZkEtRl17Mn0= */
EXPORT_TEXTPARSER bool _gen_rust_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_Number_start_MFt4WF1bMC05YS1mQS1GX10rXGJ8MFtiQl1bMDFfXStcYnwwW29PXVswLTdfXStcYnxbMC05X10qXC4/WzAtOV9dKyg/OltlRV1bLStdP1swLTlfXSspP1xi */
EXPORT_TEXTPARSER bool _gen_rust_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_rust_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_rust_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_rust_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_rust_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_rust_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);


/* === TYPESCRIPT Grammar Matchers === */
/* _gen_typescript_LineComment_start_XC9cL1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_typescript_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_typescript_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_typescript_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_Keyword_start_YXN5bmNcYnxhd2FpdFxifGNsYXNzXGJ8Y29uc3RcYnxsZXRcYnx2YXJcYnxmdW5jdGlvblxifGltcG9ydFxifGV4cG9ydFxifGZyb21cYnxkZWZhdWx0XGJ8ZXh0ZW5kc1xifGltcGxlbWVudHNcYnxpbnRlcmZhY2VcYnx0eXBlXGJ8ZW51bVxifG5hbWVzcGFjZVxifG1vZHVsZVxifGRlY2xhcmVcYnxhYnN0cmFjdFxifHJlYWRvbmx5XGJ8c3RhdGljXGJ8cHVibGljXGJ8cHJpdmF0ZVxifHByb3RlY3RlZFxifHN1cGVyXGJ8dGhpc1xifG5ld1xifHJldHVyblxifHRyeVxifGNhdGNoXGJ8ZmluYWxseVxifHRocm93XGJ8aWZcYnxlbHNlXGJ8c3dpdGNoXGJ8Y2FzZVxifGJyZWFrXGJ8Y29udGludWVcYnxkb1xifHdoaWxlXGJ8Zm9yXGJ8aW5cYnxvZlxifHR5cGVvZlxifGluc3RhbmNlb2ZcYnxrZXlvZlxifHNhdGlzZmllc1xifGFzXGJ8aW5mZXJcYnxpc1xifG5ldmVyXGJ8dW5rbm93blxifGFueVxifHlpZWxkXGJ8ZGVidWdnZXJcYnxkZWxldGVcYnx2b2lkXGJ8d2l0aFxifG51bGxcYnx1bmRlZmluZWRcYg== */
EXPORT_TEXTPARSER bool _gen_typescript_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_typescript_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_Variable_start_W2EtekEtWl8kXVthLXpBLVowLTlfJF0q */
EXPORT_TEXTPARSER bool _gen_typescript_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_typescript_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_typescript_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_Regex_start_XC8oPzpbXlwvXFwNCl18XFwuKStcL1thLXpBLVpdKg== */
EXPORT_TEXTPARSER bool _gen_typescript_Regex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_Operator_start_Pj4+PXxcP1w/PXwmJj18XHxcfD18PT09fCE9PXxcKlwqPXw8PD18Pj49fFwrXCt8XC1cLXxcKz18XC09fFwqPXxcLz18JT18Jj18XHw9fF49fD0+fD09fCE9fDw9fD49fCYmfFx8XHx8XD9cP3xcP1wufFw/Onw8PHw+PnxcKlwqfDo6fFs9PD4hJnxefitcLSovJVw/OjsuLF0= */
EXPORT_TEXTPARSER bool _gen_typescript_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_typescript_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_typescript_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_typescript_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_typescript_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_TemplateString_start_YA== */
EXPORT_TEXTPARSER bool _gen_typescript_TemplateString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_TemplateString_end_YA== */
EXPORT_TEXTPARSER bool _gen_typescript_TemplateString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHR8XFxcYHxcXHVbMC05YS1mQS1GXXs0fXxcXHhbMC05YS1mQS1GXXsyfQ== */
EXPORT_TEXTPARSER bool _gen_typescript_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_Number_start_MFt4WF1bMC05YS1mQS1GXSt8MFtvT11bMC03XSt8MFtiQl1bMDFdK3xbMC05XSpcLj9bMC05XSsoPzpbZUVdWy0rXT9bMC05XSspPw== */
EXPORT_TEXTPARSER bool _gen_typescript_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_typescript_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_typescript_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_typescript_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_typescript_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_typescript_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* === JAVA Grammar Matchers === */
/* _gen_java_LineComment_start_XC9cL1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_java_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_java_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_java_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_Annotation_start_QFthLXpBLVpfXVthLXpBLVowLTlfXSo= */
EXPORT_TEXTPARSER bool _gen_java_Annotation_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_Keyword_start_Y2xhc3NcYnxpbnRlcmZhY2VcYnxlbnVtXGJ8ZXh0ZW5kc1xifGltcGxlbWVudHNcYnxwYWNrYWdlXGJ8aW1wb3J0XGJ8cHVibGljXGJ8cHJpdmF0ZVxifHByb3RlY3RlZFxifHN0YXRpY1xifGZpbmFsXGJ8dm9pZFxifGludFxifGRvdWJsZVxifGZsb2F0XGJ8bG9uZ1xifHNob3J0XGJ8Ynl0ZVxifGNoYXJcYnxib29sZWFuXGJ8aWZcYnxlbHNlXGJ8Zm9yXGJ8d2hpbGVcYnxkb1xifHN3aXRjaFxifGNhc2VcYnxkZWZhdWx0XGJ8YnJlYWtcYnxjb250aW51ZVxifHJldHVyblxifG5ld1xifHRoaXNcYnxzdXBlclxifHRyeVxifGNhdGNoXGJ8ZmluYWxseVxifHRocm93XGJ8dGhyb3dzXGJ8aW5zdGFuY2VvZlxifHN5bmNocm9uaXplZFxifHZvbGF0aWxlXGJ8dHJhbnNpZW50XGJ8YWJzdHJhY3RcYnxuYXRpdmVcYnxzdHJpY3RmcFxifGFzc2VydFxifG51bGxcYnx2YXJcYg== */
EXPORT_TEXTPARSER bool _gen_java_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_java_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_java_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_java_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_Operator_start_XCs9fFwtPXxcKj18XC89fCU9fCY9fFxePXxcfD18PHsyfT18PnsyLDN9PXxcK1wrfFwtXC18JiZ8XHxcfHw8PXw+PXw9PXwhPXwtPnw6OnxbPTw+ISZ8Xn4rXC0qLyVcPzo7Lixd */
EXPORT_TEXTPARSER bool _gen_java_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_java_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_java_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_java_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_java_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHR8XFx1WzAtOWEtZkEtRl17NH0= */
EXPORT_TEXTPARSER bool _gen_java_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_Number_start_KD86MFt4WF1bMC05YS1mQS1GXVswLTlhLWZBLUZfXSpbbExdKnwwW2JCXVswMV1bMDFfXSpbbExdKnxbMC05XVswLTlfXSooPzpcLlswLTldWzAtOV9dKik/KD86W2VFXVstK10/WzAtOV9dKyk/W2ZGZERsTF0/KQ== */
EXPORT_TEXTPARSER bool _gen_java_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_java_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_java_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_java_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_java_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_java_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_java_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* === CSHARP Grammar Matchers === */
/* _gen_csharp_LineComment_start_XC9cL1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_csharp_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_csharp_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_csharp_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_Keyword_start_dXNpbmdcYnxuYW1lc3BhY2VcYnxjbGFzc1xifGludGVyZmFjZVxifHN0cnVjdFxifHJlY29yZFxifGVudW1cYnxwdWJsaWNcYnxwcml2YXRlXGJ8cHJvdGVjdGVkXGJ8aW50ZXJuYWxcYnxzdGF0aWNcYnxyZWFkb25seVxifHZvbGF0aWxlXGJ8dmlydHVhbFxifG92ZXJyaWRlXGJ8YWJzdHJhY3RcYnxzZWFsZWRcYnxwYXJ0aWFsXGJ8YXN5bmNcYnxhd2FpdFxifHZhclxifGludFxifGRvdWJsZVxifGZsb2F0XGJ8bG9uZ1xifHNob3J0XGJ8Ynl0ZVxifGNoYXJcYnxib29sXGJ8c3RyaW5nXGJ8b2JqZWN0XGJ8ZGVjaW1hbFxifHZvaWRcYnxpZlxifGVsc2VcYnxmb3JcYnxmb3JlYWNoXGJ8d2hpbGVcYnxkb1xifHN3aXRjaFxifGNhc2VcYnxkZWZhdWx0XGJ8YnJlYWtcYnxjb250aW51ZVxifHJldHVyblxifG5ld1xifHRoaXNcYnxiYXNlXGJ8dHJ5XGJ8Y2F0Y2hcYnxmaW5hbGx5XGJ8dGhyb3dcYnxpblxifG91dFxifHJlZlxifGdldFxifHNldFxifHZhbHVlXGJ8YWRkXGJ8cmVtb3ZlXGJ8ZGVsZWdhdGVcYnxldmVudFxifGxvY2tcYnxpbXBsaWNpdFxifGV4cGxpY2l0XGJ8b3BlcmF0b3JcYnxwYXJhbXNcYnxudWxsXGI= */
EXPORT_TEXTPARSER bool _gen_csharp_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_csharp_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_csharp_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_csharp_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_Operator_start_XD9cPz18XD9cLnxcP1w/fD0+fFwrPXxcLT18XCo9fFwvPXwlPXwmPXxcXj18XHw9fDx7Mn09fD57Mn09fFwrXCt8XC1cLXwmJnxcfFx8fDx7Mn18PnsyfXw8PXw+PXw9PXwhPXwtPnw6OnxcLnxbPTw+ISZ8Xn4rXC0qLyVcPzo7Lixd */
EXPORT_TEXTPARSER bool _gen_csharp_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_csharp_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_csharp_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_csharp_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_csharp_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHR8XFx1WzAtOWEtZkEtRl17NH18XFx4WzAtOWEtZkEtRl17Mn0= */
EXPORT_TEXTPARSER bool _gen_csharp_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_Number_start_KD86MFt4WF1bMC05YS1mQS1GXVswLTlhLWZBLUZfXSpbdVVsTF0qfDBbYkJdWzAxXVswMV9dKlt1VWxMXSp8WzAtOV1bMC05X10qKD86XC5bMC05XVswLTlfXSopPyg/OltlRV1bLStdP1swLTlfXSspP1tmRmREbExtTV0/KQ== */
EXPORT_TEXTPARSER bool _gen_csharp_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_csharp_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_csharp_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_csharp_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_csharp_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_csharp_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_csharp_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* === PHP Grammar Matchers === */
/* _gen_php_Tag_start_PFw/cGhwfDxcPw== */
EXPORT_TEXTPARSER bool _gen_php_Tag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_Tag_end_XD8+ */
EXPORT_TEXTPARSER bool _gen_php_Tag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_LineComment_start_XC9cL1teXHJcbl0qfCNbXlxyXG5dKg== */
EXPORT_TEXTPARSER bool _gen_php_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_php_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_php_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_ArrayKeyValue_start_PT4= */
EXPORT_TEXTPARSER bool _gen_php_ArrayKeyValue_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_MemberAccess_start_LT4= */
EXPORT_TEXTPARSER bool _gen_php_MemberAccess_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_Variable_start_XCRbYS16QS1aX11bYS16QS1aMC05X10q */
EXPORT_TEXTPARSER bool _gen_php_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_php_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_php_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_Operator_start_PT09fCE9PXw9PXwhPXxcKz18XC09fFwqPXxcLz18XC5cPXwlPXxcK1wrfFwtXC18PD18Pj18JiZ8XHxcfHxbPTw+ISZ8Xn4rXC0qLyVcPzo7XQ== */
EXPORT_TEXTPARSER bool _gen_php_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_php_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_php_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_php_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_php_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_StringEscape_start_XFxcXHxcXFwkfFxcXCJ8XFxcJ3xcXG58XFxyfFxcdA== */
EXPORT_TEXTPARSER bool _gen_php_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_Number_start_WzAtOV0rXC4/WzAtOV0q */
EXPORT_TEXTPARSER bool _gen_php_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_php_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_Keyword_start_ZWNob1xifHByaW50XGJ8ZnVuY3Rpb25cYnxjbGFzc1xifHB1YmxpY1xifHByaXZhdGVcYnxwcm90ZWN0ZWRcYnxzdGF0aWNcYnxpZlxifGVsc2VcYnxlbHNlaWZcYnxmb3JlYWNoXGJ8Zm9yXGJ8d2hpbGVcYnxkb1xifHN3aXRjaFxifGNhc2VcYnxkZWZhdWx0XGJ8cmV0dXJuXGJ8cmVxdWlyZVxifHJlcXVpcmVfb25jZVxifGluY2x1ZGVcYnxpbmNsdWRlX29uY2VcYnxuZXdcYnx1c2VcYnxuYW1lc3BhY2VcYnx0cnlcYnxjYXRjaFxifHRocm93XGJ8ZmluYWxseVxifGdsb2JhbFxifGFzXGI= */
EXPORT_TEXTPARSER bool _gen_php_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_php_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_php_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_php_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_php_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_php_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* === GO Grammar Matchers === */
/* _gen_go_LineComment_start_XC9cL1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_go_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_go_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_go_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_Keyword_start_YnJlYWtcYnxjYXNlXGJ8Y2hhblxifGNvbnN0XGJ8Y29udGludWVcYnxkZWZhdWx0XGJ8ZGVmZXJcYnxlbHNlXGJ8ZmFsbHRocm91Z2hcYnxmb3JcYnxmdW5jXGJ8Z29cYnxnb3RvXGJ8aWZcYnxpbXBvcnRcYnxpbnRlcmZhY2VcYnxtYXBcYnxwYWNrYWdlXGJ8cmFuZ2VcYnxyZXR1cm5cYnxzZWxlY3RcYnxzdHJ1Y3RcYnxzd2l0Y2hcYnx0eXBlXGJ8dmFyXGI= */
EXPORT_TEXTPARSER bool _gen_go_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_Boolean_start_dHJ1ZVxifGZhbHNlXGJ8bmlsXGJ8aW90YVxi */
EXPORT_TEXTPARSER bool _gen_go_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_go_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_go_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_go_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_Operator_start_XC5cLlwufDw8PXw+Pj18JlxePXxcK1wrfFwtXC18XCs9fFwtPXxcKj18XC89fCU9fCY9fFx8PXxcXj18JiZ8XHxcfHw8PXw+PXw9PXwhPXw6PXw8LXw8PHw+PnwmXF58PXw8fD58IXxcK3wtfFwqfFwvfCV8JnxcfHxcXnw6fDt8XC58LA== */
EXPORT_TEXTPARSER bool _gen_go_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_go_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_go_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_go_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_go_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_BacktickString_start_YA== */
EXPORT_TEXTPARSER bool _gen_go_BacktickString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_BacktickString_end_YA== */
EXPORT_TEXTPARSER bool _gen_go_BacktickString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_StringEscape_start_XFxbYWJmbnJ0dlxcXCInXXxcXHhbMC05YS1mQS1GXXsyfXxcXHVbMC05YS1mQS1GXXs0fXxcXFVbMC05YS1mQS1GXXs4fXxcXFswLTddezN9 */
EXPORT_TEXTPARSER bool _gen_go_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_Number_start_MFt4WF1bMC05YS1mQS1GXSt8MFtvT11bMC03XSt8MFtiQl1bMDFdK3xbMC05XSpcLj9bMC05XSsoPzpbZUVdWy0rXT9bMC05XSspPw== */
EXPORT_TEXTPARSER bool _gen_go_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_go_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_go_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_go_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_go_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_go_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* === SQL Grammar Matchers === */
/* _gen_sql_LineComment_start_LS1bXlxyXG5dKg== */
EXPORT_TEXTPARSER bool _gen_sql_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_sql_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_sql_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_Keyword_start_c2VsZWN0XGJ8aW5zZXJ0XGJ8dXBkYXRlXGJ8ZGVsZXRlXGJ8ZnJvbVxifHdoZXJlXGJ8am9pblxifGxlZnRcYnxyaWdodFxifGlubmVyXGJ8b3V0ZXJcYnxvblxifGdyb3VwXGJ8YnlcYnxoYXZpbmdcYnxvcmRlclxifGxpbWl0XGJ8b2Zmc2V0XGJ8Y3JlYXRlXGJ8dGFibGVcYnxkcm9wXGJ8YWx0ZXJcYnxhZGRcYnxjb2x1bW5cYnxpbmRleFxifHByaW1hcnlcYnxrZXlcYnxmb3JlaWduXGJ8cmVmZXJlbmNlc1xifGludG9cYnx2YWx1ZXNcYnxzZXRcYnxhbmRcYnxvclxifG5vdFxifGluXGJ8aXNcYnxudWxsXGJ8bGlrZVxifGJldHdlZW5cYnxleGlzdHNcYnxhbnlcYnxhbGxcYnxhc1xifGRpc3RpbmN0XGJ8dW5pb25cYnxpbnRlcnNlY3RcYnxleGNlcHRcYnx3aXRoXGJ8cmVjdXJzaXZlXGJ8ZGF0YWJhc2VcYnx1c2VcYnx2aWV3XGJ8dHJpZ2dlclxifHByb2NlZHVyZVxifGZ1bmN0aW9uXGJ8cmV0dXJuc1xifGRlY2xhcmVcYnxiZWdpblxifGVuZFxifGlmXGJ8ZWxzZVxifHRoZW5cYnxjb21taXRcYnxyb2xsYmFja1xifHRyYW5zYWN0aW9uXGJ8Z3JhbnRcYnxyZXZva2VcYnxjb25zdHJhaW50XGJ8ZGVmYXVsdFxifHVuaXF1ZVxifGNoZWNrXGI= */
EXPORT_TEXTPARSER bool _gen_sql_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_DataType_start_aW50XGJ8aW50ZWdlclxifHZhcmNoYXJcYnxjaGFyXGJ8dGV4dFxifGJvb2xlYW5cYnxib29sXGJ8ZGF0ZVxifHRpbWVcYnx0aW1lc3RhbXBcYnxkYXRldGltZVxifGRlY2ltYWxcYnxudW1lcmljXGJ8ZmxvYXRcYnxkb3VibGVcYnxyZWFsXGJ8YmxvYlxifGNsb2JcYnxqc29uXGJ8dXVpZFxi */
EXPORT_TEXTPARSER bool _gen_sql_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_sql_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_BacktickIdentifier_start_YA== */
EXPORT_TEXTPARSER bool _gen_sql_BacktickIdentifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_BacktickIdentifier_end_YA== */
EXPORT_TEXTPARSER bool _gen_sql_BacktickIdentifier_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_BracketIdentifier_start_XFs= */
EXPORT_TEXTPARSER bool _gen_sql_BracketIdentifier_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_BracketIdentifier_end_XF0= */
EXPORT_TEXTPARSER bool _gen_sql_BracketIdentifier_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_sql_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_Operator_start_PD58PD18Pj18IT18Oj18Wy0rKi8lJnxefjw+IT07Lix8PV0= */
EXPORT_TEXTPARSER bool _gen_sql_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_sql_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_sql_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_sql_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_sql_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHQ= */
EXPORT_TEXTPARSER bool _gen_sql_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_Number_start_XGJbMC05XSsoPzpcLlswLTldKyk/XGI= */
EXPORT_TEXTPARSER bool _gen_sql_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_sql_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_sql_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_sql_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_sql_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_sql_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_sql_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_sql_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* === BASH Grammar Matchers === */
/* _gen_bash_LineComment_start_I1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_bash_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_Keyword_start_aWZcYnx0aGVuXGJ8ZWxzZVxifGVsaWZcYnxmaVxifGZvclxifHdoaWxlXGJ8dW50aWxcYnxkb1xifGRvbmVcYnxpblxifGNhc2VcYnxlc2FjXGJ8c2VsZWN0XGJ8ZnVuY3Rpb25cYnx0aW1lXGJ8bG9jYWxcYnxkZWNsYXJlXGJ8ZXhwb3J0XGJ8cmVhZG9ubHlcYnxyZXR1cm5cYnxleGl0XGJ8YnJlYWtcYnxjb250aW51ZVxifGFsaWFzXGJ8dW5hbGlhc1xifGJ1aWx0aW5cYnxjb21tYW5kXGJ8ZXZhbFxifGV4ZWNcYnxzaGlmdFxifHNvdXJjZVxi */
EXPORT_TEXTPARSER bool _gen_bash_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_bash_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_Variable_start_XCRbYS16QS1aX11bYS16QS1aMC05X10qfFwkXHtbYS16QS1aX11bYS16QS1aMC05X10qXH18XCRbMC05QD8qIyQtXXxbYS16QS1aX11bYS16QS1aMC05X10qKD89PSk= */
EXPORT_TEXTPARSER bool _gen_bash_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_bash_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_bash_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_Operator_start_JiZ8XHxcfHw+Pnw8PHw9PXwhPXxcKz18XC09fFstKyovJSZ8Xn48PiE9Oy4sfF0= */
EXPORT_TEXTPARSER bool _gen_bash_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_bash_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_bash_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_bash_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_bash_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHR8XFxcJA== */
EXPORT_TEXTPARSER bool _gen_bash_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_Number_start_XGJbMC05XSsoPzpcLlswLTldKyk/XGI= */
EXPORT_TEXTPARSER bool _gen_bash_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_bash_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_bash_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_bash_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_bash_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_bash_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);


/* C3 Grammar Matchers */
/* _gen_c3_LineComment_start_XC9cL1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_c3_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_c3_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_c3_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_DocComment_start_XDxcKg== */
EXPORT_TEXTPARSER bool _gen_c3_DocComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_DocComment_end_XCpcPg== */
EXPORT_TEXTPARSER bool _gen_c3_DocComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_Directive_start_XCRbYS16QS1aX11bYS16QS1aMC05X10qXGI= */
EXPORT_TEXTPARSER bool _gen_c3_Directive_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_Annotation_start_QFthLXpBLVpfXVthLXpBLVowLTlfXSpcYg== */
EXPORT_TEXTPARSER bool _gen_c3_Annotation_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_Keyword_start_YnJlYWtcYnxjYXNlXGJ8Y29udGludWVcYnxkZWZhdWx0XGJ8ZGVmZXJcYnxkb1xifGVsc2VcYnxmb3JcYnxmb3JlYWNoXGJ8Zm9yZWFjaF9yXGJ8aWZcYnxuZXh0Y2FzZVxifHJldHVyblxifHN3aXRjaFxifHdoaWxlXGJ8YXNzZXJ0XGJ8YXNtXGJ8Y2F0Y2hcYnxpbmxpbmVcYnxpbXBvcnRcYnxtb2R1bGVcYnxpbnRlcmZhY2VcYnx0cnlcYnx2YXJcYnxjb25zdFxifGV4dGVyblxifHN0YXRpY1xifHRsb2NhbFxifGZuXGJ8dHlwZWRlZlxifHN0cnVjdFxifHVuaW9uXGJ8ZW51bVxifGZhdWx0ZGVmXGJ8YXR0cmRlZlxifGNvbnN0ZGVmXGJ8dm9pZFxifGJvb2xcYnxjaGFyXGJ8ZG91YmxlXGJ8ZmxvYXRcYnxmbG9hdDE2XGJ8YmZsb2F0XGJ8aW50MTI4XGJ8aWNoYXJcYnxpbnRcYnxpcHRyXGJ8aXN6XGJ8c3pcYnxsb25nXGJ8c2hvcnRcYnx1aW50MTI4XGJ8dWludFxifHVsb25nXGJ8dXB0clxifHVzaG9ydFxifHVzelxifGZsb2F0MTI4XGJ8YW55XGJ8ZmF1bHRcYnx0eXBlaWRcYnx1bnR5cGVkbGlzdFxi */
EXPORT_TEXTPARSER bool _gen_c3_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_Boolean_start_dHJ1ZVxifGZhbHNlXGJ8bnVsbFxi */
EXPORT_TEXTPARSER bool _gen_c3_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_c3_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_c3_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_c3_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_Operator_start_Ojp8Oj18Onw9PT18PT18IT18PD18Pj18JiZ8XHxcfHxcKz18XC09fFwqPXxcLz18JT18Jj18XF49fFx8PXw8PD18Pj49fC0+fDw8fD4+fFwuXC58Wz08PiEmfF5+K1wtKi8lXD86Oy4sXQ== */
EXPORT_TEXTPARSER bool _gen_c3_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_c3_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_c3_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_c3_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_c3_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHR8XFx1WzAtOWEtZkEtRl17NH18XFx4WzAtOWEtZkEtRl17Mn0= */
EXPORT_TEXTPARSER bool _gen_c3_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_Number_start_MFt4WF1bMC05YS1mQS1GX10rXGJ8MFtiQl1bMDFfXStcYnwwW29PXVswLTdfXStcYnxbMC05X10qXC4/WzAtOV9dKyg/OltlRV1bLStdP1swLTlfXSspP1xi */
EXPORT_TEXTPARSER bool _gen_c3_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_c3_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_c3_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_c3_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_c3_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_c3_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* ZIG Grammar Matchers */
/* _gen_zig_LineComment_start_XC9cL1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_zig_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_MultiLineString_start_XFxcXFteXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_zig_MultiLineString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_Directive_start_QFthLXpBLVpfXVthLXpBLVowLTlfXSpcYg== */
EXPORT_TEXTPARSER bool _gen_zig_Directive_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_Keyword_start_Zm5cYnxjb25zdFxifHZhclxifHB1YlxifHVzaW5nbmFtZXNwYWNlXGJ8YWxpZ25cYnxhbGxvd3plcm9cYnxhbmRcYnxhbnlmcmFtZVxifGFueXR5cGVcYnxhc21cYnxhc3luY1xifGF3YWl0XGJ8YnJlYWtcYnxjYW5jZWxcYnxjYXRjaFxifGNvbXB0aW1lXGJ8ZGVmZXJcYnxlcnJkZWZlclxifGVudW1cYnxleHBvcnRcYnxleHRlcm5cYnxmb3JcYnxpZlxifGlubGluZVxifG5vYWxpYXNcYnxub2lubGluZVxifG5vc3VzcGVuZFxifG9yXGJ8b3BhcXVlXGJ8cGFja2VkXGJ8cmVzdW1lXGJ8cmV0dXJuXGJ8bGlua3NlY3Rpb25cYnxzdHJ1Y3RcYnxzdXNwZW5kXGJ8c3dpdGNoXGJ8dGVzdFxifHRocmVhZGxvY2FsXGJ8dHJ5XGJ8dW5pb25cYnx1bnJlYWNoYWJsZVxifHZvbGF0aWxlXGJ8d2hpbGVcYnxvcmVsc2VcYnx2b2lkXGJ8bm9yZXR1cm5cYnx0eXBlXGJ8YW55ZXJyb3JcYnxib29sXGJ8ZjE2XGJ8ZjMyXGJ8ZjY0XGJ8ZjEyOFxifGNfc2hvcnRcYnxjX3VzaG9ydFxifGNfaW50XGJ8Y191aW50XGJ8Y19sb25nXGJ8Y191bG9uZ1xifGNfbG9uZ2xvbmdcYnxjX3Vsb25nbG9uZ1xifGNfbG9uZ2RvdWJsZVxifGNfdm9pZFxifGlzaXplXGJ8dXNpemVcYnxpOFxifHU4XGJ8aTE2XGJ8dTE2XGJ8aTMyXGJ8dTMyXGJ8aTY0XGJ8dTY0XGJ8aTEyOFxifHUxMjhcYg== */
EXPORT_TEXTPARSER bool _gen_zig_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_Boolean_start_dHJ1ZVxifGZhbHNlXGJ8bnVsbFxifHVuZGVmaW5lZFxi */
EXPORT_TEXTPARSER bool _gen_zig_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_zig_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_zig_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_zig_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_Operator_start_Ojp8Oj18Onw9PT18PT18IT18PD18Pj18JiZ8XHxcfHxcKz18XC09fFwqPXxcLz18JT18Jj18XF49fFx8PXw8PD18Pj49fC0+fDw8fD4+fFs9PD4hJnxefitcLSovJVw/OjsuLF0= */
EXPORT_TEXTPARSER bool _gen_zig_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_zig_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_zig_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_zig_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_zig_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHR8XFx1WzAtOWEtZkEtRl17NH18XFx4WzAtOWEtZkEtRl17Mn0= */
EXPORT_TEXTPARSER bool _gen_zig_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_Number_start_MFt4WF1bMC05YS1mQS1GX10rXGJ8MFtiQl1bMDFfXStcYnwwW29PXVswLTdfXStcYnxbMC05X10qXC4/WzAtOV9dKyg/OltlRV1bLStdP1swLTlfXSspP1xi */
EXPORT_TEXTPARSER bool _gen_zig_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_zig_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_zig_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_zig_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_zig_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_zig_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* SWIFT Grammar Matchers */
/* _gen_swift_LineComment_start_XC9cL1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_swift_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_swift_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_swift_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_Keyword_start_YWN0b3JcYnxhc3NvY2lhdGVkdHlwZVxifGFzeW5jXGJ8YXdhaXRcYnxjbGFzc1xifGNvbnZlbmllbmNlXGJ8ZGVpbml0XGJ8ZGlzdHJpYnV0ZWRcYnxkeW5hbWljXGJ8ZW51bVxifGV4dGVuc2lvblxifGZpbGVwcml2YXRlXGJ8ZmluYWxcYnxmdW5jXGJ8Z2V0XGJ8Z3VhcmRcYnxpbXBvcnRcYnxpbmRpcmVjdFxifGluZml4XGJ8aW5pdFxifGlub3V0XGJ8aW50ZXJuYWxcYnxpc29sYXRlZFxifGxhenlcYnxsZXRcYnxtYWNyb1xifG11dGF0aW5nXGJ8bm9uaXNvbGF0ZWRcYnxub25tdXRhdGluZ1xifG9wZW5cYnxvcGVyYXRvclxifG9wdGlvbmFsXGJ8b3ZlcnJpZGVcYnxwYWNrYWdlXGJ8cG9zdGZpeFxifHByZWNlZGVuY2Vncm91cFxifHByZWZpeFxifHByaXZhdGVcYnxwcm90b2NvbFxifHB1YmxpY1xifHJlcXVpcmVkXGJ8cmV0aHJvd3NcYnxzZXRcYnxzb21lXGJ8c3RhdGljXGJ8c3RydWN0XGJ8c3Vic2NyaXB0XGJ8dHlwZWFsaWFzXGJ8dW5vd25lZFxifHZhclxifHdlYWtcYnx3aWxsU2V0XGJ8ZGlkU2V0XGJ8d2hlcmVcYnxhbnlcYnxhc1xifGJyZWFrXGJ8Y2FzZVxifGNhdGNoXGJ8Y29udGludWVcYnxkZWZhdWx0XGJ8ZGVmZXJcYnxkb1xifGVsc2VcYnxmYWxsdGhyb3VnaFxifGZvclxifGlmXGJ8aW5cYnxpc1xifHJlcGVhdFxifHJldHVyblxifHN3aXRjaFxifHRocm93XGJ8dGhyb3dzXGJ8dHJ5XGJ8d2hpbGVcYnwjYXZhaWxhYmxlXGJ8I3NlbGVjdG9yXGJ8I2tleVBhdGhcYnwjZmlsZVxifCNsaW5lXGJ8I2NvbHVtblxifCNmdW5jdGlvblxifCNkc29oYW5kbGVcYnwjd2FybmluZ1xifCNlcnJvclxifCNpZlxifCNlbHNlXGJ8I2Vsc2VpZlxifCNlbmRpZlxi */
EXPORT_TEXTPARSER bool _gen_swift_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_Boolean_start_dHJ1ZVxifGZhbHNlXGJ8bmlsXGI= */
EXPORT_TEXTPARSER bool _gen_swift_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_swift_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_swift_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_swift_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_Operator_start_XD9cP3w9PT18IT09fFwuXC48fFwuXC5cLnwtPnxcKz18XC09fFwqPXxcLz18JT18Jj18XF49fFx8PXw8PD18Pj49fCYmfFx8XHx8PD18Pj18PT18IT18PDx8Pj58Wz08PiEmfF5+K1wtKi8lXD86Oy4sXQ== */
EXPORT_TEXTPARSER bool _gen_swift_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_swift_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_swift_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_MultiLineString_start_IiIi */
EXPORT_TEXTPARSER bool _gen_swift_MultiLineString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_MultiLineString_end_IiIi */
EXPORT_TEXTPARSER bool _gen_swift_MultiLineString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_StringInterpolation_start_XFxcKA== */
EXPORT_TEXTPARSER bool _gen_swift_StringInterpolation_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_StringInterpolation_end_XCk= */
EXPORT_TEXTPARSER bool _gen_swift_StringInterpolation_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_StringEscape_start_XFxbXFxcIiducnQwXXxcXHVce1swLTlhLWZBLUZdezEsOH1cfQ== */
EXPORT_TEXTPARSER bool _gen_swift_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_Number_start_MFt4WF1bMC05YS1mQS1GXSsoPzpcLlswLTlhLWZBLUZdKyk/KD86W3BQXVstK10/WzAtOV0rKT9cYnwwW29PXVswLTddK1xifDBbYkJdWzAxXStcYnxbMC05XSsoPzpcLlswLTldKyk/KD86W2VFXVstK10/WzAtOV0rKT9cYg== */
EXPORT_TEXTPARSER bool _gen_swift_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_swift_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_swift_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_swift_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_swift_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_swift_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* PASCAL Grammar Matchers */
/* _gen_pascal_LineComment_start_XC9cL1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_pascal_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_CompilerDirective_start_XHtcJA== */
EXPORT_TEXTPARSER bool _gen_pascal_CompilerDirective_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_CompilerDirective_end_XH0= */
EXPORT_TEXTPARSER bool _gen_pascal_CompilerDirective_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_BlockCommentCurly_start_XHs= */
EXPORT_TEXTPARSER bool _gen_pascal_BlockCommentCurly_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_BlockCommentCurly_end_XH0= */
EXPORT_TEXTPARSER bool _gen_pascal_BlockCommentCurly_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_BlockCommentParen_start_XChcKg== */
EXPORT_TEXTPARSER bool _gen_pascal_BlockCommentParen_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_BlockCommentParen_end_XCpcKQ== */
EXPORT_TEXTPARSER bool _gen_pascal_BlockCommentParen_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_Keyword_start_YW5kXGJ8b3JcYnxub3RcYnx4b3JcYnxzaGxcYnxzaHJcYnxkaXZcYnxtb2RcYnxhc1xifGlzXGJ8cHJvZ3JhbVxifHVuaXRcYnxpbnRlcmZhY2VcYnxpbXBsZW1lbnRhdGlvblxifHVzZXNcYnx2YXJcYnx0eXBlXGJ8Y29uc3RcYnxyZXNvdXJjZXN0cmluZ1xifHRocmVhZHZhclxifGJlZ2luXGJ8ZW5kXGJ8cHJvY2VkdXJlXGJ8ZnVuY3Rpb25cYnxjb25zdHJ1Y3RvclxifGRlc3RydWN0b3JcYnxwcm9wZXJ0eVxifGNsYXNzXGJ8cmVjb3JkXGJ8b2JqZWN0XGJ8aGVscGVyXGJ8c3RyaWN0XGJ8cHJpdmF0ZVxifHByb3RlY3RlZFxifHB1YmxpY1xifHB1Ymxpc2hlZFxifGluaXRpYWxpemF0aW9uXGJ8ZmluYWxpemF0aW9uXGJ8aWZcYnx0aGVuXGJ8ZWxzZVxifGNhc2VcYnxvZlxifGZvclxifHRvXGJ8ZG93bnRvXGJ8ZG9cYnx3aGlsZVxifHJlcGVhdFxifHVudGlsXGJ8d2l0aFxifHRyeVxifGV4Y2VwdFxifGZpbmFsbHlcYnxyYWlzZVxifGF0XGJ8b25cYnxpbmhlcml0ZWRcYnxpbmxpbmVcYnxvdmVybG9hZFxifG92ZXJyaWRlXGJ8dmlydHVhbFxifGFic3RyYWN0XGJ8cmVpbnRyb2R1Y2VcYnxuaWxcYnxvdXRcYnxsYWJlbFxifGdvdG9cYnxleHBvcnRzXGJ8bGlicmFyeVxifHBhY2thZ2VcYnxyZXF1aXJlc1xifGNvbnRhaW5zXGJ8YWJzb2x1dGVcYnxhc3NlbWJsZXJcYnxjZGVjbFxifHBhc2NhbFxifHJlZ2lzdGVyXGJ8c2FmZWNhbGxcYnxzdGRjYWxsXGJ8cmVmZXJlbmNlXGJ8b3BlcmF0b3JcYg== */
EXPORT_TEXTPARSER bool _gen_pascal_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_DataType_start_aW50ZWdlclxifGNhcmRpbmFsXGJ8c2hvcnRpbnRcYnxzbWFsbGludFxifGxvbmdpbnRcYnxpbnQ2NFxifGJ5dGVcYnx3b3JkXGJ8Zml4ZWRpbnRcYnxmaXhlZHVpbnRcYnxyZWFsXGJ8c2luZ2xlXGJ8ZG91YmxlXGJ8ZXh0ZW5kZWRcYnxjb21wXGJ8Y3VycmVuY3lcYnxjaGFyXGJ8YW5zaWNoYXJcYnx3aWRlY2hhclxifGJvb2xlYW5cYnxieXRlYm9vbFxifHdvcmRib29sXGJ8bG9uZ2Jvb2xcYnxzdHJpbmdcYnxhbnNpc3RyaW5nXGJ8d2lkZXN0cmluZ1xifHVuaWNvZGVzdHJpbmdcYnxzaG9ydHN0cmluZ1xifHBvaW50ZXJcYnx2YXJpYW50XGJ8dG9iamVjdFxifHRjbGFzc1xi */
EXPORT_TEXTPARSER bool _gen_pascal_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_pascal_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_CharLiteral_start_I1swLTldK3wjXCRbMC05YS1mQS1GXSs= */
EXPORT_TEXTPARSER bool _gen_pascal_CharLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_pascal_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_pascal_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_EscapedQuote_start_Jyc= */
EXPORT_TEXTPARSER bool _gen_pascal_EscapedQuote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_Number_start_XCRbMC05YS1mQS1GXStcYnxcYlswLTldKyg/OlwuWzAtOV0rKT8oPzpbZUVdWy0rXT9bMC05XSspP1xi */
EXPORT_TEXTPARSER bool _gen_pascal_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_pascal_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_Operator_start_Oj18PT18PD18Pj18PD58XCs9fFwtPXxcKj18XC89fFtAXF4uXCw6O1wrXC1cKlwvPTw+XQ== */
EXPORT_TEXTPARSER bool _gen_pascal_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_pascal_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_pascal_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_pascal_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_pascal_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_pascal_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_pascal_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_pascal_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* PERL Grammar Matchers */
/* _gen_perl_LineComment_start_I1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_perl_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_PODBlock_start_Xj1cdys= */
EXPORT_TEXTPARSER bool _gen_perl_PODBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_PODBlock_end_Xj1jdXQ= */
EXPORT_TEXTPARSER bool _gen_perl_PODBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_Keyword_start_cGFja2FnZVxifHVzZVxifHJlcXVpcmVcYnxzdWJcYnxteVxifG91clxifGxvY2FsXGJ8c3RhdGVcYnxpZlxifHVubGVzc1xifGVsc2VcYnxlbHNpZlxifGdpdmVuXGJ8d2hlblxifGRlZmF1bHRcYnxmb3JcYnxmb3JlYWNoXGJ8d2hpbGVcYnx1bnRpbFxifGRvXGJ8Y29udGludWVcYnxsYXN0XGJ8bmV4dFxifHJlZG9cYnxnb3RvXGJ8cmV0dXJuXGJ8ZGllXGJ8d2FyblxifGV4aXRcYnxldmFsXGJ8dHJ5XGJ8Y2F0Y2hcYnxmaW5hbGx5XGJ8dGhyb3dcYnxibGVzc1xifHJlZlxifHRpZVxifHVudGllXGJ8ZGVmaW5lZFxifGV4aXN0c1xifGRlbGV0ZVxifHNoaWZ0XGJ8dW5zaGlmdFxifHB1c2hcYnxwb3BcYnxzcGxpY2VcYnxzcGxpdFxifGpvaW5cYnxrZXlzXGJ8dmFsdWVzXGJ8ZWFjaFxifG1hcFxifGdyZXBcYnxzb3J0XGJ8cHJpbnRcYnxzYXlcYnxwcmludGZcYnxzcHJpbnRmXGJ8b3BlblxifGNsb3NlXGJ8cmVhZFxifHdyaXRlXGJ8c3lzb3BlblxifHN5c3JlYWRcYnxzeXN3cml0ZVxifHNlZWtcYnx0ZWxsXGJ8dHJ1bmNhdGVcYnxmbG9ja1xifGNoZGlyXGJ8bWtkaXJcYnxybWRpclxifG9wZW5kaXJcYnxjbG9zZWRpclxifHJlYWRkaXJcYnxnbG9iXGJ8dW5saW5rXGJ8cmVuYW1lXGJ8Y2htb2RcYnxjaG93blxifHVtYXNrXGJ8bGlua1xifHN5bWxpbmtcYnxyZWFkbGlua1xifHN0YXRcYnxsc3RhdFxifGZjbnRsXGJ8aW9jdGxcYnxzZWxlY3RcYnxzb2NrZXRcYnxjb25uZWN0XGJ8YmluZFxifGxpc3RlblxifGFjY2VwdFxifHNlbmRcYnxyZWN2XGJ8c2h1dGRvd25cYnxzZXRzb2Nrb3B0XGJ8Z2V0c29ja29wdFxifGZvcmtcYnxleGVjXGJ8c3lzdGVtXGJ8cXhcYnxwaXBlXGJ8d2FpdFxifHdhaXRwaWRcYnx0aW1lc1xifGFsYXJtXGJ8c2xlZXBcYnxnbXRpbWVcYnxsb2NhbHRpbWVcYnx0aW1lXGJ8Y2FsbGVyXGJ8d2FudGFycmF5XGJ8cHJvdG90eXBlXGJ8X19GSUxFX19cYnxfX0xJTkVfX1xifF9fUEFDS0FHRV9fXGI= */
EXPORT_TEXTPARSER bool _gen_perl_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_Boolean_start_XGIoPzp1bmRlZilcYg== */
EXPORT_TEXTPARSER bool _gen_perl_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_perl_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_perl_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_perl_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_perl_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_StringEscape_start_XFxbXFxcIiducnRmZWFiXXxcXHhbMC05YS1mQS1GXXsxLDJ9fFxcWzAtN117MSwzfXxcXGMu */
EXPORT_TEXTPARSER bool _gen_perl_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_Number_start_MFt4WF1bMC05YS1mQS1GXSt8MFtvT11bMC03XSt8MFtiQl1bMDFdK3xbMC05XSpcLj9bMC05XSsoPzpbZUVdWy0rXT9bMC05XSspPw== */
EXPORT_TEXTPARSER bool _gen_perl_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_Variable_start_WyRAJSpdW2EtekEtWl9dW2EtekEtWjAtOV9dKnxbJEAlKl1cXlthLXpBLVpdfFskQCUqXVwkfFskQCUqXVxkK3xbJEAlKl1bOl17Mn1bYS16QS1aX11bYS16QS1aMC05X10q */
EXPORT_TEXTPARSER bool _gen_perl_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_Operator_start_XC5cLlwufDw9PnxcfFx8fD18XHxcfHw9PnwtPnw9PXwhPXw8PXw+PXw9fnwhfnwmJnwvL3xcLlwufFwrXCt8XC1cLXxcKlwqfFwrPXxcLT18XCo9fFwvPXwlPXwmPXxcXj18PDx8Pj58PXw8fD58IXxcfHxcXnwmfFwqfC98JXxcK3wtfFwufH58LHw7 */
EXPORT_TEXTPARSER bool _gen_perl_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_perl_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_perl_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_perl_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_perl_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_perl_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_perl_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_perl_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* FORTRAN Grammar Matchers */
/* _gen_fortran_LineComment_start_IVteXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_fortran_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_Keyword_start_YWJzdHJhY3RcYnxhbGxvY2F0YWJsZVxifGFsbG9jYXRlXGJ8YXNzaWdubWVudFxifGFzc29jaWF0ZWRcYnxiaW5kXGJ8YmxvY2tcYnxjYWxsXGJ8Y2FzZVxifGNsYXNzXGJ8Y2xvc2VcYnxjb21tb25cYnxjb250YWluc1xifGNvbnRpbnVlXGJ8Y3JpdGljYWxcYnxjeWNsZVxifGRhdGFcYnxkZWFsbG9jYXRlXGJ8ZGVmYXVsdFxifGRlZmVycmVkXGJ8ZGltZW5zaW9uXGJ8ZG9cYnxlbGVtZW50YWxcYnxlbHNlXGJ8ZWxzZWlmXGJ8ZWxzZXdoZXJlXGJ8ZW5kXGJ8ZW5kZG9cYnxlbmRpZlxifGVudW1cYnxlcXVpdmFsZW5jZVxifGVycm9yXGJ8ZXhpdFxifGV4dGVuZHNcYnxleHRlcm5hbFxifGZpbmFsXGJ8Zm9yYWxsXGJ8Zm9ybWF0XGJ8ZnVuY3Rpb25cYnxnZW5lcmljXGJ8Z29cYnxpZlxifGltcGxpY2l0XGJ8aW1wb3J0XGJ8aW5cYnxpbmNsdWRlXGJ8aW5vdXRcYnxpbnRlbnRcYnxpbnRlcmZhY2VcYnxpbnRyaW5zaWNcYnxtb2R1bGVcYnxuZXdcYnxub25lXGJ8bm9wYXNzXGJ8bnVsbFxifG51bGxpZnlcYnxvbmx5XGJ8b3BlblxifG9wZXJhdG9yXGJ8b3B0aW9uYWxcYnxvdXRcYnxwYXJhbWV0ZXJcYnxwYXNzXGJ8cG9pbnRlclxifHByaW50XGJ8cHJpdmF0ZVxifHByb2NlZHVyZVxifHByb2dyYW1cYnxwcm90ZWN0ZWRcYnxwdWJsaWNcYnxwdXJlXGJ8cmVhZFxifHJlY3Vyc2l2ZVxifHJldHVyblxifHNhdmVcYnxzZWxlY3RcYnxzdG9wXGJ8c3Vicm91dGluZVxifHRhcmdldFxifHRoZW5cYnx0b1xifHR5cGVcYnx1c2VcYnx3YWl0XGJ8d2hlcmVcYnx3aGlsZVxifHdyaXRlXGI= */
EXPORT_TEXTPARSER bool _gen_fortran_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_Boolean_start_XC50cnVlXC58XC5mYWxzZVwu */
EXPORT_TEXTPARSER bool _gen_fortran_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_fortran_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_fortran_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_fortran_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_fortran_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_EscapedApostrophe_start_Jyc= */
EXPORT_TEXTPARSER bool _gen_fortran_EscapedApostrophe_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_EscapedDoubleQuote_start_IiI= */
EXPORT_TEXTPARSER bool _gen_fortran_EscapedDoubleQuote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_Number_start_XGJbMC05XSsoPzpcLlswLTldKyk/KD86W2REZUVdWy0rXT9bMC05XSspP1xifFwuWzAtOV0rKD86W2REZUVdWy0rXT9bMC05XSspP1xi */
EXPORT_TEXTPARSER bool _gen_fortran_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_fortran_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_Operator_start_Ojp8PT18XC89fD49fDw9fD0+fFwqXCp8XCt8LXxcKnxcL3w9fDx8PnwsfDp8JXw7 */
EXPORT_TEXTPARSER bool _gen_fortran_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_fortran_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_fortran_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_fortran_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_fortran_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_fortran_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_fortran_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_fortran_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* ADA Grammar Matchers */
/* _gen_ada_LineComment_start_LS1bXlxyXG5dKg== */
EXPORT_TEXTPARSER bool _gen_ada_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_Keyword_start_YWJvcnRcYnxhYnNcYnxhYnN0cmFjdFxifGFjY2VwdFxifGFjY2Vzc1xifGFsaWFzZWRcYnxhbGxcYnxhbmRcYnxhcnJheVxifGF0XGJ8YmVnaW5cYnxib2R5XGJ8Y2FzZVxifGNvbnN0YW50XGJ8ZGVjbGFyZVxifGRlbGF5XGJ8ZGVsdGFcYnxkaWdpdHNcYnxkb1xifGVsc2VcYnxlbHNpZlxifGVuZFxifGVudHJ5XGJ8ZXhjZXB0aW9uXGJ8ZXhpdFxifGZvclxifGZ1bmN0aW9uXGJ8Z2VuZXJpY1xifGdvdG9cYnxpZlxifGluXGJ8aW50ZXJmYWNlXGJ8aXNcYnxsaW1pdGVkXGJ8bG9vcFxifG1vZFxifG5ld1xifG5vdFxifG51bGxcYnxvZlxifG9yXGJ8b3RoZXJzXGJ8b3V0XGJ8b3ZlcnJpZGluZ1xifHBhY2thZ2VcYnxwcmFnbWFcYnxwcml2YXRlXGJ8cHJvY2VkdXJlXGJ8cHJvdGVjdGVkXGJ8cmFpc2VcYnxyYW5nZVxifHJlY29yZFxifHJlbVxifHJlbmFtZXNcYnxyZXF1ZXVlXGJ8cmV0dXJuXGJ8cmV2ZXJzZVxifHNlbGVjdFxifHNlcGFyYXRlXGJ8c29tZVxifHN1YnR5cGVcYnxzeW5jaHJvbml6ZWRcYnx0YWdnZWRcYnx0YXNrXGJ8dGVybWluYXRlXGJ8dGhlblxifHR5cGVcYnx1bnRpbFxifHVzZVxifHdoZW5cYnx3aGlsZVxifHdpdGhcYnx4b3JcYg== */
EXPORT_TEXTPARSER bool _gen_ada_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_DataType_start_aW50ZWdlclxifGZsb2F0XGJ8Y2hhcmFjdGVyXGJ8Ym9vbGVhblxifHN0cmluZ1xifG5hdHVyYWxcYnxwb3NpdGl2ZVxifGxvbmdfaW50ZWdlclxifGxvbmdfZmxvYXRcYnxzaG9ydF9pbnRlZ2VyXGJ8c2hvcnRfZmxvYXRcYnx3aWRlX2NoYXJhY3RlclxifHdpZGVfc3RyaW5nXGJ8ZHVyYXRpb25cYnxhZGRyZXNzXGJ8Y291bnRcYg== */
EXPORT_TEXTPARSER bool _gen_ada_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_ada_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_CharLiteral_start_Jy4n */
EXPORT_TEXTPARSER bool _gen_ada_CharLiteral_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_SingleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_ada_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_SingleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_ada_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_EscapedQuote_start_IiI= */
EXPORT_TEXTPARSER bool _gen_ada_EscapedQuote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_Number_start_WzAtOV0rKD86I1swLTlhLWZBLUZdKyg/OlwuWzAtOWEtZkEtRl0rKT8jKT8oPzpcLlswLTldKyk/KD86W2VFXVstK10/WzAtOV0rKT9cYg== */
EXPORT_TEXTPARSER bool _gen_ada_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_Attribute_start_J1thLXpBLVpfXVthLXpBLVowLTlfXSo= */
EXPORT_TEXTPARSER bool _gen_ada_Attribute_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_ada_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_Operator_start_Oj18PT58XC5cLnw8Pnw+PXw8PXxcKlwqfC89fFs9PD4rXC0qLyYuLDs6XQ== */
EXPORT_TEXTPARSER bool _gen_ada_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_ada_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_ada_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_ada_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_ada_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_ada_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_ada_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_ada_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);


/* ASM Grammar Matchers */
/* _gen_asm_LineComment_start_O1teXHJcbl0qfFwvXC9bXlxyXG5dKg== */
EXPORT_TEXTPARSER bool _gen_asm_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_asm_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_asm_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_Keyword_start_bW92XGJ8YWRkXGJ8c3ViXGJ8bXVsXGJ8ZGl2XGJ8aW11bFxifGlkaXZcYnxpbmNcYnxkZWNcYnxuZWdcYnxjbXBcYnx0ZXN0XGJ8YW5kXGJ8b3JcYnx4b3JcYnxub3RcYnxzaGxcYnxzaHJcYnxzYWxcYnxzYXJcYnxyb2xcYnxyb3JcYnxyY2xcYnxyY3JcYnxwdXNoXGJ8cG9wXGJ8cHVzaGZcYnxwb3BmXGJ8cHVzaGFcYnxwb3BhXGJ8Y2FsbFxifHJldFxifHJldGZcYnxpcmV0ZFxifGludFxifGludG9cYnxzeXNjYWxsXGJ8c3lzZW50ZXJcYnxqbXBcYnxqZVxifGpuZVxifGp6XGJ8am56XGJ8amdcYnxqbFxifGpnZVxifGpsZVxifGphXGJ8amJcYnxqYWVcYnxqYmVcYnxqb1xifGpub1xifGpzXGJ8am5zXGJ8anBcYnxqbnBcYnxqY3h6XGJ8amVjeHpcYnxsb29wXGJ8bG9vcGVcYnxsb29wbmVcYnxub3BcYnxobHRcYnxjbGlcYnxzdGlcYnxjbGRcYnxzdGRcYnxjbWNcYnxjbGNcYnxzdGNcYnxtb3ZzYlxifG1vdnN3XGJ8bW92c2RcYnxjbXBzYlxifGNtcHN3XGJ8Y21wc2RcYnxzY2FzYlxifHNjYXN3XGJ8c2Nhc2RcYnxsb2RzYlxifGxvZHN3XGJ8bG9kc2RcYnxzdG9zYlxifHN0b3N3XGJ8c3Rvc2RcYnxsZWFcYnxtb3Z6eFxifG1vdnN4XGJ8Y3dkZVxifGNkcVxifGNxb1xifHhjaGdcYnxic3dhcFxifGJ0XGJ8YnRzXGJ8YnRyXGJ8YnRjXGJ8YnNmXGJ8YnNyXGJ8c2V0ZVxifHNldG5lXGJ8c2V0Z1xifHNldGxcYnxzZXRnZVxifHNldGxlXGJ8c2V0YVxifHNldGJcYnxzZXRhZVxifHNldGJlXGJ8Y3B1aWRcYnxyZHRzY1xifHJkbXNyXGJ8d3Jtc3JcYnxsZ2R0XGJ8bGlkdFxifHNnZHRcYnxzaWR0XGI= */
EXPORT_TEXTPARSER bool _gen_asm_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_Directive_start_XC5bYS16QS1aX11bYS16QS1aMC05X10qfHNlY3Rpb25cYnxzZWdtZW50XGJ8cHJvY1xifGVuZHBcYnxhc3N1bWVcYnxtb2RlbFxifHN0YWNrXGJ8ZGF0YVxifGNvZGVcYnxwdWJsaWNcYnxleHRlcm5cYnxnbG9iYWxcYnxhbGlnblxifG9yZ1xifGVxdVxifD1cYnxkYlxifGR3XGJ8ZGRcYnxkcVxifGR0XGJ8cmVzYlxifHJlc3dcYnxyZXNkXGJ8cmVzcVxifGluY2JpblxifG1hY3JvXGJ8ZW5kbVxifGxvY2FsXGJ8cmVwdFxifGlycFxifGlycGNcYnxleGl0bVxi */
EXPORT_TEXTPARSER bool _gen_asm_Directive_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_Register_start_W3JlXT9bYWJjZF14XGJ8W3JlXT9bYnNdcFxifFtyZV0/W2JzXWlcYnxbcmVdP1tkaV1wXGJ8clswLTldK1xifGU/W2FiY2RdeFxifGU/W2JzXXBcYnxlP1tic11pXGJ8ZT9bZGldcFxifHJbMC05XStbZHddP1xifHN0XChbMC05XStcKVxifG1tWzAtN11cYnx4bW1bMC05XStcYnx5bW1bMC05XStcYnx6bW1bMC05XStcYnxjclswLThdXGJ8ZHJbMC03XVxifGNzXGJ8ZHNcYnxlc1xifGZzXGJ8Z3NcYnxzc1xifGF4XGJ8YnhcYnxjeFxifGR4XGJ8c2lcYnxkaVxifGJwXGJ8c3BcYnxpcFxifGFoXGJ8YWxcYnxiaFxifGJsXGJ8Y2hcYnxjbFxifGRoXGJ8ZGxcYg== */
EXPORT_TEXTPARSER bool _gen_asm_Register_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_Number_start_MFt4WF1bMC05YS1mQS1GXStcYnwwW29PcVFdWzAtN10rXGJ8MFtiQnlZXVswMV0rXGJ8WzAtOV0rKD86XC5bMC05XSspP1xi */
EXPORT_TEXTPARSER bool _gen_asm_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_asm_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_asm_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_asm_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_Operator_start_XCt8LXxcKnxcL3wsfDp8Ow== */
EXPORT_TEXTPARSER bool _gen_asm_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_asm_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_asm_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_asm_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_asm_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_asm_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_asm_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_asm_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* MATLAB Grammar Matchers */
/* _gen_matlab_LineComment_start_JVteXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_matlab_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_Keyword_start_YnJlYWtcYnxjYXNlXGJ8Y2F0Y2hcYnxjbGFzc2RlZlxifGNvbnRpbnVlXGJ8ZWxzZVxifGVsc2VpZlxifGVuZFxifGVudW1lcmF0aW9uXGJ8ZXZlbnRzXGJ8Zm9yXGJ8ZnVuY3Rpb25cYnxnbG9iYWxcYnxpZlxifG1ldGhvZHNcYnxvdGhlcndpc2VcYnxwYXJmb3JcYnxwZXJzaXN0ZW50XGJ8cHJvcGVydGllc1xifHJldHVyblxifHNwbWRcYnxzd2l0Y2hcYnx0cnlcYnx3aGlsZVxi */
EXPORT_TEXTPARSER bool _gen_matlab_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_matlab_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_matlab_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_matlab_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_matlab_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_matlab_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_StringEscape_start_Jyd8IiI= */
EXPORT_TEXTPARSER bool _gen_matlab_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_Number_start_WzAtOV0qXC4/WzAtOV0rKD86W2VFXVstK10/WzAtOV0rKT9baUlqSl0/fFwuWzAtOV0rKD86W2VFXVstK10/WzAtOV0rKT9baUlqSl0/fDBbeFhdWzAtOWEtZkEtRl0r */
EXPORT_TEXTPARSER bool _gen_matlab_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_Variable_start_W2EtekEtWl1bYS16QS1aMC05X10q */
EXPORT_TEXTPARSER bool _gen_matlab_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_Operator_start_XC5cKnxcLlwvfFwuXFx8XC5cXnw9PXx+PXxcLid8JiZ8XHxcfHw8PHw+PnxcKz18XC09fFwqPXxcLz18Wz1+Jnw8Pj0rXC0qL1xcXicuOjssQF0= */
EXPORT_TEXTPARSER bool _gen_matlab_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_matlab_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_matlab_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_matlab_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_matlab_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_matlab_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_matlab_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_matlab_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* R Grammar Matchers */
/* _gen_r_LineComment_start_I1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_r_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_Keyword_start_KD86YnJlYWt8ZWxzZXxmb3J8ZnVuY3Rpb258aWZ8aW58bmV4dHxyZXBlYXR8cmV0dXJufHN3aXRjaHx3aGlsZSlcYig/IVwuKQ== */
EXPORT_TEXTPARSER bool _gen_r_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_Boolean_start_KD86VFJVRXxGQUxTRXxOVUxMfE5BfEluZnxOYU58TkFfaW50ZWdlcl98TkFfcmVhbF98TkFfY29tcGxleF98TkFfY2hhcmFjdGVyXylcYig/IVwuKQ== */
EXPORT_TEXTPARSER bool _gen_r_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_r_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_r_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_r_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_r_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_BacktickString_start_YA== */
EXPORT_TEXTPARSER bool _gen_r_BacktickString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_BacktickString_end_YA== */
EXPORT_TEXTPARSER bool _gen_r_BacktickString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_StringEscape_start_XFxbXFxcIiducnQwXXxcXHhbMC05YS1mQS1GXXsxLDJ9fFxcdVswLTlhLWZBLUZdezR9fFxcVVswLTlhLWZBLUZdezh9 */
EXPORT_TEXTPARSER bool _gen_r_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_Number_start_MFt4WF1bMC05YS1mQS1GXStbbExdP3xbMC05XSsoPzpcLlswLTldKyk/KD86W2VFXVstK10/WzAtOV0rKT9bbExpXT98XC5bMC05XSsoPzpbZUVdWy0rXT9bMC05XSspP1tsTGldPw== */
EXPORT_TEXTPARSER bool _gen_r_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_Variable_start_W2EtekEtWi5dW2EtekEtWjAtOV8uXSp8XC5bYS16QS1aLl1bYS16QS1aMC05Xy5dKg== */
EXPORT_TEXTPARSER bool _gen_r_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_Operator_start_PC18PDwtfC0+fC0+Pnw6Ojp8Ojp8JVteJVxyXG5dKiV8XCpcKnx+fFwkfEB8XHxcfHwmJnxcfHwmfCE9fDw9fD49fD09fFs9PD4hK1wtKi9eOiw7XQ== */
EXPORT_TEXTPARSER bool _gen_r_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_r_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_r_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_r_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_r_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_r_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_r_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_r_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* JAI Grammar Matchers */
/* _gen_jai_LineComment_start_XC9cL1teXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_jai_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_BlockComment_start_XC9cKg== */
EXPORT_TEXTPARSER bool _gen_jai_BlockComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_BlockComment_end_XCpcLw== */
EXPORT_TEXTPARSER bool _gen_jai_BlockComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_Directive_start_Iyg/OmltcG9ydHxhZGRfY29udGV4dHxhbGlnbnxhc3xhc3NlcnR8YmFrZV9hcmd1bWVudHN8YmFrZV9jb25zdGFudHN8YmFrZXxieXRlc3xjYWxsZXJfY29kZXxjYWxsZXJfbG9jYXRpb258Y2hhcnxjb2RlfGNvbXBpbGVfdGltZXxjb21wbGV0ZXxkdW1wfGVsc2V3aGVyZXxleHBhbmR8ZmlsZXxmaWxlcGF0aHxpZnh8aWZ8aW5zZXJ0fGludHJpbnNpY3xtb2RpZnl8bW9kdWxlX3BhcmFtZXRlcnN8bXVzdHxub19hYmN8bm9fYWxpYXN8bm9fcGFkZGluZ3xub19yZXNldHxwbGFjZXxwbGFjZWhvbGRlcnxwcm9jZWR1cmVfbmFtZXxwcm9jZWR1cmVfb2ZfY2FsbHxwcm9ncmFtX2V4cG9ydHxydW58c3BlY2lmaWVkfHN5bW1ldHJpY3x0aGlzfHRocm91Z2h8dHlwZV9pbmZvfHR5cGV8dW5zaGFyZWR8c2NvcGVfZXhwb3J0fHNjb3BlX2ZpbGV8c2NvcGVfbW9kdWxlfHN0cmluZylcYg== */
EXPORT_TEXTPARSER bool _gen_jai_Directive_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_Keyword_start_aWZcYnxpZnhcYnx0aGVuXGJ8ZWxzZVxifGNhc2VcYnxyZXR1cm5cYnxicmVha1xifGNvbnRpbnVlXGJ8d2hpbGVcYnxmb3JcYnxyZW1vdmVcYnxkZWZlclxifGNvbnRleHRcYnxwdXNoX2NvbnRleHRcYnx1c2luZ1xifHRlbXBcYnxzdHJ1Y3RcYnxlbnVtXGJ8dW5pb25cYnxjYXN0XGJ8dHJ1bmNcYnxub19jaGVja1xifHh4XGJ8aW5saW5lXGJ8aW50XGJ8dThcYnx1MTZcYnx1MzJcYnx1NjRcYnxzOFxifHMxNlxifHMzMlxifHM2NFxifGZsb2F0XGJ8ZmxvYXQzMlxifGZsb2F0NjRcYnxib29sXGJ8c3RyaW5nXGJ8dm9pZFxifENvZGVcYnxUeXBlXGI= */
EXPORT_TEXTPARSER bool _gen_jai_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_Boolean_start_dHJ1ZVxifGZhbHNlXGJ8bnVsbFxi */
EXPORT_TEXTPARSER bool _gen_jai_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_jai_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_jai_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_jai_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_Operator_start_Ojp8Oj18Onw9PT18PT18IT18PD18Pj18JiZ8XHxcfHxcKz18XC09fFwqPXxcLz18JT18Jj18XF49fFx8PXw8PD18Pj49fC0+fDw8fD4+fFs9PD4hJnxefitcLSovJVw/OjsuLF0= */
EXPORT_TEXTPARSER bool _gen_jai_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_jai_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_jai_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_StringEscape_start_XFxcXHxcXFwifFxcXCd8XFxufFxccnxcXHR8XFx1WzAtOWEtZkEtRl17NH18XFx4WzAtOWEtZkEtRl17Mn0= */
EXPORT_TEXTPARSER bool _gen_jai_StringEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_Number_start_MFt4WF1bMC05YS1mQS1GX10rXGJ8MFtiQl1bMDFfXStcYnwwW2hIXVswLTlhLWZBLUZfXStcYnxbMC05X10qXC4/WzAtOV9dKyg/OltlRV1bLStdP1swLTlfXSspP1xi */
EXPORT_TEXTPARSER bool _gen_jai_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_jai_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_jai_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_jai_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_jai_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_jai_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* VB Grammar Matchers */
/* _gen_vb_LineComment_start_J1teXHJcbl0qfHJlbVxzLio= */
EXPORT_TEXTPARSER bool _gen_vb_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_Keyword_start_YWRkaGFuZGxlclxifGFkZHJlc3NvZlxifGFsaWFzXGJ8YW5kXGJ8YW5kYWxzb1xifGFzXGJ8YnlyZWZcYnxieXZhbFxifGNhbGxcYnxjYXNlXGJ8Y2F0Y2hcYnxjbGFzc1xifGNvbnN0XGJ8Y29udGludWVcYnxkZWNsYXJlXGJ8ZGVmYXVsdFxifGRlbGVnYXRlXGJ8ZGltXGJ8ZG9cYnxlYWNoXGJ8ZWxzZVxifGVsc2VpZlxifGVuZFxifGVuZGlmXGJ8ZW51bVxifGVyYXNlXGJ8ZXJyb3JcYnxldmVudFxifGV4aXRcYnxmaW5hbGx5XGJ8Zm9yXGJ8ZnJpZW5kXGJ8ZnVuY3Rpb25cYnxnZXRcYnxnbG9iYWxcYnxnb3N1YlxifGdvdG9cYnxoYW5kbGVzXGJ8aWZcYnxpbXBsZW1lbnRzXGJ8aW1wb3J0c1xifGluXGJ8aW5oZXJpdHNcYnxpbnRlcmZhY2VcYnxpc1xifGlzbm90XGJ8bGliXGJ8bGlrZVxifGxvb3BcYnxtZVxifG1vZFxifG1vZHVsZVxifG11c3Rpbmhlcml0XGJ8bXVzdG92ZXJyaWRlXGJ8bXliYXNlXGJ8bXljbGFzc1xifG5hbWVzcGFjZVxifG5hcnJvd2luZ1xifG5leHRcYnxuZXdcYnxub3RcYnxub3RoaW5nXGJ8bm90aW5oZXJpdGFibGVcYnxub3RvdmVycmlkYWJsZVxifG9mXGJ8b2ZmXGJ8b25cYnxvcGVyYXRvclxifG9wdGlvblxifG9wdGlvbmFsXGJ8b3JcYnxvcmVsc2VcYnxvdmVybG9hZHNcYnxvdmVycmlkYWJsZVxifG92ZXJyaWRlc1xifHBhcmFtYXJyYXlcYnxwYXJ0aWFsXGJ8cHJlc2VydmVcYnxwcml2YXRlXGJ8cHJvcGVydHlcYnxwcm90ZWN0ZWRcYnxwdWJsaWNcYnxyYWlzZWV2ZW50XGJ8cmVhZG9ubHlcYnxyZWRpbVxifHJlbVxifHJlbW92ZWhhbmRsZXJcYnxyZXN1bWVcYnxyZXR1cm5cYnxzZWxlY3RcYnxzZXRcYnxzaGFkb3dzXGJ8c2hhcmVkXGJ8c3RhdGljXGJ8c3RlcFxifHN0b3BcYnxzdHJ1Y3R1cmVcYnxzdWJcYnxzeW5jbG9ja1xifHRoZW5cYnx0aHJvd1xifHRvXGJ8dHJ5XGJ8dHlwZW9mXGJ8dW5pY29kZVxifHVudGlsXGJ8dXNpbmdcYnx3aGVuXGJ8d2hpbGVcYnx3aWRlbmluZ1xifHdpdGhcYnx3aXRoZXZlbnRzXGJ8d3JpdGVvbmx5XGJ8eG9yXGI= */
EXPORT_TEXTPARSER bool _gen_vb_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_DataType_start_Ym9vbGVhblxifGJ5dGVcYnxjaGFyXGJ8Y2RhdGVcYnxkYXRlXGJ8ZGVjaW1hbFxifGRvdWJsZVxifGludGVnZXJcYnxsb25nXGJ8b2JqZWN0XGJ8c2J5dGVcYnxzaG9ydFxifHNpbmdsZVxifHN0cmluZ1xifHVpbnRlZ2VyXGJ8dWxvbmdcYnx1c2hvcnRcYg== */
EXPORT_TEXTPARSER bool _gen_vb_DataType_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_vb_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_vb_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_vb_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_EscapedQuote_start_IiI= */
EXPORT_TEXTPARSER bool _gen_vb_EscapedQuote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_Number_start_JkhbMC05YS1mQS1GXSt8Jk9bMC03XSt8WzAtOV0rKD86XC5bMC05XSspPyg/OltlRV1bLStdP1swLTldKyk/WyEjJSZAXT8= */
EXPORT_TEXTPARSER bool _gen_vb_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKg== */
EXPORT_TEXTPARSER bool _gen_vb_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_Operator_start_PDx8Pj58PD18Pj18PD58Oj18XCs9fFwtPXxbPTw+K1wtKi9cXF4mLiw7Ol0= */
EXPORT_TEXTPARSER bool _gen_vb_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_vb_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_vb_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_vb_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_vb_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_vb_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_vb_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_vb_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* SCRATCH Grammar Matchers */
/* _gen_scratch_LineComment_start_I1teXHJcbl0qfFwvXC9bXlxyXG5dKg== */
EXPORT_TEXTPARSER bool _gen_scratch_LineComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_scratch_Keyword_start_d2hlblxifGdyZWVuXGJ8ZmxhZ1xifGNsaWNrZWRcYnxrZXlcYnxwcmVzc2VkXGJ8c3BhY2VcYnxhbnlcYnxtb3ZlXGJ8c3RlcHNcYnx0dXJuXGJ8Y3dcYnxjY3dcYnxkZWdyZWVzXGJ8Z29cYnx0b1xifGdsaWRlXGJ8c2Vjc1xifHBvaW50XGJ8dG93YXJkc1xifGRpcmVjdGlvblxifGNoYW5nZVxifHNldFxifGVmZmVjdFxifHNpemVcYnxncmFwaGljXGJ8Y2xlYXJcYnxzaG93XGJ8aGlkZVxifHNheVxifHRoaW5rXGJ8c2Vjb25kc1xifHdhaXRcYnxjb3N0dW1lXGJ8YmFja2Ryb3BcYnxzd2l0Y2hcYnxuZXh0XGJ8c291bmRcYnxwbGF5XGJ8dW50aWxcYnxzdG9wXGJ8YWxsXGJ8dGhpc1xifHNwcml0ZVxifHNjcmlwdFxifGNyZWF0ZVxifGNsb25lXGJ8ZGVsZXRlXGJ8Zm9yZXZlclxifHJlcGVhdFxifGlmXGJ8dGhlblxifGVsc2VcYnxlbmRcYnxmb3JcYnxicm9hZGNhc3RcYnxtZXNzYWdlXGJ8bWVzc2FnZTFcYnxuZXdcYnxhc2tcYnxhbnN3ZXJcYnxtb3VzZVxifHhcYnx5XGJ8ZG93blxifGxvdWRuZXNzXGJ8dGltZXJcYnxyZXNldFxifHJhbmRvbVxifHBpY2tcYnxiZXR3ZWVuXGJ8am9pblxifGxldHRlclxifG9mXGJ8bGVuZ3RoXGJ8Y29udGFpbnNcYnxyb3VuZFxifHNxcnRcYnxhYnNcYnxhbmRcYnxvclxifG5vdFxifHRvdWNoaW5nXGJ8Y29sb3JcYnxkaXN0YW5jZVxifGl0ZW1cYnxsaXN0XGJ8YWRkXGJ8aW5zZXJ0XGJ8cmVwbGFjZVxifGNvdW50ZXJcYnxtb2RcYg== */
EXPORT_TEXTPARSER bool _gen_scratch_Keyword_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_scratch_Boolean_start_dHJ1ZVxifGZhbHNlXGI= */
EXPORT_TEXTPARSER bool _gen_scratch_Boolean_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_scratch_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_scratch_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_scratch_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_scratch_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_scratch_Number_start_WzAtOV0qXC4/WzAtOV0r */
EXPORT_TEXTPARSER bool _gen_scratch_Number_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_scratch_Variable_start_W2EtekEtWl9dW2EtekEtWjAtOV9dKnxcW1thLXpBLVpfXVthLXpBLVowLTlfXSpcXQ== */
EXPORT_TEXTPARSER bool _gen_scratch_Variable_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_scratch_Operator_start_Wz08PitcLSovXQ== */
EXPORT_TEXTPARSER bool _gen_scratch_Operator_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_scratch_Parenthesis_start_XCg= */
EXPORT_TEXTPARSER bool _gen_scratch_Parenthesis_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_scratch_Parenthesis_end_XCk= */
EXPORT_TEXTPARSER bool _gen_scratch_Parenthesis_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_scratch_ArrayIndex_start_XFs= */
EXPORT_TEXTPARSER bool _gen_scratch_ArrayIndex_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_scratch_ArrayIndex_end_XF0= */
EXPORT_TEXTPARSER bool _gen_scratch_ArrayIndex_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_scratch_CodeBlock_start_XHs= */
EXPORT_TEXTPARSER bool _gen_scratch_CodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_scratch_CodeBlock_end_XH0= */
EXPORT_TEXTPARSER bool _gen_scratch_CodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

/* MD Grammar Matchers */
/* _gen_md_HtmlComment_start_PCEtLQ== */
EXPORT_TEXTPARSER bool _gen_md_HtmlComment_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_HtmlComment_end_LS0+ */
EXPORT_TEXTPARSER bool _gen_md_HtmlComment_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_FencedCodeBlock_start_YGBgfH5+fg== */
EXPORT_TEXTPARSER bool _gen_md_FencedCodeBlock_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_FencedCodeBlock_end_YGBgfH5+fg== */
EXPORT_TEXTPARSER bool _gen_md_FencedCodeBlock_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Heading_start_I3sxLDZ9WyBcdF0rW15cclxuXSo= */
EXPORT_TEXTPARSER bool _gen_md_Heading_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_HorizontalRule_start_LS0tWyBcdF0qfFwqXCpcKlsgXHRdKnxfX19bIFx0XSo= */
EXPORT_TEXTPARSER bool _gen_md_HorizontalRule_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Blockquote_start_PlsgXHRdKlteXHJcbl0q */
EXPORT_TEXTPARSER bool _gen_md_Blockquote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_HtmlTag_start_PFwvP1thLXpBLVowLTk6LV0r */
EXPORT_TEXTPARSER bool _gen_md_HtmlTag_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_HtmlTag_end_XC8/Pg== */
EXPORT_TEXTPARSER bool _gen_md_HtmlTag_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_DoubleString_start_Ig== */
EXPORT_TEXTPARSER bool _gen_md_DoubleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_DoubleString_end_Ig== */
EXPORT_TEXTPARSER bool _gen_md_DoubleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_SingleString_start_Jw== */
EXPORT_TEXTPARSER bool _gen_md_SingleString_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_SingleString_end_Jw== */
EXPORT_TEXTPARSER bool _gen_md_SingleString_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Equal_start_PQ== */
EXPORT_TEXTPARSER bool _gen_md_Equal_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_AttributeName_start_W2EtekEtWjAtOTotXSs= */
EXPORT_TEXTPARSER bool _gen_md_AttributeName_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_TaskCheckbox_start_XFtbIHhYXVxd */
EXPORT_TEXTPARSER bool _gen_md_TaskCheckbox_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Footnote_start_XFtcXlthLXpBLVowLTlfLV0rXF0= */
EXPORT_TEXTPARSER bool _gen_md_Footnote_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Image_start_IVxb */
EXPORT_TEXTPARSER bool _gen_md_Image_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Image_end_XF0oPzpcKFteKV0qXCl8XFtbXlxdXSpcXSk/ */
EXPORT_TEXTPARSER bool _gen_md_Image_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Link_start_XFs= */
EXPORT_TEXTPARSER bool _gen_md_Link_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Link_end_XF0oPzpcKFteKV0qXCl8XFtbXlxdXSpcXSk/ */
EXPORT_TEXTPARSER bool _gen_md_Link_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_InlineCode_start_YA== */
EXPORT_TEXTPARSER bool _gen_md_InlineCode_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_InlineCode_end_YA== */
EXPORT_TEXTPARSER bool _gen_md_InlineCode_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Bold_start_XCpcKnxfXw== */
EXPORT_TEXTPARSER bool _gen_md_Bold_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Bold_end_XCpcKnxfXw== */
EXPORT_TEXTPARSER bool _gen_md_Bold_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Italic_start_XCp8Xw== */
EXPORT_TEXTPARSER bool _gen_md_Italic_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Italic_end_XCp8Xw== */
EXPORT_TEXTPARSER bool _gen_md_Italic_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Strikethrough_start_fn4= */
EXPORT_TEXTPARSER bool _gen_md_Strikethrough_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_Strikethrough_end_fn4= */
EXPORT_TEXTPARSER bool _gen_md_Strikethrough_end(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_UnorderedList_start_Wy0qK11bIFx0XSs= */
EXPORT_TEXTPARSER bool _gen_md_UnorderedList_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_OrderedList_start_WzAtOV0rWy4pXVsgXHRdKw== */
EXPORT_TEXTPARSER bool _gen_md_OrderedList_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_TablePipe_start_XHw= */
EXPORT_TEXTPARSER bool _gen_md_TablePipe_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);
/* _gen_md_BackslashEscape_start_XFxbXFxgKl97fVxbXF0oKSMrXC0uIXx+PiInXQ== */
EXPORT_TEXTPARSER bool _gen_md_BackslashEscape_start(enum textparser_encoding encoding, const char *start, size_t max_len, size_t *offset, size_t *length, bool is_caseless, bool only_at_start);

#ifdef __cplusplus
}
#endif
