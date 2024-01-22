#pragma once

#include "textparser.h"
#include <stddef.h>


enum text_parser_json_tags {
    TextParser_json_Object,
    TextParser_json_ObjectMember,
    TextParser_json_Key,
    TextParser_json_Value,
    TextParser_json_Array,
    TextParser_json_ArrayItem,
    TextParser_json_String,
    TextParser_json_Number,
    TextParser_json_Boolean,
    TextParser_json_KeySeparator,
    TextParser_json_ValueSeparator,
    TextParser_json_EscapeCharacters,
};

static const language_definition json_definition = {
    .name = "json",
    .case_sensitivity = false,
    .default_file_extensions = (const char *[]){"json", NULL},
    .default_text_encoding = ADV_REGEX_TEXT_LATIN1,
    .starts_with = (int []) {TextParser_json_Object,
                             TextParser_json_Array,
                             TextParser_END},
    .tokens = (textparser_token[]) {
        {
            .name = "Object",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "\\{",
            .end_string = "\\}",
            .immediate_start = true,
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_json_ObjectMember,
                TextParser_json_ValueSeparator,
                TextParser_END
            }
        },
        {
            .name = "ObjectMember",
            .type = TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER,
            .immediate_start = true,
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_json_Key,
                TextParser_json_KeySeparator,
                TextParser_json_Value,
                TextParser_END
            }
        },
        {
            .name = "Key",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "\\\"",
            .end_string = "\\\"",
            .immediate_start = true,
        },
        {
            .name = "Value",
            .type = TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY,
            .immediate_start = true,
            .must_have_one_child = true,
            .nested_tokens = (int []) {
                TextParser_json_Object,
                TextParser_json_Array,
                TextParser_json_String,
                TextParser_json_Number,
                TextParser_json_Boolean,
                TextParser_END
            }
        },
        {
            .name = "Array",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "\\[",
            .end_string = "\\]",
            .immediate_start = true,
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_json_ArrayItem,
                TextParser_json_ValueSeparator,
                TextParser_END
            }
        },
        {
            .name = "ArrayItem",
            .type = TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY,
            .immediate_start = true,
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_json_Object,
                TextParser_json_Array,
                TextParser_json_String,
                TextParser_json_Number,
                TextParser_json_Boolean,
                TextParser_END
            }
        },
        {
            .name = "String",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "\\\"",
            .end_string = "\\\"",
            .immediate_start = true,
            .nested_tokens = (int []) {
                TextParser_json_EscapeCharacters,
                TextParser_END
            }
        },
        {
            .name = "Number",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "[-+]?[0-9]+\\.?[0-9]*",
            .immediate_start = true,
        },
        {
            .name = "Boolean",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "true|false",
            .immediate_start = true,
        },
        {
            .name = "KeySeparator",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = ":",
            .immediate_start = true,
        },
        {
            .name = "ValueSeparator",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = ",",
            .immediate_start = true,
        },
        {
            .name = "EscapeCharacters",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
            .start_string = "\\\\\\\\|\\\\\"",
        },
        {}
    }
};
