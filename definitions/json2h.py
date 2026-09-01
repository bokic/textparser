#!/usr/bin/env python3

import base64
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


SUPPORTED_BOM_ITEMS = {
    "utf-8":        "TEXTPARSER_BOM_UTF_8",
    "utf-16-be":    "TEXTPARSER_BOM_UTF_16_BE",
    "utf-16-le":    "TEXTPARSER_BOM_UTF_16_LE",
    "utf-32-be":    "TEXTPARSER_BOM_UTF_32_BE",
    "utf-32-le":    "TEXTPARSER_BOM_UTF_32_LE",
    # "utf-7-1":      "TEXTPARSER_BOM_UTF_7_1",
    # "utf-7-2":      "TEXTPARSER_BOM_UTF_7_2",
    # "utf-7-3":      "TEXTPARSER_BOM_UTF_7_3",
    # "utf-7-4":      "TEXTPARSER_BOM_UTF_7_4",
    # "utf-7-5":      "TEXTPARSER_BOM_UTF_7_5",
    # "utf-1":        "TEXTPARSER_BOM_UTF_1",
    # "utf-ebcdic":   "TEXTPARSER_BOM_UTF_EBCDIC",
    # "utf-scsu":     "TEXTPARSER_BOM_UTF_SCSU",
    # "utf-bocu1":    "TEXTPARSER_BOM_UTF_BOCU1",
    # "gb-18030":     "TEXTPARSER_BOM_UTF_GB_18030",
}


def bom_mask_to_c_string(root):
    supported_bom = root.get("supportedBom") or root.get("SupportedBom")
    if not supported_bom:
        return "0"

    if isinstance(supported_bom, list):
        items = [str(item).strip() for item in supported_bom]
    elif isinstance(supported_bom, str):
        items = [item.strip() for item in supported_bom.split(",")]
    else:
        print("supportedBom must be a comma-separated string or array of strings.")
        exit(1)

    if len(items) == 0 or (len(items) == 1 and items[0] == ""):
        return "0"

    constants = []
    for item in items:
        normalized = item.lower()
        if normalized not in SUPPORTED_BOM_ITEMS:
            valid = ", ".join(sorted(SUPPORTED_BOM_ITEMS.keys()))
            print("Invalid supportedBom item [" + item + "]. Valid options are: " + valid)
            exit(1)
        constants.append(SUPPORTED_BOM_ITEMS[normalized])

    return " | ".join(constants)


def main(args):
    skip_native_regex = False
    in_file = None
    for arg in args:
        if arg == "--no-native-regex":
            skip_native_regex = True
        else:
            in_file = arg

    if in_file is None:
        print("No input json definition file specified.")
        print("Usage: json2h.py [--no-native-regex] <definition.json>")
        exit(1)

    out_file = in_file + ".h"
    if not in_file.endswith(".json"):
        print("Not json extension.")
        exit(1)

    root = json.loads(open(in_file, "r").read())

    text = "#pragma once" + os.linesep
    text += "" + os.linesep
    text += "#include \"textparser.h\"" + os.linesep
    if not skip_native_regex:
        text += "#include \"search_function_gen.h\"" + os.linesep
    text += "#include <stddef.h>" + os.linesep
    text += "" + os.linesep
    text += "" + os.linesep

    gen_header_content = ""
    if not skip_native_regex:
        gen_header_paths = [
            os.path.join(os.path.dirname(__file__), "..", "include", "search_function_gen.h"),
            os.path.join(os.path.dirname(__file__), "include", "search_function_gen.h"),
            os.path.join(os.path.dirname(__file__), "search_function_gen.h"),
            "include/search_function_gen.h",
            "search_function_gen.h",
        ]
        for path in gen_header_paths:
            if os.path.exists(path):
                with open(path, "r", encoding="utf-8") as gf:
                    gen_header_content = gf.read()
                break

    name_lowercase = root["name"].lower()

    text += "enum text_parser_" + name_lowercase + "_tags {" + os.linesep

    for token in list(root["tokens"].keys()):
        text += "    TextParser_" + name_lowercase + "_" + token + "," + os.linesep
    text += "};" + os.linesep
    text += "" + os.linesep

    if "mergeSignIntoNumber" in root and isinstance(root["mergeSignIntoNumber"], dict):
        merge = root["mergeSignIntoNumber"]
        for key, list_name in (("signTokens", "sign_tokens"), ("numberTokens", "number_tokens"), ("operandTokens", "operand_tokens")):
            if key in merge:
                text += "static const int " + name_lowercase + "_" + list_name + "[] = {"
                for token_name in merge[key]:
                    text += "TextParser_" + name_lowercase + "_" + token_name + ", "
                text += "TextParser_END};" + os.linesep
        text += "" + os.linesep

    if "operator_precedence" in root and isinstance(root["operator_precedence"], list) and len(root["operator_precedence"]) > 0:
        prec_list = root["operator_precedence"]
        for idx, item in enumerate(prec_list):
            text += "static const int " + name_lowercase + "_prec_ops_" + str(idx) + "[] = {"
            for op_name in item.get("operators", []):
                text += "TextParser_" + name_lowercase + "_" + op_name + ", "
            text += "TextParser_END};" + os.linesep
        text += "static const textparser_precedence_rule " + name_lowercase + "_prec_rules[] = {" + os.linesep
        for idx, item in enumerate(prec_list):
            assoc_str = "TEXTPARSER_ASSOC_RIGHT" if item.get("associativity", "").lower() == "right" else "TEXTPARSER_ASSOC_LEFT"
            text += "    { .operators = " + name_lowercase + "_prec_ops_" + str(idx) + ", .associativity = " + assoc_str + " }," + os.linesep
        text += "};" + os.linesep
        text += "static const textparser_operator_precedence " + name_lowercase + "_operator_precedence = {" + os.linesep
        text += "    .count = " + str(len(prec_list)) + "," + os.linesep
        text += "    .rules = " + name_lowercase + "_prec_rules" + os.linesep
        text += "};" + os.linesep + os.linesep

    if "regexVsDivision" in root and isinstance(root["regexVsDivision"], dict):
        reg_div = root["regexVsDivision"]
        for key, list_name in (("regexTokens", "regex_tokens"), ("divisionTokens", "division_tokens"), ("operandTokens", "operand_tokens")):
            if key in reg_div:
                text += "static const int " + name_lowercase + "_regdiv_" + list_name + "[] = {"
                for token_name in reg_div[key]:
                    text += "TextParser_" + name_lowercase + "_" + token_name + ", "
                text += "TextParser_END};" + os.linesep
        if "controlKeywords" in reg_div:
            text += "static const char *" + name_lowercase + "_regdiv_control_keywords[] = {"
            for kw in reg_div["controlKeywords"]:
                text += "\"" + kw + "\", "
            text += "NULL};" + os.linesep
        text += "static const textparser_regex_disambiguation " + name_lowercase + "_regex_disambiguation = {" + os.linesep
        text += "    .regex_tokens = " + (name_lowercase + "_regdiv_regex_tokens" if "regexTokens" in reg_div else "NULL") + "," + os.linesep
        text += "    .division_tokens = " + (name_lowercase + "_regdiv_division_tokens" if "divisionTokens" in reg_div else "NULL") + "," + os.linesep
        text += "    .operand_tokens = " + (name_lowercase + "_regdiv_operand_tokens" if "operandTokens" in reg_div else "NULL") + "," + os.linesep
        text += "    .control_keywords = " + (name_lowercase + "_regdiv_control_keywords" if "controlKeywords" in reg_div else "NULL") + os.linesep
        text += "};" + os.linesep + os.linesep

    if "templateDisambiguation" in root and isinstance(root["templateDisambiguation"], dict):
        tpl = root["templateDisambiguation"]
        for key, list_name in (("templateOpenTokens", "template_open_tokens"), ("templateCloseTokens", "template_close_tokens"), ("validInnerTokens", "template_valid_inner_tokens")):
            if key in tpl:
                text += "static const int " + name_lowercase + "_" + list_name + "[] = {"
                for token_name in tpl[key]:
                    text += "TextParser_" + name_lowercase + "_" + token_name + ", "
                text += "TextParser_END};" + os.linesep
        if "invalidInnerOperators" in tpl:
            text += "static const char *" + name_lowercase + "_template_invalid_operators[] = {"
            for op in tpl["invalidInnerOperators"]:
                text += "\"" + op + "\", "
            text += "NULL};" + os.linesep
        tpl_grp_id = ("TextParser_" + name_lowercase + "_" + tpl["templateGroupToken"]) if "templateGroupToken" in tpl else "-1"
        text += "static const textparser_template_disambiguation " + name_lowercase + "_template_disambiguation = {" + os.linesep
        text += "    .template_open_tokens = " + (name_lowercase + "_template_open_tokens" if "templateOpenTokens" in tpl else "NULL") + "," + os.linesep
        text += "    .template_close_tokens = " + (name_lowercase + "_template_close_tokens" if "templateCloseTokens" in tpl else "NULL") + "," + os.linesep
        text += "    .valid_inner_tokens = " + (name_lowercase + "_template_valid_inner_tokens" if "validInnerTokens" in tpl else "NULL") + "," + os.linesep
        text += "    .invalid_inner_operators = " + (name_lowercase + "_template_invalid_operators" if "invalidInnerOperators" in tpl else "NULL") + "," + os.linesep
        text += "    .template_group_token_id = " + tpl_grp_id + os.linesep
        text += "};" + os.linesep + os.linesep

    if "castDisambiguation" in root and isinstance(root["castDisambiguation"], dict):
        cst = root["castDisambiguation"]
        if "typeTokens" in cst:
            text += "static const int " + name_lowercase + "_cast_type_tokens[] = {"
            for token_name in cst["typeTokens"]:
                text += "TextParser_" + name_lowercase + "_" + token_name + ", "
            text += "TextParser_END};" + os.linesep
        if "typeKeywords" in cst:
            text += "static const char *" + name_lowercase + "_cast_type_keywords[] = {"
            for kw in cst["typeKeywords"]:
                text += "\"" + kw + "\", "
            text += "NULL};" + os.linesep
        if "typeSuffixes" in cst:
            text += "static const char *" + name_lowercase + "_cast_type_suffixes[] = {"
            for sfx in cst["typeSuffixes"]:
                text += "\"" + sfx + "\", "
            text += "NULL};" + os.linesep
        cast_tok_id = ("TextParser_" + name_lowercase + "_" + cst["castToken"]) if "castToken" in cst else "-1"
        text += "static const textparser_cast_disambiguation " + name_lowercase + "_cast_disambiguation = {" + os.linesep
        text += "    .type_tokens = " + (name_lowercase + "_cast_type_tokens" if "typeTokens" in cst else "NULL") + "," + os.linesep
        text += "    .type_keywords = " + (name_lowercase + "_cast_type_keywords" if "typeKeywords" in cst else "NULL") + "," + os.linesep
        text += "    .type_suffixes = " + (name_lowercase + "_cast_type_suffixes" if "typeSuffixes" in cst else "NULL") + "," + os.linesep
        text += "    .cast_token_id = " + cast_tok_id + os.linesep
        text += "};" + os.linesep + os.linesep

    if "declarationDisambiguation" in root and isinstance(root["declarationDisambiguation"], dict):
        decl = root["declarationDisambiguation"]
        for key, list_name in [("returnTypeTokens", "return_type_tokens"), ("declaratorTokens", "declarator_tokens")]:
            if key in decl:
                text += "static const int " + name_lowercase + "_declaration_" + list_name + "[] = {"
                for token_name in decl[key]:
                    text += "TextParser_" + name_lowercase + "_" + token_name + ", "
                text += "TextParser_END};" + os.linesep
        def decl_token_id(key):
            return ("TextParser_" + name_lowercase + "_" + decl[key]) if key in decl else "-1"
        text += "static const textparser_declaration_disambiguation " + name_lowercase + "_declaration_disambiguation = {" + os.linesep
        text += "    .return_type_tokens = " + (name_lowercase + "_declaration_return_type_tokens" if "returnTypeTokens" in decl else "NULL") + "," + os.linesep
        text += "    .declarator_tokens = " + (name_lowercase + "_declaration_declarator_tokens" if "declaratorTokens" in decl else "NULL") + "," + os.linesep
        text += "    .identifier_token_id = " + decl_token_id("identifierToken") + "," + os.linesep
        text += "    .type_name_token_id = " + decl_token_id("typeNameToken") + "," + os.linesep
        text += "    .function_token_id = " + decl_token_id("functionToken") + "," + os.linesep
        text += "    .parameter_list_token_id = " + decl_token_id("parameterListToken") + os.linesep
        text += "};" + os.linesep + os.linesep

    text += "static const textparser_language_definition " + name_lowercase + "_definition = {" + os.linesep

    if "name" in root:
        text += "    .name = \"" + root["name"] + "\"," + os.linesep

    if "version" in root:
        text += "    .version = " + str(root["version"]) + "," + os.linesep

    if "emptySegmentLanguage" in root:
        text += "    .empty_segment_language = \"" + root["emptySegmentLanguage"] + "\"," + os.linesep
    else:
        text += "    .empty_segment_language = NULL," + os.linesep

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
                text += "    .default_text_encoding = TEXTPARSER_ENCODING_LATIN1," + os.linesep
            case "utf-8":
                text += "    .default_text_encoding = TEXTPARSER_ENCODING_UTF_8," + os.linesep
            case "unicode":
                text += "    .default_text_encoding = TEXTPARSER_ENCODING_UNICODE," + os.linesep
            case "utf-16":
                text += "    .default_text_encoding = TEXTPARSER_ENCODING_UTF_16," + os.linesep
            case "utf-32":
                text += "    .default_text_encoding = TEXTPARSER_ENCODING_UTF_32," + os.linesep
            case _:
                print("Illegal default_text_encoding. Valid options are: latin1, utf-8, unicode, utf-16, utf-32")
                exit(1)

    text += "    .supported_bom = " + bom_mask_to_c_string(root) + "," + os.linesep

    if "startTokens" not in root:
        print("starts_with is missing ...")
        exit(1)

    text += "    .starts_with = (int []) {"
    for token_name in root["startTokens"]:
        text += "TextParser_" + name_lowercase + "_" + token_name + "," + os.linesep + "                             "
    text += "TextParser_END}," + os.linesep

    if "overrideStartTokens" in root and isinstance(root["overrideStartTokens"], list) and len(root["overrideStartTokens"]) > 0:
        text += "    .override_start_tokens = (textparser_override_start_token_rule []) {" + os.linesep
        for rule in root["overrideStartTokens"]:
            text += "        {" + os.linesep
            text += "            .file_extensions = (const char *[]) {"
            for ext in rule.get("if", {}).get("fileExtensions", []):
                text += "\"" + ext + "\", "
            text += "NULL}," + os.linesep
            text += "            .regex = R\"regex(" + rule.get("if", {}).get("regex", "") + ")regex\"," + os.linesep
            text += "            .start_tokens = (int []) {"
            for token_name in rule.get("startTokens", []):
                text += "TextParser_" + name_lowercase + "_" + token_name + ", "
            text += "TextParser_END}" + os.linesep
            text += "        }," + os.linesep
        text += "        { .file_extensions = NULL, .regex = NULL, .start_tokens = NULL }" + os.linesep
        text += "    }," + os.linesep
    else:
        text += "    .override_start_tokens = NULL," + os.linesep

    if "otherTextInside" in root:
        text += "    .other_text_inside = " + python_bool_to_c_string(root["otherTextInside"]) + "," + os.linesep

    if "mergeSignIntoNumber" in root and isinstance(root["mergeSignIntoNumber"], dict):
        merge = root["mergeSignIntoNumber"]
        text += "    .sign_merge = (textparser_sign_merge []) {{" + os.linesep
        if "signTokens" in merge:
            text += "        .sign_tokens = " + name_lowercase + "_sign_tokens," + os.linesep
        if "numberTokens" in merge:
            text += "        .number_tokens = " + name_lowercase + "_number_tokens," + os.linesep
        if "operandTokens" in merge:
            text += "        .operand_tokens = " + name_lowercase + "_operand_tokens," + os.linesep
        text += "    }}," + os.linesep
    else:
        text += "    .sign_merge = NULL," + os.linesep

    if "operator_precedence" in root and isinstance(root["operator_precedence"], list) and len(root["operator_precedence"]) > 0:
        text += "    .operator_precedence = (textparser_operator_precedence *)&" + name_lowercase + "_operator_precedence," + os.linesep
    else:
        text += "    .operator_precedence = NULL," + os.linesep

    if "regexVsDivision" in root and isinstance(root["regexVsDivision"], dict):
        text += "    .regex_disambiguation = (textparser_regex_disambiguation *)&" + name_lowercase + "_regex_disambiguation," + os.linesep
    else:
        text += "    .regex_disambiguation = NULL," + os.linesep

    if "templateDisambiguation" in root and isinstance(root["templateDisambiguation"], dict):
        text += "    .template_disambiguation = (textparser_template_disambiguation *)&" + name_lowercase + "_template_disambiguation," + os.linesep
    else:
        text += "    .template_disambiguation = NULL," + os.linesep

    if "castDisambiguation" in root and isinstance(root["castDisambiguation"], dict):
        text += "    .cast_disambiguation = (textparser_cast_disambiguation *)&" + name_lowercase + "_cast_disambiguation," + os.linesep
    else:
        text += "    .cast_disambiguation = NULL," + os.linesep

    if "declarationDisambiguation" in root and isinstance(root["declarationDisambiguation"], dict):
        text += "    .declaration_disambiguation = (textparser_declaration_disambiguation *)&" + name_lowercase + "_declaration_disambiguation," + os.linesep
    else:
        text += "    .declaration_disambiguation = NULL," + os.linesep

    # Static legacy definitions do not yet compile schema-v2 grammar tables.
    text += "    .grammar = NULL," + os.linesep
    text += "    .initial_lexer_mode = NULL," + os.linesep
    text += "    .lexer_mode_count = 0," + os.linesep
    text += "    .lexer_modes = NULL," + os.linesep
    text += "    .lexer_goal_count = 0," + os.linesep
    text += "    .lexer_goals = NULL," + os.linesep
    text += "    .lexer_rules = NULL," + os.linesep

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
            case "Sequence":
                text += "TEXTPARSER_TOKEN_TYPE_SEQUENCE"
            case _:
                print("Invalid token type!")
                exit(1)

        text += "," + os.linesep

        if "startRegex" in current_token:
            text += "            .start_regex = R\"regex(" + current_token["startRegex"] + ")regex\"," + os.linesep
        elif "regex" in current_token:
            text += "            .start_regex = R\"regex(" + current_token["regex"] + ")regex\"," + os.linesep
        else:
            text += "            .start_regex = NULL," + os.linesep

        if "endRegex" in current_token:
            text += "            .end_regex = R\"regex(" + current_token["endRegex"] + ")regex\"," + os.linesep
        else:
            text += "            .end_regex = NULL," + os.linesep

        start_regex_val = current_token.get("startRegex") or current_token.get("regex")
        end_regex_val = current_token.get("endRegex")

        has_start_regex = (start_regex_val is not None)
        has_end_regex = (end_regex_val is not None)

        start_fn_name = f"_gen_{name_lowercase}_{token}_start"
        end_fn_name = f"_gen_{name_lowercase}_{token}_end"

        if has_start_regex:
            b64_start = base64.b64encode(start_regex_val.encode("utf-8")).decode("utf-8")
            start_tag = f"{start_fn_name}_{b64_start}"
            if not skip_native_regex and start_tag in gen_header_content and start_fn_name in gen_header_content:
                text += "            .startRegexFunction = " + start_fn_name + "," + os.linesep
            else:
                text += "            .startRegexFunction = NULL," + os.linesep
        else:
            text += "            .startRegexFunction = NULL," + os.linesep

        if has_end_regex:
            b64_end = base64.b64encode(end_regex_val.encode("utf-8")).decode("utf-8")
            end_tag = f"{end_fn_name}_{b64_end}"
            if not skip_native_regex and end_tag in gen_header_content and end_fn_name in gen_header_content:
                text += "            .endRegexFunction = " + end_fn_name + "," + os.linesep
            else:
                text += "            .endRegexFunction = NULL," + os.linesep
        else:
            text += "            .endRegexFunction = NULL," + os.linesep

        # immediate_start
        if "otherTextInside" in current_token:
            text += "            .other_text_inside = " + python_bool_to_c_string(current_token["otherTextInside"]) + "," + os.linesep
        else:
            text += "            .other_text_inside = false," + os.linesep

        # delete_if_only_one_child
        if "deleteIfOnlyOneChild" in current_token:
            text += "            .delete_if_only_one_child = " + python_bool_to_c_string(current_token["deleteIfOnlyOneChild"]) + "," + os.linesep
        else:
            text += "            .delete_if_only_one_child = false," + os.linesep

        # must_have_one_child
        if "mustHaveOneChild" in current_token:
            text += "            .must_have_one_child = " + python_bool_to_c_string(current_token["mustHaveOneChild"]) + "," + os.linesep
        else:
            text += "            .must_have_one_child = false," + os.linesep

        # multi_line
        if "multiLine" in current_token:
            text += "            .multi_line = " + python_bool_to_c_string(current_token["multiLine"]) + "," + os.linesep
        else:
            text += "            .multi_line = false," + os.linesep

        # search_parent_end_token_last
        if "searchParentEndTokenLast" in current_token:
            text += "            .search_parent_end_token_last = " + python_bool_to_c_string(current_token["searchParentEndTokenLast"]) + "," + os.linesep
        else:
            text += "            .search_parent_end_token_last = false," + os.linesep

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
        else:
            text += "            .text_flags = 0," + os.linesep

        if "delimiterTextColor" in current_token:
            text += "            .delimiter_text_color = " + current_token["delimiterTextColor"] + "," + os.linesep
        else:
            text += "            .delimiter_text_color = TEXTPARSER_NOCOLOR," + os.linesep

        if "delimiterTextBackground" in current_token:
            text += "            .delimiter_text_background = " + current_token["delimiterTextBackground"] + "," + os.linesep
        else:
            text += "            .delimiter_text_background = TEXTPARSER_NOCOLOR," + os.linesep

        if "delimiterTextFlags" in current_token:
            text += "            .delimiter_text_flags = " + current_token["delimiterTextFlags"] + "," + os.linesep
        else:
            text += "            .delimiter_text_flags = 0," + os.linesep

        # nested_tokens
        if "nestedTokens" in current_token:
            text += "            .nested_tokens = (int []) {" + os.linesep
            for token_name in current_token["nestedTokens"]:
                text += "                TextParser_" + name_lowercase + "_" + token_name + "," + os.linesep
            text += "                TextParser_END" + os.linesep
            text += "            }," + os.linesep
        else:
            text += "            .nested_tokens = NULL," + os.linesep

        # context_nested_tokens
        if "contextNestedTokens" in current_token and isinstance(current_token["contextNestedTokens"], list) and len(current_token["contextNestedTokens"]) > 0:
            text += "            .context_nested_tokens = (textparser_context_nested_tokens []) {" + os.linesep
            for rule in current_token["contextNestedTokens"]:
                text += "                {" + os.linesep
                text += "                    .when_parent_in = (int []) {" + os.linesep
                for parent_name in rule.get("whenParentIn", []):
                    text += "                        TextParser_" + name_lowercase + "_" + parent_name + "," + os.linesep
                text += "                        TextParser_END" + os.linesep
                text += "                    }," + os.linesep
                text += "                    .nested_tokens = (int []) {" + os.linesep
                for token_name in rule.get("nestedTokens", []):
                    text += "                        TextParser_" + name_lowercase + "_" + token_name + "," + os.linesep
                text += "                        TextParser_END" + os.linesep
                text += "                    }" + os.linesep
                text += "                }," + os.linesep
            text += "                { .when_parent_in = NULL, .nested_tokens = NULL }" + os.linesep
            text += "            }" + os.linesep
        else:
            text += "            .context_nested_tokens = NULL" + os.linesep

        text += "        }," + os.linesep

    text += "        {" + os.linesep
    text += "            .name = NULL," + os.linesep
    text += "            .type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN," + os.linesep
    text += "            .start_regex = NULL," + os.linesep
    text += "            .end_regex = NULL," + os.linesep
    text += "            .startRegexFunction = NULL," + os.linesep
    text += "            .endRegexFunction = NULL," + os.linesep
    text += "            .other_text_inside = false," + os.linesep
    text += "            .delete_if_only_one_child = false," + os.linesep
    text += "            .must_have_one_child = false," + os.linesep
    text += "            .multi_line = false," + os.linesep
    text += "            .search_parent_end_token_last = false," + os.linesep
    text += "            .text_color = 0," + os.linesep
    text += "            .text_background = 0," + os.linesep
    text += "            .text_flags = 0," + os.linesep
    text += "            .delimiter_text_color = TEXTPARSER_NOCOLOR," + os.linesep
    text += "            .delimiter_text_background = TEXTPARSER_NOCOLOR," + os.linesep
    text += "            .delimiter_text_flags = 0," + os.linesep
    text += "            .nested_tokens = NULL," + os.linesep
    text += "            .context_nested_tokens = NULL," + os.linesep
    text += "        }," + os.linesep

    text += "    }," + os.linesep
    text += "    .error_string = NULL," + os.linesep
    text += "    .string_pool = NULL," + os.linesep
    text += "};" + os.linesep

    open(out_file, "w").write(text)


if __name__ == "__main__":
    main(sys.argv[1:])
