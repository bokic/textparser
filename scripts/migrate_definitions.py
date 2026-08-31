#!/usr/bin/env python3
"""
Definition Migration Tool: Legacy v1 -> Unified Schema v2
Migrates textparser JSON definitions from legacy format (tokens dict with StartStop,
Group, Sequence, contextNestedTokens, regexVsDivision, etc.) to the unified v2 schema.
"""

import json
import os
import sys
import glob

def migrate_definition(v1_data):
    if v1_data.get("formatVersion") == 2:
        return v1_data

    v2_data = {
        "$schema": "../schema/textparser-schema.json",
        "formatVersion": 2,
        "name": v1_data.get("name", "unnamed"),
        "version": v1_data.get("version", "1.0"),
        "caseSensitivity": v1_data.get("caseSensitivity", True),
        "defaultFileExtensions": v1_data.get("defaultFileExtensions", []),
        "defaultTextEncoding": v1_data.get("defaultTextEncoding", "utf-8"),
    }

    if "SupportedBom" in v1_data:
        v2_data["supportedBom"] = v1_data["SupportedBom"]
    elif "supportedBom" in v1_data:
        v2_data["supportedBom"] = v1_data["supportedBom"]

    if "otherTextInside" in v1_data:
        v2_data["otherTextInside"] = v1_data["otherTextInside"]

    v1_tokens = v1_data.get("tokens", {})

    lexer_tokens = {}
    trivia_tokens = {}
    grammar_productions = {}

    # Identify comments or whitespace tokens for trivia
    for name, tok in v1_tokens.items():
        tok_type = tok.get("type", "SimpleToken")
        color = tok.get("textColor")
        bg = tok.get("textBackground")
        flags = tok.get("textFlags")

        # Check if it's trivia (comments, whitespace)
        is_comment = "comment" in name.lower()
        is_whitespace = "whitespace" in name.lower() or "lineterminator" in name.lower()

        if tok_type == "SimpleToken":
            t_obj = {}
            if "startRegex" in tok:
                t_obj["regex"] = tok["startRegex"]
            if color is not None:
                t_obj["textColor"] = color
            if bg is not None:
                t_obj["textBackground"] = bg
            if flags is not None:
                t_obj["textFlags"] = flags

            if is_whitespace or is_comment:
                trivia_tokens[name] = t_obj
            else:
                lexer_tokens[name] = t_obj

            # Also create a terminal production reference
            grammar_productions[name] = {
                "token": name
            }

        elif tok_type in ("StartStop", "StartOptStop"):
            start_tok_name = f"{name}_Start"
            end_tok_name = f"{name}_End"

            start_t_obj = {"regex": tok.get("startRegex", "")}
            if tok.get("delimiterTextColor") is not None:
                start_t_obj["textColor"] = tok["delimiterTextColor"]
            elif color is not None:
                start_t_obj["textColor"] = color

            end_t_obj = {"regex": tok.get("endRegex", "")}
            if tok.get("delimiterTextColor") is not None:
                end_t_obj["textColor"] = tok["delimiterTextColor"]
            elif color is not None:
                end_t_obj["textColor"] = color

            lexer_tokens[start_tok_name] = start_t_obj
            if tok.get("endRegex"):
                lexer_tokens[end_tok_name] = end_t_obj

            # Production for StartStop
            body_choices = []
            if "nestedTokens" in tok:
                for nested in tok["nestedTokens"]:
                    body_choices.append({"ref": nested})

            seq = [{"token": start_tok_name}]
            if body_choices:
                if len(body_choices) == 1:
                    seq.append({"repeat": body_choices[0]})
                else:
                    seq.append({"repeat": {"choice": body_choices}})

            if tok.get("endRegex"):
                if tok_type == "StartOptStop":
                    seq.append({"optional": {"token": end_tok_name}})
                else:
                    seq.append({"token": end_tok_name})

            prod = {
                "astKind": name,
                "sequence": seq
            }
            grammar_productions[name] = prod

        elif tok_type in ("Group", "GroupOneChildOnly", "GroupAllChildrenInSameOrder"):
            nested = tok.get("nestedTokens", [])
            if tok_type == "GroupOneChildOnly":
                prod = {
                    "astKind": name,
                    "choice": [{"ref": n} for n in nested]
                }
            elif tok_type == "GroupAllChildrenInSameOrder":
                prod = {
                    "astKind": name,
                    "sequence": [{"ref": n} for n in nested]
                }
            else: # Group
                prod = {
                    "astKind": name,
                    "repeat": {"choice": [{"ref": n} for n in nested]} if len(nested) > 1 else ({"ref": nested[0]} if nested else {})
                }
            grammar_productions[name] = prod

        elif tok_type == "Sequence":
            nested = tok.get("nestedTokens", [])
            prod = {
                "astKind": name,
                "sequence": [{"ref": n} for n in nested]
            }
            grammar_productions[name] = prod

    # Start tokens become root production choice
    start_tokens = v1_data.get("startTokens", [])
    if start_tokens:
        grammar_productions["Root"] = {
            "choice": [{"ref": st} for st in start_tokens]
        }
        start_prod_name = "Root"
    elif grammar_productions:
        start_prod_name = next(iter(grammar_productions.keys()))
    else:
        start_prod_name = "SourceFile"

    v2_data["lexer"] = {
        "initialMode": "default",
        "tokens": lexer_tokens,
    }
    if trivia_tokens:
        v2_data["lexer"]["trivia"] = trivia_tokens

    v2_data["grammar"] = {
        "start": start_prod_name,
        "productions": grammar_productions
    }

    # Precedence rules migration
    if "operatorPrecedence" in v1_data:
        op_rules = []
        for rule in v1_data["operatorPrecedence"].get("rules", []):
            assoc = "right" if rule.get("associativity") == 1 else "left"
            for op_name in rule.get("operators", []):
                op_rules.append({
                    "token": op_name,
                    "role": "infix",
                    "associativity": assoc
                })
        if op_rules:
            v2_data["operators"] = op_rules

    return v2_data

def main():
    if len(sys.argv) < 2:
        print("Usage: migrate_definitions.py <input.json [output.json]> | --all [dir] [--output-dir <dir>] [--in-place]")
        sys.exit(1)

    if sys.argv[1] == "--all":
        args = sys.argv[2:]
        directory = "definitions"
        output_dir = None
        in_place = False
        i = 0
        while i < len(args):
            if args[i] == "--output-dir" and i + 1 < len(args):
                output_dir = args[i + 1]
                i += 2
            elif args[i] == "--in-place":
                in_place = True
                i += 1
            else:
                directory = args[i]
                i += 1

        if not in_place and output_dir is None:
            output_dir = os.path.join(directory, "schema_v2")

        if output_dir:
            os.makedirs(output_dir, exist_ok=True)

        pattern = os.path.join(directory, "*_definition.json")
        files = glob.glob(pattern)
        print(f"Found {len(files)} definition files to migrate.")
        for f in files:
            with open(f, "r", encoding="utf-8") as fp:
                try:
                    data = json.load(fp)
                except Exception as e:
                    print(f"Error reading {f}: {e}")
                    continue
            migrated = migrate_definition(data)
            if in_place:
                out_path = f
            else:
                out_path = os.path.join(output_dir, os.path.basename(f))
            with open(out_path, "w", encoding="utf-8") as fp:
                json.dump(migrated, fp, indent=2)
            print(f"Migrated {f} -> {out_path}")
    else:
        file_path = sys.argv[1]
        with open(file_path, "r", encoding="utf-8") as fp:
            data = json.load(fp)
        migrated = migrate_definition(data)
        out_path = sys.argv[2] if len(sys.argv) > 2 else file_path
        with open(out_path, "w", encoding="utf-8") as fp:
            json.dump(migrated, fp, indent=2)
        print(f"Migrated {file_path} -> {out_path}")

if __name__ == "__main__":
    main()
