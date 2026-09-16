#!/usr/bin/env python3

import base64
import json
import os
import sys
from pathlib import Path

def python_bool_to_c_string(val):
    if val is False:
        return "false"
    elif val is True:
        return "true"
    else:
        raise ValueError("Invalid boolean value")

SUPPORTED_BOM_ITEMS = {
    "utf-8":        "TEXTPARSER_BOM_UTF_8",
    "utf-16-be":    "TEXTPARSER_BOM_UTF_16_BE",
    "utf-16-le":    "TEXTPARSER_BOM_UTF_16_LE",
    "utf-32-be":    "TEXTPARSER_BOM_UTF_32_BE",
    "utf-32-le":    "TEXTPARSER_BOM_UTF_32_LE",
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
        raise ValueError("supportedBom must be a comma-separated string or array of strings.")
    if len(items) == 0 or (len(items) == 1 and items[0] == ""):
        return "0"
    constants = []
    for item in items:
        normalized = item.lower()
        if normalized not in SUPPORTED_BOM_ITEMS:
            valid = ", ".join(sorted(SUPPORTED_BOM_ITEMS.keys()))
            raise ValueError("Invalid supportedBom item [" + item + "]. Valid options are: " + valid)
        constants.append(SUPPORTED_BOM_ITEMS[normalized])
    return " | ".join(constants)

def diagnostic_string(value):
    if not isinstance(value, str) or not value or "\0" in value:
        raise ValueError("Diagnostic strings must be nonempty and contain no NUL")
    return value

def c_diagnostic_string(value):
    return '"' + ''.join(chr(b) if 32 <= b <= 126 and b not in (34, 63, 92)
                         else "\\%03o" % b for b in value.encode("utf-8")) + '"'

def c_string_literal(value):
    if value is None:
        return "NULL"
    return c_diagnostic_string(value)

def diagnostic_initializer(owner, allowed):
    templates = {}
    if "diagnostics" in owner:
        templates = owner["diagnostics"]
    elif any(k in owner for k in ("expected", "tokenExpected", "recovered")):
        templates = owner
    if templates:
        if not isinstance(templates, dict) or set(templates) - set(allowed):
            raise ValueError("Invalid diagnostics object")
        for template in templates.values():
            if not isinstance(template, dict) or set(template) != {"code", "message"}:
                raise ValueError("Diagnostic templates require code and message")
            diagnostic_string(template["code"])
            message = diagnostic_string(template["message"])
            i, substitutions = 0, 0
            while i < len(message):
                if message[i] == "%":
                    i += 1
                    if i == len(message) or message[i] not in ("%", "s"):
                        raise ValueError("Diagnostic templates support only %s and %%")
                    substitutions += message[i] == "s"
                i += 1
            if substitutions > 1:
                raise ValueError("Diagnostic templates support at most one %s")
    fields = []
    for key, field in (("expected", "expected"), ("tokenExpected", "token_expected"), ("recovered", "recovered")):
        value = templates.get(key)
        code = c_diagnostic_string(value["code"]) if value else "NULL"
        message = c_diagnostic_string(value["message"]) if value else "NULL"
        fields.append("." + field + " = { .code = " + code + ", .message = " + message + " }")
    return "{ " + ", ".join(fields) + " }"

def parse_event_binding(binding):
    if isinstance(binding, str):
        if not binding:
            raise ValueError("Event binding handler cannot be empty")
        return binding, None
    elif isinstance(binding, dict):
        if set(binding.keys()) - {"handler", "configuration"}:
            raise ValueError("Invalid event binding keys")
        handler = binding.get("handler")
        if not isinstance(handler, str) or not handler:
            raise ValueError("Event binding requires valid handler")
        config = binding.get("configuration")
        config_str = None
        if config is not None:
            config_str = json.dumps(config, separators=(',', ':'))
        return handler, config_str
    else:
        raise ValueError("Invalid event binding")

class GrammarBuilder:
    def __init__(self, token_map, source_file_kinds, global_sync_token_count):
        self.token_map = token_map
        self.source_file_kinds = source_file_kinds
        self.global_sync_token_count = global_sync_token_count
        self.items = []
        self.named_map = {}

    def append_anonymous(self):
        idx = len(self.items)
        self.items.append({
            "id": idx,
            "name": None,
            "token_id": -1,
            "referenced_production": -1,
            "kind": None,
            "children": [],
            "predicate_name": None,
            "context_name": None,
            "context_value": 0,
            "minimum_precedence": 0,
            "recovery_insert_token": -1,
            "recovery_insert_enabled": False,
            "recovery_sync_tokens": [],
            "recovery_skip": False,
            "allow_automatic_semicolon": False,
            "expected_description": None,
            "validate_handler": None,
            "validate_configuration": None,
            "commit_handler": None,
            "commit_configuration": None,
            "recovery_handler": None,
            "recovery_configuration": None,
            "lexical_goal": None,
            "capture_name": None,
            "category": "TEXTPARSER_CST_UNKNOWN",
            "guard": None,
            "diagnostics": {},
        })
        return idx

    def parse_guard(self, obj, prod):
        if not isinstance(obj, dict) or not obj:
            raise ValueError("Invalid when guard")
        prod["kind"] = "TEXTPARSER_PROD_PREDICATE"
        if "native" in obj:
            if len(obj) != 1 or not isinstance(obj["native"], str) or not obj["native"]:
                raise ValueError("Invalid native guard")
            prod["predicate_name"] = obj["native"]
            return
        guard = {
            "line_terminator_before": 0,
            "next_tokens": [],
            "allow_eof": False,
            "next_token_text": None,
            "file_suffixes": None,
        }
        has_line = False
        has_eof = False
        for k, v in obj.items():
            if k in ("noLineTerminatorBefore", "lineTerminatorBefore"):
                if has_line or not isinstance(v, bool):
                    raise ValueError("Invalid line terminator guard")
                has_line = True
                required = v if k == "lineTerminatorBefore" else not v
                guard["line_terminator_before"] = 1 if required else -1
            elif k in ("nextTokenIn", "nextToken"):
                single = k == "nextToken"
                if guard["next_tokens"] or (not single and (not isinstance(v, list) or len(v) == 0)):
                    raise ValueError("Invalid nextToken guard")
                names = [v] if single else v
                for name in names:
                    if not isinstance(name, str) or not name:
                        raise ValueError("Invalid token name in guard")
                    if name not in self.token_map:
                        raise ValueError(f"Undefined token in guard: {name}")
                    guard["next_tokens"].append(self.token_map[name])
            elif k == "allowEOF":
                if not isinstance(v, bool):
                    raise ValueError("Invalid allowEOF guard")
                has_eof = True
                guard["allow_eof"] = v
            elif k == "nextTokenText":
                if not isinstance(v, str) or not v:
                    raise ValueError("Invalid nextTokenText guard")
                guard["next_token_text"] = v
            elif k == "sourceFileKind":
                if not isinstance(v, str) or v not in self.source_file_kinds:
                    raise ValueError(f"Invalid sourceFileKind guard: {v}")
                guard["file_suffixes"] = self.source_file_kinds[v]
            else:
                raise ValueError(f"Unknown guard key: {k}")
        if has_eof and not guard["next_tokens"]:
            raise ValueError("allowEOF requires nextTokens")
        prod["guard"] = guard

    def parse_construct_core(self, construct, prod_id):
        if not isinstance(construct, dict):
            raise ValueError("Construct must be dict")
        keys = [
            "token", "ref", "sequence", "choice", "optional", "repeat",
            "lookahead", "not", "when", "withContext", "commit", "pratt", "withGoal",
            "capture", "matchCapture"
        ]
        present = [k for k in keys if k in construct]
        if len(present) != 1:
            raise ValueError(f"Construct must have exactly one core key, found {present}")
        for k in construct:
            if k != "astKind" and k not in keys:
                raise ValueError(f"Unknown construct key: {k}")
            if k == "astKind" and not isinstance(construct["astKind"], str):
                raise ValueError("astKind must be string")

        key = present[0]
        val = construct[key]
        prod = self.items[prod_id]

        if key == "token":
            if not isinstance(val, str) or val not in self.token_map:
                raise ValueError(f"Invalid or undefined token: {val}")
            prod["kind"] = "TEXTPARSER_PROD_TOKEN"
            prod["token_id"] = self.token_map[val]
        elif key == "ref":
            if not isinstance(val, str) or val not in self.named_map:
                raise ValueError(f"Invalid or undefined ref: {val}")
            prod["kind"] = "TEXTPARSER_PROD_REF"
            prod["referenced_production"] = self.named_map[val]
        elif key in ("sequence", "choice"):
            if not isinstance(val, list):
                raise ValueError(f"{key} must be a list")
            prod["kind"] = "TEXTPARSER_PROD_SEQUENCE" if key == "sequence" else "TEXTPARSER_PROD_CHOICE"
            child_ids = []
            for child_item in val:
                child_id = self.append_anonymous()
                child_ids.append(child_id)
                self.parse_construct(child_item, child_id)
            prod["children"] = child_ids
        elif key in ("optional", "repeat", "lookahead", "not"):
            if not isinstance(val, dict):
                raise ValueError(f"{key} must be a dict")
            kind_map = {
                "optional": "TEXTPARSER_PROD_OPTIONAL",
                "repeat": "TEXTPARSER_PROD_REPEAT",
                "lookahead": "TEXTPARSER_PROD_LOOKAHEAD",
                "not": "TEXTPARSER_PROD_NOT",
            }
            prod["kind"] = kind_map[key]
            child_id = self.append_anonymous()
            prod["children"] = [child_id]
            self.parse_construct(val, child_id)
        elif key == "when":
            self.parse_guard(val, prod)
        elif key == "commit":
            if not isinstance(val, bool) or not val:
                raise ValueError("commit must be true")
            prod["kind"] = "TEXTPARSER_PROD_COMMIT"
        elif key == "pratt":
            if not isinstance(val, dict):
                raise ValueError("pratt must be a dict")
            if "primary" not in val or len(val) > 3:
                raise ValueError("Invalid pratt construct")
            if "minimumPrecedence" in val and not isinstance(val["minimumPrecedence"], int):
                raise ValueError("minimumPrecedence must be int")
            if "postfix" in val and not isinstance(val["postfix"], dict):
                raise ValueError("postfix must be dict")
            primary = val["primary"]
            postfix = val.get("postfix")
            minimum = val.get("minimumPrecedence", 0)
            prod["kind"] = "TEXTPARSER_PROD_PRATT"
            prod["minimum_precedence"] = minimum
            child_count = 2 if postfix is not None else 1
            child_ids = []
            cid0 = self.append_anonymous()
            child_ids.append(cid0)
            self.parse_construct(primary, cid0)
            if child_count == 2:
                cid1 = self.append_anonymous()
                child_ids.append(cid1)
                self.parse_construct(postfix, cid1)
            prod["children"] = child_ids
        elif key == "withGoal":
            if not isinstance(val, dict) or len(val) != 2:
                raise ValueError("withGoal must have exactly name and production")
            name = val.get("name")
            inner = val.get("production")
            if not isinstance(name, str) or not name or not isinstance(inner, dict):
                raise ValueError("Invalid withGoal construct")
            child_id = self.append_anonymous()
            prod["kind"] = "TEXTPARSER_PROD_LEXICAL_GOAL"
            prod["lexical_goal"] = name
            prod["children"] = [child_id]
            self.parse_construct(inner, child_id)
        elif key in ("capture", "matchCapture"):
            if not isinstance(val, dict):
                raise ValueError(f"{key} must be dict")
            expected_len = 3 if key == "capture" else 2
            if len(val) != expected_len:
                raise ValueError(f"Invalid {key} fields")
            name = val.get("name")
            inner = val.get("production")
            then = val.get("then")
            if not isinstance(name, str) or not name or not isinstance(inner, dict):
                raise ValueError(f"Invalid {key} construct")
            if key == "capture" and not isinstance(then, dict):
                raise ValueError("capture requires 'then' dict")
            child_count = 2 if key == "capture" else 1
            child_ids = []
            cid0 = self.append_anonymous()
            child_ids.append(cid0)
            self.parse_construct(inner, cid0)
            if child_count == 2:
                cid1 = self.append_anonymous()
                child_ids.append(cid1)
                self.parse_construct(then, cid1)
            prod["kind"] = "TEXTPARSER_PROD_CAPTURE" if key == "capture" else "TEXTPARSER_PROD_MATCH_CAPTURE"
            prod["capture_name"] = name
            prod["children"] = child_ids
        elif key == "withContext":
            if not isinstance(val, dict):
                raise ValueError("withContext must be dict")
            set_obj = val.get("set")
            if not isinstance(set_obj, dict) or not set_obj:
                raise ValueError("withContext requires nonempty set dict")
            inner_key = None
            if "ref" in val:
                inner_key = "ref"
            if "sequence" in val:
                if inner_key is not None:
                    raise ValueError("withContext cannot have both ref and sequence")
                inner_key = "sequence"
            if inner_key is None or len(val) != 2:
                raise ValueError("withContext requires set and ref/sequence")
            current_id = prod_id
            for ctx_name, ctx_val in set_obj.items():
                if not isinstance(ctx_val, (bool, int)):
                    raise ValueError("context value must be bool or int")
                child_id = self.append_anonymous()
                p = self.items[current_id]
                p["kind"] = "TEXTPARSER_PROD_CONTEXT"
                p["context_name"] = ctx_name
                p["context_value"] = int(ctx_val)
                p["children"] = [child_id]
                current_id = child_id
            self.parse_construct({inner_key: val[inner_key]}, current_id)

    def parse_construct(self, construct, prod_id):
        if not isinstance(construct, dict):
            raise ValueError("Construct must be dict")
        plain = {}
        envelope_keys = ("expect", "recover", "recoverUntil", "allowASI", "events", "diagnostics", "category")
        env = {}
        for k, v in construct.items():
            if k in envelope_keys:
                env[k] = v
            else:
                plain[k] = v
        self.parse_construct_core(plain, prod_id)
        prod = self.items[prod_id]
        if "diagnostics" in env:
            prod["diagnostics"] = env["diagnostics"]
        if "category" in env:
            cat_val = env["category"]
            cat_map = {
                "unknown": "TEXTPARSER_CST_UNKNOWN",
                "token": "TEXTPARSER_CST_TOKEN",
                "source_file": "TEXTPARSER_CST_SOURCE_FILE",
                "declaration": "TEXTPARSER_CST_DECLARATION",
                "statement": "TEXTPARSER_CST_STATEMENT",
                "expression": "TEXTPARSER_CST_EXPRESSION",
                "type": "TEXTPARSER_CST_TYPE",
                "jsx": "TEXTPARSER_CST_JSX",
                "pattern": "TEXTPARSER_CST_PATTERN",
                "other": "TEXTPARSER_CST_OTHER",
            }
            if not isinstance(cat_val, str) or cat_val not in cat_map:
                raise ValueError(f"Invalid category: {cat_val}")
            prod["category"] = cat_map[cat_val]
        if "events" in env:
            events = env["events"]
            if not isinstance(events, dict) or not events:
                raise ValueError("Invalid production events")
            for ek, ev in events.items():
                if ek == "onValidate":
                    if prod["validate_handler"] is not None:
                        raise ValueError("Duplicate onValidate")
                    prod["validate_handler"], prod["validate_configuration"] = parse_event_binding(ev)
                elif ek == "onCommit":
                    if prod["commit_handler"] is not None:
                        raise ValueError("Duplicate onCommit")
                    prod["commit_handler"], prod["commit_configuration"] = parse_event_binding(ev)
                elif ek == "onRecovery":
                    if prod["recovery_handler"] is not None:
                        raise ValueError("Duplicate onRecovery")
                    prod["recovery_handler"], prod["recovery_configuration"] = parse_event_binding(ev)
                else:
                    raise ValueError(f"Unknown production event: {ek}")
        if "expect" in env:
            if not isinstance(env["expect"], str) or not env["expect"]:
                raise ValueError("expect must be nonempty string")
            prod["expected_description"] = env["expect"]
        if "allowASI" in env:
            if not isinstance(env["allowASI"], bool):
                raise ValueError("allowASI must be bool")
            prod["allow_automatic_semicolon"] = env["allowASI"]
            if prod["allow_automatic_semicolon"] and prod["kind"] != "TEXTPARSER_PROD_TOKEN":
                raise ValueError("allowASI is only supported on token productions")
        if "recoverUntil" in env:
            r_until = env["recoverUntil"]
            if not isinstance(r_until, list) or not r_until or prod["recovery_sync_tokens"]:
                raise ValueError("Invalid recoverUntil")
            for t in r_until:
                if not isinstance(t, str) or t not in self.token_map:
                    raise ValueError(f"Undefined token in recoverUntil: {t}")
                prod["recovery_sync_tokens"].append(self.token_map[t])
            prod["recovery_skip"] = True
        if "recover" in env:
            r = env["recover"]
            if not isinstance(r, dict):
                raise ValueError("recover must be dict")
            insert = r.get("insert")
            skip = r.get("skip")
            sync = r.get("synchronize")
            if set(r.keys()) - {"insert", "skip", "synchronize"}:
                raise ValueError("Invalid recover keys")
            if insert is not None:
                if not isinstance(insert, str) or insert not in self.token_map:
                    raise ValueError(f"Undefined token in recover.insert: {insert}")
                prod["recovery_insert_token"] = self.token_map[insert]
                prod["recovery_insert_enabled"] = True
            if skip is not None:
                if not isinstance(skip, bool):
                    raise ValueError("recover.skip must be bool")
                prod["recovery_skip"] = skip
            if sync is not None:
                if not isinstance(sync, list) or not sync or prod["recovery_sync_tokens"]:
                    raise ValueError("Invalid recover.synchronize")
                for t in sync:
                    if not isinstance(t, str) or t not in self.token_map:
                        raise ValueError(f"Undefined token in recover.synchronize: {t}")
                    prod["recovery_sync_tokens"].append(self.token_map[t])
            if not prod["recovery_insert_enabled"] and not prod["recovery_skip"]:
                raise ValueError("recover requires insert or skip")
            if prod["recovery_skip"] and len(prod["recovery_sync_tokens"]) == 0 and self.global_sync_token_count == 0:
                raise ValueError("recover.skip requires sync tokens or global sync tokens")

def validate_grammar(builder):
    count = len(builder.items)
    nullable = [False] * count
    for _ in range(count):
        changed = False
        for i in range(count):
            p = builder.items[i]
            val = False
            kind = p["kind"]
            if kind == "TEXTPARSER_PROD_TOKEN":
                val = False
            elif kind == "TEXTPARSER_PROD_REF":
                val = p["referenced_production"] >= 0 and nullable[p["referenced_production"]]
            elif kind == "TEXTPARSER_PROD_SEQUENCE":
                val = True
                for c in p["children"]:
                    val = val and nullable[c]
            elif kind == "TEXTPARSER_PROD_CHOICE":
                val = False
                for c in p["children"]:
                    val = val or nullable[c]
            elif kind in ("TEXTPARSER_PROD_OPTIONAL", "TEXTPARSER_PROD_REPEAT",
                          "TEXTPARSER_PROD_LOOKAHEAD", "TEXTPARSER_PROD_NOT",
                          "TEXTPARSER_PROD_PREDICATE", "TEXTPARSER_PROD_COMMIT"):
                val = True
            elif kind in ("TEXTPARSER_PROD_CONTEXT", "TEXTPARSER_PROD_LEXICAL_GOAL", "TEXTPARSER_PROD_MATCH_CAPTURE"):
                val = len(p["children"]) == 1 and nullable[p["children"][0]]
            elif kind == "TEXTPARSER_PROD_CAPTURE":
                val = len(p["children"]) == 2 and nullable[p["children"][0]] and nullable[p["children"][1]]
            elif kind == "TEXTPARSER_PROD_PRATT":
                val = len(p["children"]) >= 1 and nullable[p["children"][0]]
            if val and not nullable[i]:
                nullable[i] = True
                changed = True
        if not changed:
            break

    for i in range(count):
        p = builder.items[i]
        if p["kind"] == "TEXTPARSER_PROD_REPEAT" and nullable[p["children"][0]]:
            raise ValueError(f"Grammar error: nullable repeat at production {i}")
        if p["kind"] == "TEXTPARSER_PROD_PRATT" and len(p["children"]) == 2 and nullable[p["children"][1]]:
            raise ValueError(f"Grammar error: nullable repeat at production {i}")

    colors = [0] * count
    def visit(u):
        if colors[u] == 1:
            return True
        if colors[u] == 2:
            return False
        colors[u] = 1
        p = builder.items[u]
        cycle = False
        kind = p["kind"]
        if kind == "TEXTPARSER_PROD_REF":
            cycle = visit(p["referenced_production"])
        elif kind == "TEXTPARSER_PROD_SEQUENCE":
            for c in p["children"]:
                cycle = visit(c)
                if cycle or not nullable[c]:
                    break
        elif kind == "TEXTPARSER_PROD_CHOICE":
            for c in p["children"]:
                cycle = visit(c)
                if cycle:
                    break
        elif kind in ("TEXTPARSER_PROD_OPTIONAL", "TEXTPARSER_PROD_REPEAT",
                      "TEXTPARSER_PROD_LOOKAHEAD", "TEXTPARSER_PROD_NOT",
                      "TEXTPARSER_PROD_CONTEXT", "TEXTPARSER_PROD_LEXICAL_GOAL",
                      "TEXTPARSER_PROD_PRATT", "TEXTPARSER_PROD_MATCH_CAPTURE"):
            cycle = visit(p["children"][0])
        elif kind == "TEXTPARSER_PROD_CAPTURE":
            cycle = visit(p["children"][0])
            if not cycle and nullable[p["children"][0]]:
                cycle = visit(p["children"][1])
        colors[u] = 2
        return cycle

    for i in range(count):
        colors = [0] * count
        if visit(i):
            raise ValueError(f"Grammar error: left recursion detected at production {i}")


def generate_header(in_file, out_file, skip_native_regex=False):
    root = json.loads(open(in_file, "r").read())
    has_lexer = "lexer" in root and isinstance(root["lexer"], dict)
    has_grammar = "grammar" in root and isinstance(root["grammar"], dict)
    has_operators = "operators" in root and isinstance(root["operators"], list)
    has_recovery = "recovery" in root and isinstance(root["recovery"], dict)

    if "tokens" not in root and has_lexer:
        normalized = {}
        for section in ("tokens", "trivia"):
            for token_name, token in root["lexer"].get(section, {}).items():
                item = dict(token)
                item["type"] = "SimpleToken"
                if "regex" in item:
                    item["startRegex"] = item.pop("regex")
                normalized[token_name] = item
        root["tokens"] = normalized
        root.setdefault("startTokens", list(normalized.keys()))

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
    token_list = list(root["tokens"].keys())
    token_map = {name: i for i, name in enumerate(token_list)}

    text += "enum text_parser_" + name_lowercase + "_tags {" + os.linesep
    for token in token_list:
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

    # Schema v2: Contextual Lexer tables
    lexer_rules = []
    lexer_modes = []
    lexer_goals = []
    initial_lexer_mode = "default"
    if has_lexer:
        lexer_obj = root["lexer"]
        initial_lexer_mode = lexer_obj.get("initialMode", "default")
        trivia_dict = lexer_obj.get("trivia", {})
        tokens_dict = lexer_obj.get("tokens", {})
        for t in token_list:
            is_triv = t in trivia_dict
            t_info = tokens_dict.get(t, {})
            prio = t_info.get("priority", 0)
            push = t_info.get("pushMode")
            pop = t_info.get("popMode", False)
            val = t_info.get("validator")
            lexer_rules.append({
                "priority": prio,
                "is_trivia": is_triv,
                "push_mode": push,
                "pop_mode": pop,
                "validator": val,
                "capture": t_info.get("capture", 0),
                "capture_flag": t_info.get("captureFlag", 0),
                "dynamic": t_info.get("dynamic", 0),
                "dynamic_trigger": t_info.get("dynamicTrigger", 0),
            })
        if "modes" in lexer_obj and isinstance(lexer_obj["modes"], dict):
            modes_dict = lexer_obj["modes"]
            for m_name, m_val in modes_dict.items():
                toks = [token_map[t] for t in m_val["tokens"]] if "tokens" in m_val else None
                triv = [token_map[t] for t in m_val["trivia"]] if "trivia" in m_val else None
                lexer_modes.append({"name": m_name, "tokens": toks, "trivia": triv})
            mode_names = set(modes_dict.keys())
            if initial_lexer_mode not in mode_names:
                raise ValueError(f"initialMode '{initial_lexer_mode}' not found in lexer modes")
            for r in lexer_rules:
                if r["push_mode"] and r["push_mode"] not in mode_names:
                    raise ValueError(f"pushMode '{r['push_mode']}' not found in lexer modes")

        if "goals" in lexer_obj and isinstance(lexer_obj["goals"], dict):
            for g_name, g_val in lexer_obj["goals"].items():
                mappings = []
                for src_name, tgt_name in g_val.items():
                    if src_name not in token_map or tgt_name not in token_map:
                        raise ValueError(f"Undefined token in goal {g_name}")
                    mappings.append((token_map[src_name], token_map[tgt_name]))
                lexer_goals.append({"name": g_name, "mappings": mappings})

    # Schema v2: Pratt operators
    op_defs = []
    if has_operators:
        role_to_role = {
            "infix": "TEXTPARSER_OP_INFIX",
            "prefix": "TEXTPARSER_OP_PREFIX",
            "postfix": "TEXTPARSER_OP_POSTFIX",
            "ternary": "TEXTPARSER_OP_TERNARY",
        }
        for item in root["operators"]:
            token_id = token_map[item["token"]]
            roles = item.get("roles") or [item["role"]]
            for role_str in roles:
                role_enum = role_to_role[role_str]
                prec_key = (role_str + "Precedence") if (role_str + "Precedence") in item else "precedence"
                prec = item[prec_key]
                assoc = "TEXTPARSER_ASSOC_RIGHT" if item.get("associativity") == "right" else "TEXTPARSER_ASSOC_LEFT"
                sec_tok = token_map[item["middleTerminator"]] if role_str == "ternary" else -1
                op_defs.append({
                    "token_id": token_id,
                    "role": role_enum,
                    "precedence": prec,
                    "associativity": assoc,
                    "secondary_token_id": sec_tok,
                    "left_validator": item.get("leftValidator"),
                    "operand_validator": item.get("operandValidator"),
                })

    # Schema v2: Recovery policy
    max_diag = 0
    max_skip = 0
    max_recovery = 0
    global_sync_tokens = []
    if has_recovery or has_grammar:
        max_diag = 100
        max_skip = 256
        max_recovery = 100
        if has_recovery:
            rec = root["recovery"]
            if "maximumDiagnostics" in rec:
                max_diag = rec["maximumDiagnostics"]
            if "maximumSkippedTokens" in rec:
                max_skip = rec["maximumSkippedTokens"]
            if "maximumRecoveryAttempts" in rec:
                max_recovery = rec["maximumRecoveryAttempts"]
            if "synchronizationTokens" in rec:
                global_sync_tokens = [token_map[t] for t in rec["synchronizationTokens"]]

    # Schema v2: Declarative grammar
    builder = None
    start_production = 0
    source_complete_handler = None
    source_complete_configuration = None
    if has_grammar:
        grammar_obj = root["grammar"]
        source_file_kinds = grammar_obj.get("sourceFileKinds", {})
        builder = GrammarBuilder(token_map, source_file_kinds, len(global_sync_tokens))
        prods_obj = grammar_obj["productions"]
        builder.named_map = {name: i for i, name in enumerate(prods_obj.keys())}
        for i, name in enumerate(prods_obj.keys()):
            builder.items.append({
                "id": i,
                "name": name,
                "token_id": -1,
                "referenced_production": -1,
                "kind": None,
                "children": [],
                "predicate_name": None,
                "context_name": None,
                "context_value": 0,
                "minimum_precedence": 0,
                "recovery_insert_token": -1,
                "recovery_insert_enabled": False,
                "recovery_sync_tokens": [],
                "recovery_skip": False,
                "allow_automatic_semicolon": False,
                "expected_description": None,
                "validate_handler": None,
                "validate_configuration": None,
                "commit_handler": None,
                "commit_configuration": None,
                "recovery_handler": None,
                "recovery_configuration": None,
                "lexical_goal": None,
                "capture_name": None,
                "category": "TEXTPARSER_CST_UNKNOWN",
                "guard": None,
                "diagnostics": {},
            })
        start_name = grammar_obj["start"]
        if start_name not in builder.named_map:
            raise ValueError(f"Start production '{start_name}' not found")
        start_production = builder.named_map[start_name]
        for i, (name, construct) in enumerate(prods_obj.items()):
            builder.parse_construct(construct, i)
        validate_grammar(builder)

        if "events" in grammar_obj:
            gevents = grammar_obj["events"]
            if "onSourceComplete" in gevents:
                source_complete_handler, source_complete_configuration = parse_event_binding(gevents["onSourceComplete"])

    # Emit C static tables for schema v2
    if has_lexer:
        text += f"static const textparser_contextual_lexer_rule {name_lowercase}_lexer_rules[] = {{" + os.linesep
        for rule in lexer_rules:
            push_str = c_string_literal(rule["push_mode"])
            val_str = c_string_literal(rule["validator"])
            text += f"    {{ .priority = {rule['priority']}, .is_trivia = {python_bool_to_c_string(rule['is_trivia'])}, .push_mode = {push_str}, .pop_mode = {python_bool_to_c_string(rule['pop_mode'])}, .validator = {val_str}, .capture = {rule['capture']}, .capture_flag = {rule['capture_flag']}, .dynamic = {rule['dynamic']}, .dynamic_trigger = {rule['dynamic_trigger']} }}," + os.linesep
        text += "};" + os.linesep + os.linesep

        if lexer_modes:
            for m_idx, mode in enumerate(lexer_modes):
                if mode["tokens"] is not None:
                    text += f"static const int {name_lowercase}_mode_{m_idx}_tokens[] = {{"
                    for tid in mode["tokens"]:
                        text += f"{tid}, "
                    text += f"TextParser_END}};" + os.linesep
                if mode["trivia"] is not None:
                    text += f"static const int {name_lowercase}_mode_{m_idx}_trivia[] = {{"
                    for tid in mode["trivia"]:
                        text += f"{tid}, "
                    text += f"TextParser_END}};" + os.linesep
            text += f"static const textparser_lexer_mode {name_lowercase}_lexer_modes[] = {{" + os.linesep
            for m_idx, mode in enumerate(lexer_modes):
                tok_ptr = f"(int *){name_lowercase}_mode_{m_idx}_tokens" if mode["tokens"] is not None else "NULL"
                triv_ptr = f"(int *){name_lowercase}_mode_{m_idx}_trivia" if mode["trivia"] is not None else "NULL"
                text += f"    {{ .name = {c_string_literal(mode['name'])}, .tokens = {tok_ptr}, .trivia = {triv_ptr} }}," + os.linesep
            text += "};" + os.linesep + os.linesep

        if lexer_goals:
            for g_idx, goal in enumerate(lexer_goals):
                text += f"static const textparser_lexer_goal_mapping {name_lowercase}_goal_{g_idx}_mappings[] = {{" + os.linesep
                for src, tgt in goal["mappings"]:
                    text += f"    {{ .source_token = {src}, .target_token = {tgt} }}," + os.linesep
                text += "};" + os.linesep
            text += f"static const textparser_lexer_goal {name_lowercase}_lexer_goals[] = {{" + os.linesep
            for g_idx, goal in enumerate(lexer_goals):
                map_ptr = f"(textparser_lexer_goal_mapping *){name_lowercase}_goal_{g_idx}_mappings"
                text += f"    {{ .name = {c_string_literal(goal['name'])}, .mapping_count = {len(goal['mappings'])}, .mappings = {map_ptr} }}," + os.linesep
            text += "};" + os.linesep + os.linesep

    if has_operators:
        text += f"static const struct textparser_operator_def {name_lowercase}_operator_definitions[] = {{" + os.linesep
        for op in op_defs:
            lval = c_string_literal(op["left_validator"])
            oval = c_string_literal(op["operand_validator"])
            text += f"    {{ .token_id = {op['token_id']}, .role = {op['role']}, .precedence = {op['precedence']}, .associativity = {op['associativity']}, .secondary_token_id = {op['secondary_token_id']}, .left_validator = {lval}, .operand_validator = {oval} }}," + os.linesep
        text += "};" + os.linesep + os.linesep

    if global_sync_tokens:
        text += f"static const int {name_lowercase}_recovery_sync_tokens[] = {{"
        for tid in global_sync_tokens:
            text += f"{tid}, "
        text += "TextParser_END};" + os.linesep + os.linesep

    if has_grammar:
        # Guards
        for p_idx, prod in enumerate(builder.items):
            guard = prod["guard"]
            if not guard:
                continue
            if guard["next_tokens"]:
                text += f"static const int {name_lowercase}_guard_{p_idx}_tokens[] = {{"
                for tid in guard["next_tokens"]:
                    text += f"{tid}, "
                text += "TextParser_END};" + os.linesep
            if guard["file_suffixes"]:
                text += f"static const char *{name_lowercase}_guard_{p_idx}_suffixes[] = {{"
                for sfx in guard["file_suffixes"]:
                    text += f"{c_string_literal(sfx)}, "
                text += "NULL};" + os.linesep
            tok_ptr = f"{name_lowercase}_guard_{p_idx}_tokens" if guard["next_tokens"] else "NULL"
            sfx_ptr = f"(const char **){name_lowercase}_guard_{p_idx}_suffixes" if guard["file_suffixes"] else "NULL"
            eof_str = python_bool_to_c_string(guard["allow_eof"])
            txt_str = c_string_literal(guard["next_token_text"])
            text += f"static const textparser_guard {name_lowercase}_guard_{p_idx} = {{" + os.linesep
            text += f"    .line_terminator_before = {guard['line_terminator_before']}," + os.linesep
            text += f"    .next_tokens = {tok_ptr}," + os.linesep
            text += f"    .next_token_count = {len(guard['next_tokens'])}," + os.linesep
            text += f"    .allow_eof = {eof_str}," + os.linesep
            text += f"    .next_token_text = {txt_str}," + os.linesep
            text += f"    .file_suffixes = {sfx_ptr}" + os.linesep
            text += "};" + os.linesep

        # Children
        for p_idx, prod in enumerate(builder.items):
            if prod["children"]:
                text += f"static const int {name_lowercase}_prod_{p_idx}_children[] = {{"
                for cid in prod["children"]:
                    text += f"{cid}, "
                text += "TextParser_END};" + os.linesep

        # Recovery sync tokens
        for p_idx, prod in enumerate(builder.items):
            if prod["recovery_sync_tokens"]:
                text += f"static const int {name_lowercase}_prod_{p_idx}_sync_tokens[] = {{"
                for tid in prod["recovery_sync_tokens"]:
                    text += f"{tid}, "
                text += "TextParser_END};" + os.linesep

        # Productions
        text += f"static const textparser_production {name_lowercase}_grammar_productions[] = {{" + os.linesep
        for p_idx, prod in enumerate(builder.items):
            name_str = c_string_literal(prod["name"])
            children_ptr = f"{name_lowercase}_prod_{p_idx}_children" if prod["children"] else "NULL"
            sync_ptr = f"{name_lowercase}_prod_{p_idx}_sync_tokens" if prod["recovery_sync_tokens"] else "NULL"
            guard_ptr = f"&{name_lowercase}_guard_{p_idx}" if prod["guard"] else "NULL"
            pred_str = c_string_literal(prod["predicate_name"])
            ctx_str = c_string_literal(prod["context_name"])
            exp_str = c_string_literal(prod["expected_description"])
            v_h = c_string_literal(prod["validate_handler"])
            v_c = c_string_literal(prod["validate_configuration"])
            c_h = c_string_literal(prod["commit_handler"])
            c_c = c_string_literal(prod["commit_configuration"])
            r_h = c_string_literal(prod["recovery_handler"])
            r_c = c_string_literal(prod["recovery_configuration"])
            l_g = c_string_literal(prod["lexical_goal"])
            c_n = c_string_literal(prod["capture_name"])
            rec_ins_en = python_bool_to_c_string(prod["recovery_insert_enabled"])
            rec_skip = python_bool_to_c_string(prod["recovery_skip"])
            asi_str = python_bool_to_c_string(prod["allow_automatic_semicolon"])
            diag_str = diagnostic_initializer(prod["diagnostics"], ("expected", "recovered"))

            text += "    {" + os.linesep
            text += f"        .id = {prod['id']}," + os.linesep
            text += f"        .name = {name_str}," + os.linesep
            text += f"        .kind = {prod['kind']}," + os.linesep
            text += f"        .children = {children_ptr}," + os.linesep
            text += f"        .child_count = {len(prod['children'])}," + os.linesep
            text += f"        .token_id = {prod['token_id']}," + os.linesep
            text += f"        .referenced_production = {prod['referenced_production']}," + os.linesep
            text += f"        .predicate_name = {pred_str}," + os.linesep
            text += f"        .context_name = {ctx_str}," + os.linesep
            text += f"        .context_value = {prod['context_value']}," + os.linesep
            text += f"        .minimum_precedence = {prod['minimum_precedence']}," + os.linesep
            text += f"        .recovery_insert_token = {prod['recovery_insert_token']}," + os.linesep
            text += f"        .recovery_insert_enabled = {rec_ins_en}," + os.linesep
            text += f"        .recovery_sync_tokens = {sync_ptr}," + os.linesep
            text += f"        .recovery_sync_token_count = {len(prod['recovery_sync_tokens'])}," + os.linesep
            text += f"        .recovery_skip = {rec_skip}," + os.linesep
            text += f"        .allow_automatic_semicolon = {asi_str}," + os.linesep
            text += f"        .expected_description = {exp_str}," + os.linesep
            text += f"        .validate_handler = {v_h}," + os.linesep
            text += f"        .validate_configuration = {v_c}," + os.linesep
            text += f"        .commit_handler = {c_h}," + os.linesep
            text += f"        .commit_configuration = {c_c}," + os.linesep
            text += f"        .recovery_handler = {r_h}," + os.linesep
            text += f"        .recovery_configuration = {r_c}," + os.linesep
            text += f"        .lexical_goal = {l_g}," + os.linesep
            text += f"        .capture_name = {c_n}," + os.linesep
            text += f"        .category = {prod['category']}," + os.linesep
            text += f"        .guard = {guard_ptr}," + os.linesep
            text += f"        .diagnostics = {diag_str}" + os.linesep
            text += "    }," + os.linesep
        text += "};" + os.linesep + os.linesep

        sc_h = c_string_literal(source_complete_handler)
        sc_c = c_string_literal(source_complete_configuration)
        text += f"static const textparser_grammar_definition {name_lowercase}_grammar_definition = {{" + os.linesep
        text += f"    .start_production = {start_production}," + os.linesep
        text += f"    .production_count = {len(builder.items)}," + os.linesep
        text += f"    .productions = (textparser_production *){name_lowercase}_grammar_productions," + os.linesep
        text += f"    .source_complete_handler = {sc_h}," + os.linesep
        text += f"    .source_complete_configuration = {sc_c}" + os.linesep
        text += "};" + os.linesep + os.linesep

    # Main language definition struct
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
            case "utf-8" | "utf8":
                text += "    .default_text_encoding = TEXTPARSER_ENCODING_UTF_8," + os.linesep
            case "unicode":
                text += "    .default_text_encoding = TEXTPARSER_ENCODING_UNICODE," + os.linesep
            case "utf-16" | "utf16":
                text += "    .default_text_encoding = TEXTPARSER_ENCODING_UTF_16," + os.linesep
            case "utf-32" | "utf32":
                text += "    .default_text_encoding = TEXTPARSER_ENCODING_UTF_32," + os.linesep
            case _:
                raise ValueError("Illegal default_text_encoding.")

    text += "    .supported_bom = " + bom_mask_to_c_string(root) + "," + os.linesep

    if "startTokens" not in root:
        raise ValueError("startTokens is missing")

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

    # Schema v2 language definition fields
    if has_grammar:
        text += f"    .grammar = (textparser_grammar_definition *)&{name_lowercase}_grammar_definition," + os.linesep
    else:
        text += "    .grammar = NULL," + os.linesep

    if has_lexer:
        text += f"    .initial_lexer_mode = {c_string_literal(initial_lexer_mode)}," + os.linesep
        text += f"    .lexer_mode_count = {len(lexer_modes)}," + os.linesep
        if lexer_modes:
            text += f"    .lexer_modes = (textparser_lexer_mode *){name_lowercase}_lexer_modes," + os.linesep
        else:
            text += "    .lexer_modes = NULL," + os.linesep
        text += f"    .lexer_goal_count = {len(lexer_goals)}," + os.linesep
        if lexer_goals:
            text += f"    .lexer_goals = (textparser_lexer_goal *){name_lowercase}_lexer_goals," + os.linesep
        else:
            text += "    .lexer_goals = NULL," + os.linesep
        text += f"    .lexer_rules = (textparser_contextual_lexer_rule *){name_lowercase}_lexer_rules," + os.linesep
    else:
        text += "    .initial_lexer_mode = NULL," + os.linesep
        text += "    .lexer_mode_count = 0," + os.linesep
        text += "    .lexer_modes = NULL," + os.linesep
        text += "    .lexer_goal_count = 0," + os.linesep
        text += "    .lexer_goals = NULL," + os.linesep
        text += "    .lexer_rules = NULL," + os.linesep

    if has_operators:
        text += f"    .operator_definition_count = {len(op_defs)}," + os.linesep
        text += f"    .operator_definitions = (struct textparser_operator_def *){name_lowercase}_operator_definitions," + os.linesep
    else:
        text += "    .operator_definition_count = 0," + os.linesep
        text += "    .operator_definitions = NULL," + os.linesep

    text += f"    .maximum_diagnostics = {max_diag}," + os.linesep
    text += f"    .maximum_skipped_tokens = {max_skip}," + os.linesep
    text += f"    .maximum_recovery_attempts = {max_recovery}," + os.linesep
    text += f"    .recovery_sync_token_count = {len(global_sync_tokens)}," + os.linesep
    if global_sync_tokens:
        text += f"    .recovery_sync_tokens = (int *){name_lowercase}_recovery_sync_tokens," + os.linesep
    else:
        text += "    .recovery_sync_tokens = NULL," + os.linesep

    text += "    .tokens = (textparser_token[]) {" + os.linesep
    for token in token_list:
        current_token = root["tokens"][token]

        text += "        {" + os.linesep
        text += "            .name = \"" + token + "\"," + os.linesep

        text += "            .type = "
        match current_token.get("type", "SimpleToken"):
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
                raise ValueError("Invalid token type!")
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

        text += "            .other_text_inside = " + python_bool_to_c_string(current_token.get("otherTextInside", False)) + "," + os.linesep
        text += "            .delete_if_only_one_child = " + python_bool_to_c_string(current_token.get("deleteIfOnlyOneChild", False)) + "," + os.linesep
        text += "            .must_have_one_child = " + python_bool_to_c_string(current_token.get("mustHaveOneChild", False)) + "," + os.linesep
        text += "            .multi_line = " + python_bool_to_c_string(current_token.get("multiLine", False)) + "," + os.linesep
        text += "            .search_parent_end_token_last = " + python_bool_to_c_string(current_token.get("searchParentEndTokenLast", False)) + "," + os.linesep

        text += "            .text_color = " + str(current_token.get("textColor", "TEXTPARSER_NOCOLOR")) + "," + os.linesep
        text += "            .text_background = " + str(current_token.get("textBackground", "TEXTPARSER_NOCOLOR")) + "," + os.linesep
        text += "            .text_flags = " + str(current_token.get("textFlags", "0")) + "," + os.linesep
        text += "            .delimiter_text_color = " + str(current_token.get("delimiterTextColor", "TEXTPARSER_NOCOLOR")) + "," + os.linesep
        text += "            .delimiter_text_background = " + str(current_token.get("delimiterTextBackground", "TEXTPARSER_NOCOLOR")) + "," + os.linesep
        text += "            .delimiter_text_flags = " + str(current_token.get("delimiterTextFlags", "0")) + "," + os.linesep

        if "nestedTokens" in current_token:
            text += "            .nested_tokens = (int []) {" + os.linesep
            for token_name in current_token["nestedTokens"]:
                text += "                TextParser_" + name_lowercase + "_" + token_name + "," + os.linesep
            text += "                TextParser_END" + os.linesep
            text += "            }," + os.linesep
        else:
            text += "            .nested_tokens = NULL," + os.linesep

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
            text += "            }," + os.linesep
        else:
            text += "            .context_nested_tokens = NULL," + os.linesep

        spelling = c_diagnostic_string(diagnostic_string(current_token["spelling"])) if "spelling" in current_token else "NULL"
        text += "            .spelling = " + spelling + "," + os.linesep
        text += "            .diagnostics = " + diagnostic_initializer(current_token, ("expected",)) + "," + os.linesep
        text += "        }," + os.linesep

    # Sentinel empty token
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
    text += "            .spelling = NULL," + os.linesep
    text += "            .diagnostics = " + diagnostic_initializer({}, ()) + "," + os.linesep
    text += "        }," + os.linesep

    text += "    }," + os.linesep
    text += "    .error_string = NULL," + os.linesep
    text += "    .string_pool = NULL," + os.linesep
    text += "    .diagnostics = " + diagnostic_initializer(root, ("expected", "tokenExpected", "recovered")) + "," + os.linesep
    text += "};" + os.linesep

    open(out_file, "w").write(text)



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

    generate_header(in_file, out_file, skip_native_regex)

if __name__ == "__main__":
    main(sys.argv[1:])
