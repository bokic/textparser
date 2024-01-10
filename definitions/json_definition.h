#pragma once

#include "textparser.h"
#include <stddef.h>


enum text_parser_json_tags {
    TextParser_json_Object,
    TextParser_json_Array,
    TextParser_json_String,
    TextParser_json_Number,
    TextParser_json_Operator,
    TextParser_json_Boolean,
    TextParser_json_KeySeparator,
    TextParser_json_ValueSeparator,
    TextParser_json_EscapeCharacters,
    TextParser_json_End = -1,
};

static const language_definition json_definition = {
    .name = "json",
    .case_sensitivity = false,
    .default_file_extensions = (const char *[]){"json", NULL},
    .default_text_encoding = TEXTPARSER_LATIN_1,
    .starts_with = (int []) {TextParser_json_Object, TextParser_json_Array, TextParser_json_End},
    .other_text_inside = false,
    .tokens = (textparser_token[]) {
        {.name = "jsonObject",      .start_string = "\\{",                                         .end_string = "\\}",     .only_start_tag = false, .multi_line = true,  .can_have_other_text_inside = false, .end_tag_is_optional = false,
            .nested_tokens = (int []) {TextParser_json_Object, TextParser_json_Array, TextParser_json_String, TextParser_json_Number, TextParser_json_Operator, TextParser_json_Boolean, TextParser_json_ValueSeparator, TextParser_json_KeySeparator, TextParser_END}
        },
        {.name = "jsonArray",       .start_string = "\\[",                                         .end_string = "\\]",     .only_start_tag = false, .multi_line = true,  .can_have_other_text_inside = false, .end_tag_is_optional = false,
            .nested_tokens = (int []) {TextParser_json_Object, TextParser_json_Array, TextParser_json_String, TextParser_json_Number, TextParser_json_Operator, TextParser_json_Boolean, TextParser_json_ValueSeparator, TextParser_END}
        },
        {.name = "String",          .start_string = "\\\"",                                        .end_string = "\\\"",    .only_start_tag = false, .multi_line = false, .can_have_other_text_inside = true,  .end_tag_is_optional = false,
            .nested_tokens = (int []) {TextParser_json_EscapeCharacters, TextParser_END}
        },
        {.name = "Number",          .start_string = "[-+]?[0-9]+\\.?[0-9]*",                       .end_string = NULL,      .only_start_tag = true,  .multi_line = false, .can_have_other_text_inside = false, .end_tag_is_optional = false },
        {.name = "Operator",        .start_string = "=",                                           .end_string = NULL,      .only_start_tag = true,  .multi_line = false, .can_have_other_text_inside = false, .end_tag_is_optional = false },
        {.name = "Boolean",         .start_string = "true|false",                                  .end_string = NULL,      .only_start_tag = true,  .multi_line = false, .can_have_other_text_inside = false, .end_tag_is_optional = false },
        {.name = "KeySeparator",    .start_string = ":",                                           .end_string = NULL,      .only_start_tag = true,  .multi_line = false, .can_have_other_text_inside = false, .end_tag_is_optional = false },
        {.name = "ValueSeparator",  .start_string = ",",                                           .end_string = NULL,      .only_start_tag = true,  .multi_line = false, .can_have_other_text_inside = false, .end_tag_is_optional = false },
        {.name = "EscapeCharacters",.start_string = "\\\\\\\\|\\\\\"",                             .end_string = NULL,      .only_start_tag = true,  .multi_line = false, .can_have_other_text_inside = false, .end_tag_is_optional = false },
        {.name = NULL,              .start_string = NULL,                                          .end_string = NULL,      .only_start_tag = false, .multi_line = false, .can_have_other_text_inside = false, .end_tag_is_optional = false },
    },
};
