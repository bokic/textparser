#pragma once

#include "textparser.h"
#include <stddef.h>


enum text_parser_cfml_tags {
    TextParser_cfml_ScriptTagPair,
    TextParser_cfml_ScriptStartTag,
    TextParser_cfml_ScriptEndTag,
    TextParser_cfml_OutputTagPair,
    TextParser_cfml_OutputStartTag,
    TextParser_cfml_OutputEndTag,
    TextParser_cfml_StartTag,
    TextParser_cfml_EndTag,
    TextParser_cfml_Comment,
    TextParser_cfml_SingleString,
    TextParser_cfml_DoubleString,
    TextParser_cfml_SingleChar,
    TextParser_cfml_DoubleChar,
    TextParser_cfml_SharpChar,
    TextParser_cfml_SharpExpression,
    TextParser_cfml_Expression,
    TextParser_cfml_ScriptExpression,
    TextParser_cfml_OutputExpression,
    TextParser_cfml_ScriptBlockComment,
    TextParser_cfml_ScriptLineComment,
    TextParser_cfml_ExpressionEnd,
    TextParser_cfml_Number,
    TextParser_cfml_Boolean,
    TextParser_cfml_ObjectMember,
    TextParser_cfml_Function,
    TextParser_cfml_Separator,
    TextParser_cfml_Variable,
    TextParser_cfml_Object,
    TextParser_cfml_Assigment,
    TextParser_cfml_Operator,
    TextParser_cfml_SubExpression,
    TextParser_cfml_VariableIndex,
    TextParser_cfml_CodeBlock,
    TextParser_cfml_Keyword,
};

static const language_definition cfml_definition = {
    .name = "CFML",
    .empty_segment_language = "html",
    .case_sensitivity = false,
    .default_file_extensions = (const char *[]) {"cfm", "cfc", NULL},
    .default_text_encoding = ADV_REGEX_TEXT_LATIN1,
    .starts_with = (int []) {TextParser_cfml_ScriptTagPair,
                             TextParser_cfml_OutputTagPair,
                             TextParser_cfml_StartTag,
                             TextParser_cfml_EndTag,
                             TextParser_cfml_Comment,
                             TextParser_END},
    .tokens = (textparser_token[]) {
        {
            .name = "ScriptTagPair",
            .type = TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER,
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_cfml_ScriptStartTag,
                TextParser_cfml_ScriptExpression,
                TextParser_cfml_ScriptEndTag,
                TextParser_END
            }
        },
        {
            .name = "ScriptStartTag",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "<cfscript",
            .end_string = ">",
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_cfml_Expression,
                TextParser_END
            }
        },
        {
            .name = "ScriptEndTag",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "<\\/cfscript>",
        },
        {
            .name = "OutputTagPair",
            .type = TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER,
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_cfml_OutputStartTag,
                TextParser_cfml_OutputExpression,
                TextParser_cfml_OutputEndTag,
                TextParser_END
            }
        },
        {
            .name = "OutputStartTag",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "<cfoutput",
            .end_string = ">",
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_cfml_ScriptTagPair,
                TextParser_cfml_OutputTagPair,
                TextParser_cfml_StartTag,
                TextParser_cfml_EndTag,
                TextParser_cfml_Comment,
                TextParser_END
            }
        },
        {
            .name = "OutputEndTag",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "<\\/cfoutput>",
        },
        {
            .name = "StartTag",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "<cf[a-z0-9_]+",
            .end_string = "\\/?>",
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_cfml_Expression,
                TextParser_END
            }
        },
        {
            .name = "EndTag",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "<\\/cf(?!output|!script)",
            .end_string = ">",
            .multi_line = true,
        },
        {
            .name = "Comment",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "<!---",
            .end_string = "--->",
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_cfml_Comment,
                TextParser_END
            }
        },
        {
            .name = "SingleString",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "'",
            .end_string = "'",
            .immediate_start = true,
            .search_parent_end_token_last = true,
            .nested_tokens = (int []) {
                TextParser_cfml_SharpChar,
                TextParser_cfml_SharpExpression,
                TextParser_cfml_SingleChar,
                TextParser_END
            }
        },
        {
            .name = "DoubleString",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "\"",
            .end_string = "\"",
            .immediate_start = true,
            .search_parent_end_token_last = true,
            .nested_tokens = (int []) {
                TextParser_cfml_SharpChar,
                TextParser_cfml_SharpExpression,
                TextParser_cfml_DoubleChar,
                TextParser_END
            }
        },
        {
            .name = "SingleChar",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "''",
        },
        {
            .name = "DoubleChar",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "\"\"",
        },
        {
            .name = "SharpChar",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "##",
        },
        {
            .name = "SharpExpression",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "#",
            .end_string = "#",
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_cfml_Expression,
                TextParser_cfml_ExpressionEnd,
                TextParser_END
            }
        },
        {
            .name = "Expression",
            .type = TEXTPARSER_TOKEN_TYPE_GROUP,
            .immediate_start = true,
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_cfml_SingleString,
                TextParser_cfml_DoubleString,
                TextParser_cfml_Separator,
                TextParser_cfml_ScriptLineComment,
                TextParser_cfml_ScriptBlockComment,
                TextParser_cfml_ExpressionEnd,
                TextParser_cfml_Number,
                TextParser_cfml_Boolean,
                TextParser_cfml_Operator,
                TextParser_cfml_ObjectMember,
                TextParser_cfml_Keyword,
                TextParser_cfml_Function,
                TextParser_cfml_Object,
                TextParser_cfml_Assigment,
                TextParser_cfml_Variable,
                TextParser_cfml_SubExpression,
                TextParser_cfml_VariableIndex,
                TextParser_END
            }
        },
        {
            .name = "ScriptExpression",
            .type = TEXTPARSER_TOKEN_TYPE_GROUP,
            .immediate_start = true,
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_cfml_SingleString,
                TextParser_cfml_DoubleString,
                TextParser_cfml_Separator,
                TextParser_cfml_CodeBlock,
                TextParser_cfml_ScriptLineComment,
                TextParser_cfml_ScriptBlockComment,
                TextParser_cfml_ExpressionEnd,
                TextParser_cfml_Number,
                TextParser_cfml_Boolean,
                TextParser_cfml_Operator,
                TextParser_cfml_ObjectMember,
                TextParser_cfml_Keyword,
                TextParser_cfml_Function,
                TextParser_cfml_Object,
                TextParser_cfml_Assigment,
                TextParser_cfml_Variable,
                TextParser_cfml_SubExpression,
                TextParser_cfml_VariableIndex,
                TextParser_END
            }
        },
        {
            .name = "OutputExpression",
            .type = TEXTPARSER_TOKEN_TYPE_GROUP,
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_cfml_StartTag,
                TextParser_cfml_EndTag,
                TextParser_cfml_Comment,
                TextParser_cfml_SharpExpression,
                TextParser_END
            }
        },
        {
            .name = "ScriptBlockComment",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "\\/\\*",
            .end_string = "\\*\\/",
            .immediate_start = true,
            .multi_line = true,
        },
        {
            .name = "ScriptLineComment",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "\\/\\/.*",
            .immediate_start = true,
        },
        {
            .name = "ExpressionEnd",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = ";",
            .immediate_start = true,
        },
        {
            .name = "Number",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "[-+]?\\b[0-9]*\\.?[0-9]+\\b",
            .immediate_start = true,
        },
        {
            .name = "Boolean",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "(\\btrue\\b|\\bfalse\\b|\\byes\\b|\\bno\\b)",
            .immediate_start = true,
        },
        {
            .name = "ObjectMember",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "\\.",
            .immediate_start = true,
        },
        {
            .name = "Function",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "[a-z_]+[a-z0-9_]*[ \\t]*\\(",
            .end_string = "\\)",
            .immediate_start = true,
            .nested_tokens = (int []) {
                TextParser_cfml_Expression,
                TextParser_END
            }
        },
        {
            .name = "Separator",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = ",",
            .immediate_start = true,
        },
        {
            .name = "Variable",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "[a-z_]+[a-z0-9_]*",
            .immediate_start = true,
        },
        {
            .name = "Object",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "[a-z_]+[a-z0-9_]*\\.",
            .immediate_start = true,
        },
        {
            .name = "Assigment",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "(=|\\+=|\\-=|\\*=|/=|%=|&=)",
            .immediate_start = true,
        },
        {
            .name = "Operator",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "(\\bgreater\\s+than\\s+or\\s+equal\\s+to\\b|\\bless\\s+than\\s+or\\s+equal\\s+to\\b|\\bdoes\\s+not\\s+contain\\b|\\bgreater\\s+than\\b|\\bnot\\s+equal\\b|\\bless\\s+than\\b|\\bcontains\\b|\\bis\\s+not\\b|\\bequal\\b|\\+\\+|\\bmod\\b|\\bnot\\b|\\band\\b|\\bxor\\b|\\beqv\\b|\\bgte\\b|\\blte\\b|\\bimp\\b|\\bneq\\b|\\bis\\b|\\bor\\b|\\bgt\\b|\\bge\\b|\\blt\\b|\\ble\\b|--|\\+|\\*|\\|\\^|&&|\\|\\|\\beq\\b|\\?|-|/|&|%|:|!|=)",
            .immediate_start = true,
        },
        {
            .name = "SubExpression",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "\\(",
            .end_string = "\\)",
            .immediate_start = true,
            .nested_tokens = (int []) {
                TextParser_cfml_Expression,
                TextParser_END
            }
        },
        {
            .name = "VariableIndex",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "\\[",
            .end_string = "\\]",
            .immediate_start = true,
            .nested_tokens = (int []) {
                TextParser_cfml_Expression,
                TextParser_END
            }
        },
        {
            .name = "CodeBlock",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "\\{",
            .end_string = "\\}",
            .immediate_start = true,
            .nested_tokens = (int []) {
                TextParser_cfml_ScriptExpression,
                TextParser_END
            }
        },
        {
            .name = "Keyword",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "(\\bvar\\b|\\bfunction\\b|\\bthis\\b|try\\b|\\bcatch\\b|\\bif\\b|\\bthen\\b|\\belse\\b)",
            .immediate_start = true,
        },
        {.name = NULL}
    }
};
