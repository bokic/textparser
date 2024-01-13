#pragma once

#include "textparser.h"
#include <stddef.h>


enum text_parser_json_tags {
    TextParser_json_Object,
    TextParser_json_ObjectMember,
    TextParser_json_Key,
    TextParser_json_Value,
    TextParser_json_Array,
    TextParser_json_String,
    TextParser_json_Number,
    TextParser_json_Operator,
    TextParser_json_Boolean,
    TextParser_json_KeySeparator,
    TextParser_json_ValueSeparator,
    TextParser_json_EscapeCharacters,
};

static const language_definition json_definition = {
    .name = "json",
    .case_sensitivity = false,
    .default_file_extensions = (const char *[]){"json", NULL},
    .default_text_encoding = ADV_REGEX_TEXT_UTF_8,
    .starts_with = (int []) {TextParser_json_Object,
                             TextParser_json_Array,
                             TextParser_END},
    .other_text_inside = false,
    .tokens = (textparser_token[]) {
        {
            .name = "Object",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "\\{",
            .end_string = "\\}",
            .immediate_start = true,
            .delete_if_only_one_child = false,
            .must_have_one_child = false,
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_json_ObjectMember,
                TextParser_END
            }
        },

        {
            .name = "ObjectMember",
            .type = TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER,
            .immediate_start = true,
            .delete_if_only_one_child = false,
            .must_have_one_child = false,
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
            .delete_if_only_one_child = false,
            .must_have_one_child = false,
            .multi_line = false,
            .nested_tokens = (int []) {
                TextParser_END
            }
        },

        {
            .name = "Value",
            .type = TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY,
            .immediate_start = true,
            .delete_if_only_one_child = false,
            .must_have_one_child = true,
            .multi_line = false,
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
            .name = "jsonArray",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "\\[",
            .end_string = "\\]",
            .multi_line = true,
            .nested_tokens = (int []) {
                TextParser_json_Object,
                TextParser_json_Array,
                TextParser_json_String,
                TextParser_json_Number,
                TextParser_json_Operator,
                TextParser_json_Boolean,
                TextParser_json_ValueSeparator,
                TextParser_END
            }
        },
        {
            .name = "String",
            .type = TEXTPARSER_TOKEN_TYPE_START_STOP,
            .start_string = "\\\"",
            .end_string = "\\\"",
            .nested_tokens = (int []) {
                TextParser_json_EscapeCharacters,
                TextParser_END
            }
        },
        {
            .name = "Number",
            .start_string = "[-+]?[0-9]+\\.?[0-9]*",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
        },
        {
            .name = "Operator",
            .start_string = "=",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
        },
        {
            .name = "Boolean",
            .start_string = "true|false",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
        },
        {
            .name = "KeySeparator",
            .start_string = ":",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
        },
        {
            .name = "ValueSeparator",
            .start_string = ",",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
        },
        {
            .name = "EscapeCharacters",
            .start_string = "\\\\\\\\|\\\\\"",
            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN,
        },
        {},
    },
};
