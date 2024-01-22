#!/usr/bin/python3

import json
import os
import sys


def python_bool_to_c_string(val):
    if val is False:
        return "false"
    elif val is True:
        return "true"
    else:
        print("Invalid boolean value")
        exit(1)


def main(args):
    in_file = args[0]
    out_file = in_file + ".h"
    if not in_file.endswith(".json"):
        print("Not json extension.")
        exit(1)

    root = json.loads(open(in_file, "r").read())

    text = "#pragma once" + os.linesep
    text += "" + os.linesep
    text += "#include \"textparser.h\"" + os.linesep
    text += "#include <stddef.h>" + os.linesep
    text += "" + os.linesep
    text += "" + os.linesep

    name_lowercase = root["name"].lower()

    text += "enum text_parser_" + name_lowercase + "_tags {" + os.linesep

    for token in root["tokens"]:
        text += "    TextParser_" + name_lowercase + "_" + token["name"] + "," + os.linesep
    text += "};" + os.linesep
    text += "" + os.linesep

    text += "static const language_definition " + name_lowercase + "_definition = {" + os.linesep

    if "name" in root:
        text += "    .name = \"" + root["name"] + "\"," + os.linesep

    if "case_sensitivity" in root:
        text += "    .case_sensitivity = " + python_bool_to_c_string(root["case_sensitivity"]) + "," + os.linesep

    if "file_extensions" in root:
        text += "    .default_file_extensions = (const char *[]) {"
        for ext in root["file_extensions"]:
            text += "\"" + ext + "\", "
        text += "NULL}," + os.linesep

    if "default_text_encoding" in root:
        match root["default_text_encoding"].lower():
            case "latin1":
                text += "    .default_text_encoding = ADV_REGEX_TEXT_LATIN1," + os.linesep
            case "utf-8":
                text += "    .default_text_encoding = ADV_REGEX_TEXT_UTF_8," + os.linesep
            case "unicode":
                text += "    .default_text_encoding = ADV_REGEX_TEXT_UNICODE," + os.linesep
            case "utf-16":
                text += "    .default_text_encoding = ADV_REGEX_TEXT_UTF_16," + os.linesep
            case "utf-32":
                text += "    .default_text_encoding = ADV_REGEX_TEXT_UTF_32," + os.linesep
            case _:
                print("Illegal default_text_encoding. Valid options are: latin1, utf-8, unicode, utf-16, utf-32")
                exit(1)

    if "starts_with" not in root:
        print("starts_with is missing ...")
        exit(1)

    text += "    .starts_with = (int []) {"
    for token_name in root["starts_with"]:
        text += "TextParser_" + name_lowercase + "_" + token_name + "," + os.linesep + "                             "
    text += "TextParser_END}," + os.linesep

    text += "    .tokens = (textparser_token[]) {" + os.linesep
    for token in root["tokens"]:
        text += "        {" + os.linesep
        text += "            .name = \"" + token["name"] + "\"," + os.linesep

        if "type" not in token:
            print("token type missing for token name [" + token["name"] + "].")
            exit(1)

        text += "            .type = "
        match token["type"]:
            case "group":
                text += "TEXTPARSER_TOKEN_TYPE_GROUP"
            case "group_all_children_in_same_order":
                text += "TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER"
            case "group_one_child_only":
                text += "TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY"
            case "simple":
                text += "TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN"
            case "start_stop":
                text += "TEXTPARSER_TOKEN_TYPE_START_STOP"
            case "start_opt_stop":
                text += "TEXTPARSER_TOKEN_TYPE_START_OPT_STOP"
            case "dual_start_stop":
                text += "TEXTPARSER_TOKEN_TYPE_DUAL_START_AND_STOP"
            case _:
                print("Invalid token type!")
                exit(1)

        text += "," + os.linesep

        if "start_regex" in token:
            text += "            .start_string = " + json.dumps(token["start_regex"]) + "," + os.linesep
        elif "regex" in token:
            text += "            .start_string = " + json.dumps(token["regex"]) + "," + os.linesep

        if "end_regex" in token:
            text += "            .end_string = " + json.dumps(token["end_regex"]) + "," + os.linesep

        # immediate_start
        if "immediate_start" in token:
            text += "            .immediate_start = " + python_bool_to_c_string(token["immediate_start"]) + "," + os.linesep

        # delete_if_only_one_child
        if "delete_if_only_one_child" in token:
            print("delete_if_only_one_child is not implemented!")
            exit(1)

        # must_have_one_child
        if "must_have_one_child" in token:
            text += "            .must_have_one_child = " + python_bool_to_c_string(token["must_have_one_child"]) + "," + os.linesep

        # multi_line
        if "multi_line" in token:
            text += "            .multi_line = " + python_bool_to_c_string(token["multi_line"]) + "," + os.linesep

        # search_parent_end_token_last
        if "search_parent_end_token_last" in token:
            text += "            .search_parent_end_token_last = " + python_bool_to_c_string(token["search_parent_end_token_last"]) + "," + os.linesep

        # nested_tokens
        if "nested_tokens" in token:
            text += "            .nested_tokens = (int []) {" + os.linesep
            for token_name in token["nested_tokens"]:
                text += "                TextParser_" + name_lowercase + "_" + token_name + "," + os.linesep
            text += "                TextParser_END" + os.linesep
            text += "            }" + os.linesep

        text += "        }," + os.linesep

    text += "        {}" + os.linesep
    text += "    }" + os.linesep
    text += "};" + os.linesep

    open(out_file, "w").write(text)


if __name__ == "__main__":
    main(sys.argv[1:])
