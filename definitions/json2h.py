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

    for token in list(root["tokens"].keys()):
        text += "    TextParser_" + name_lowercase + "_" + token + "," + os.linesep
    text += "};" + os.linesep
    text += "" + os.linesep

    text += "static const language_definition " + name_lowercase + "_definition = {" + os.linesep

    if "name" in root:
        text += "    .name = \"" + root["name"] + "\"," + os.linesep

    if "version" in root:
        text += "    .version = " + str(root["version"]) + "," + os.linesep

    if "emptySegmentLanguage" in root:
        text += "    .empty_segment_language = \"" + root["emptySegmentLanguage"] + "\"," + os.linesep

    if "caseSensitivity" in root:
        text += "    .case_sensitivity = " + python_bool_to_c_string(root["caseSensitivity"]) + "," + os.linesep

    if "defaultFileExtensions" in root:
        text += "    .default_file_extensions = (const char *[]) {"
        for ext in root["defaultFileExtensions"]:
            text += "\"" + ext + "\", "
        text += "NULL}," + os.linesep

    if "defaultTextEncoding" in root:
        match root["defaultTextEncoding"].lower():
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

    if "startTokens" not in root:
        print("starts_with is missing ...")
        exit(1)

    text += "    .starts_with = (int []) {"
    for token_name in root["startTokens"]:
        text += "TextParser_" + name_lowercase + "_" + token_name + "," + os.linesep + "                             "
    text += "TextParser_END}," + os.linesep

    if "otherTextInside" in root:
        text += "    .other_text_inside = " + python_bool_to_c_string(root["otherTextInside"]) + "," + os.linesep

    text += "    .tokens = (textparser_token[]) {" + os.linesep
    for token in root["tokens"]:
        current_token = root["tokens"][token]

        text += "        {" + os.linesep
        text += "            .name = \"" + token + "\"," + os.linesep

        if "type" not in current_token:
            print("token type missing for token name [" + token + "].")
            exit(1)

        text += "            .type = "
        match current_token["type"]:
            case "Group":
                text += "TEXTPARSER_TOKEN_TYPE_GROUP"
            case "GroupAllChildrenInSameOrder":
                text += "TEXTPARSER_TOKEN_TYPE_GROUP_ALL_CHILDREN_IN_SAME_ORDER"
            case "GroupOneChildOnly":
                text += "TEXTPARSER_TOKEN_TYPE_GROUP_ONE_CHILD_ONLY"
            case "SimpleToken":
                text += "TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN"
            case "StartStop":
                text += "TEXTPARSER_TOKEN_TYPE_START_STOP"
            case "StartOptStop":
                text += "TEXTPARSER_TOKEN_TYPE_START_OPT_STOP"
            case "DualStartStop":
                text += "TEXTPARSER_TOKEN_TYPE_DUAL_START_AND_STOP"
            case _:
                print("Invalid token type!")
                exit(1)

        text += "," + os.linesep

        if "startRegex" in current_token:
            text += "            .start_regex = R\"regex(" + current_token["startRegex"] + ")regex\"," + os.linesep
        elif "regex" in current_token:
            text += "            .start_regex = R\"regex(" + current_token["regex"] + ")regex\"," + os.linesep

        if "endRegex" in current_token:
            text += "            .end_regex = R\"regex(" + current_token["endRegex"] + ")regex\"," + os.linesep

        # immediate_start
        if "otherTextInside" in current_token:
            text += "            .other_text_inside = " + python_bool_to_c_string(current_token["otherTextInside"]) + "," + os.linesep

        # delete_if_only_one_child
        if "deleteIfOnlyOneChild" in current_token:
            text += "            .delete_if_only_one_child = " + python_bool_to_c_string(current_token["deleteIfOnlyOneChild"]) + "," + os.linesep

        # must_have_one_child
        if "mustHaveOneChild" in current_token:
            text += "            .must_have_one_child = " + python_bool_to_c_string(current_token["mustHaveOneChild"]) + "," + os.linesep

        # multi_line
        if "multiLine" in current_token:
            text += "            .multi_line = " + python_bool_to_c_string(current_token["multiLine"]) + "," + os.linesep

        # search_parent_end_token_last
        if "searchParentEndTokenLast" in current_token:
            text += "            .search_parent_end_token_last = " + python_bool_to_c_string(current_token["searchParentEndTokenLast"]) + "," + os.linesep

        if "textColor" in current_token:
            text += "            .text_color = " + current_token["textColor"] + "," + os.linesep
        else:
            text += "            .text_color = TEXTPARSER_NOCOLOR," + os.linesep

        if "textBackground" in current_token:
            text += "            .text_background = " + current_token["textBackground"] + "," + os.linesep
        else:
            text += "            .text_background = TEXTPARSER_NOCOLOR," + os.linesep

        if "textFlags" in current_token:
            text += "            .text_flags = " + current_token["textFlags"] + "," + os.linesep

        # nested_tokens
        if "nestedTokens" in current_token:
            text += "            .nested_tokens = (int []) {" + os.linesep
            for token_name in current_token["nestedTokens"]:
                text += "                TextParser_" + name_lowercase + "_" + token_name + "," + os.linesep
            text += "                TextParser_END" + os.linesep
            text += "            }" + os.linesep

        text += "        }," + os.linesep

    text += "        {" + os.linesep
    text += "            .name = NULL," + os.linesep
    text += "        }," + os.linesep

    text += "    }" + os.linesep
    text += "};" + os.linesep

    open(out_file, "w").write(text)


if __name__ == "__main__":
    main(sys.argv[1:])
