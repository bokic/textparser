#pragma once

#include"textparser.h"
#include <stddef.h>


enum text_parser_php_tags {
    TextParser_PHP_Tag,
    TextParser_PHP_LineComment,
    TextParser_PHP_BlockComment,
    TextParser_PHP_ArrayKeyValue,
    TextParser_PHP_MemberAccess,
    TextParser_PHP_Variable,
    TextParser_PHP_CodeBlock,
    TextParser_PHP_Operator,
    TextParser_PHP_SingleString,
    TextParser_PHP_DoubleString,
    TextParser_PHP_Number,
    TextParser_PHP_Boolean
};

static const language_definition_t php_definition = {
    .name = "PHP",
    .version = 5.0,
    .empty_segment_language = "html",
    .case_sensitivity = false,
    .default_file_extensions = (const char *[]) {"php", "php5", NULL},
    .default_text_encoding = TEXTPARSE_LATIN_1,
    .starts_with = (int []) {TextParser_PHP_Tag, -1},
    .can_have_other_text_inside = true,
    .tokens = (textparse_token_t[]) {
        {.name = "PHPTag",        .start_string = "<\\?php|<\\?",                                .end_string = "\\?>",    .only_start_tag = false, .multi_line = true,  .can_have_other_text_inside = false,  .end_tag_is_optional = true,
            .nested_tokens = (int []) {TextParser_PHP_LineComment, TextParser_PHP_BlockComment, TextParser_PHP_Operator, TextParser_PHP_SingleString, TextParser_PHP_DoubleString, TextParser_PHP_Number, TextParser_PHP_Boolean, 0}
        },
        {.name = "LineComment",   .start_string = "//",                                          .end_string = "\\n|\\r", .only_start_tag = false, .multi_line = false, .can_have_other_text_inside = true,   .end_tag_is_optional = true},
        {.name = "BlockComment",  .start_string = "/*",                                          .end_string = "*/",      .only_start_tag = false, .multi_line = true,  .can_have_other_text_inside = true,   .end_tag_is_optional = false},
        {.name = "ArrayKeyValue", .start_string = "=>",                                          .end_string = NULL,      .only_start_tag = true,  .multi_line = false, .can_have_other_text_inside = false },
        {.name = "MemberAccess",  .start_string = "->",                                          .end_string = NULL,      .only_start_tag = true,  .multi_line = false, .can_have_other_text_inside = false },
        {.name = "Variable",      .start_string = "[$][a-z_][a-z0-9_]*",                         .end_string = NULL,      .only_start_tag = true,  .multi_line = false, .can_have_other_text_inside = false },
        {.name = "CodeBlock",     .start_string = "{",                                           .end_string = "}",       .only_start_tag = true,  .multi_line = true,  .can_have_other_text_inside = false,  .end_tag_is_optional = false,
            .nested_tokens = (int []) {TextParser_PHP_LineComment, TextParser_PHP_BlockComment, TextParser_PHP_Operator, TextParser_PHP_SingleString, TextParser_PHP_DoubleString, TextParser_PHP_Number, TextParser_PHP_Boolean, 0}
        },
        {.name = "Operator",      .start_string = "=",                                           .end_string = NULL,      .only_start_tag = true,  .multi_line = false, .can_have_other_text_inside = false },
        {.name = "SingleString",  .start_string = "'",                                           .end_string = "'",       .only_start_tag = false, .multi_line = false, .can_have_other_text_inside = true,   .end_tag_is_optional = false },
        {.name = "DoubleString",  .start_string = "\"",                                          .end_string = "\"",      .only_start_tag = false, .multi_line = false, .can_have_other_text_inside = true,   .end_tag_is_optional = false,
            .nested_tokens = (int []) {TextParser_PHP_Variable, TextParser_PHP_CodeBlock, 0}
        },
        {.name = "Number",        .start_string = "[-+]?[0-9]+\\.?[0-9]*",                       .end_string = NULL,      .only_start_tag = true,  .multi_line = false, .can_have_other_text_inside = false },
        {.name = "Boolean",       .start_string = "true|false",                                  .end_string = NULL,      .only_start_tag = true,  .multi_line = false, .can_have_other_text_inside = false },
        {.name = NULL,            .start_string = NULL,                                          .end_string = NULL,      .only_start_tag = false, .multi_line = false, .can_have_other_text_inside = false },
    },
};
