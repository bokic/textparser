"""Compile generated headers and compare diagnostic metadata with the C loader."""

import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class GeneratedHeaderTests(unittest.TestCase):
    def test_empty_and_single_token_definitions(self):
        (ROOT / "tmp").mkdir(exist_ok=True)
        for with_token in (False, True):
            for native_regex in (False, True):
                with self.subTest(with_token=with_token, native_regex=native_regex):
                    with tempfile.TemporaryDirectory(dir=ROOT / "tmp") as directory:
                        directory = Path(directory)
                        definition = {
                            "name": "WarningTest", "version": 1,
                            "caseSensitivity": True,
                            "defaultFileExtensions": [],
                            "defaultTextEncoding": "utf-8",
                            "otherTextInside": False,
                            "startTokens": ["Word"] if with_token else [],
                            "tokens": {"Word": {
                                "type": "SimpleToken", "startRegex": "[a-z]+",
                            }} if with_token else {},
                        }
                        source = directory / "definition.json"
                        source.write_text(json.dumps(definition))
                        command = [sys.executable, str(ROOT / "definitions/json2h.py")]
                        if not native_regex:
                            command.append("--no-native-regex")
                        subprocess.run(command + [str(source)], check=True, capture_output=True)
                        translation_unit = directory / "check.cpp"
                        translation_unit.write_text('''
#include <textparser.hpp>
#include "definition.json.h"
int main() {
    const auto &definition = warningtest_definition;
    return definition.maximum_diagnostics != 0 ||
           definition.maximum_skipped_tokens != 0 ||
           definition.maximum_recovery_attempts != 0 ||
           definition.recovery_sync_token_count != 0 ||
           definition.recovery_sync_tokens != nullptr;
}
''')
                        executable = directory / "check"
                        command = shlex.split(os.environ.get("CXX", "c++")) + [
                            "-std=gnu++23", "-Werror=missing-field-initializers",
                            "-Werror=trigraphs", "-I", str(ROOT / "include"),
                            str(translation_unit), "-o", str(executable),
                        ]
                        result = subprocess.run(command, capture_output=True, text=True)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        subprocess.run([str(executable)], check=True)

    def test_diagnostic_metadata_matches_c_loader(self):
        (ROOT / "tmp").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT / "tmp") as directory:
            directory = Path(directory)
            definition = {
                "name": "MetadataTest", "version": 2, "caseSensitivity": True,
                "defaultFileExtensions": [], "defaultTextEncoding": "utf-8",
                "otherTextInside": False, "startTokens": ["Word"],
                "diagnostics": {
                    "expected": {"code": "E_EXPECTED", "message": "Need %s; 100%%."},
                    "tokenExpected": {"code": "E_TOKEN", "message": "Token '%s' expected."},
                    "recovered": {"code": "E_RECOVER", "message": "Recovered."},
                },
                "tokens": {"Word": {
                    "type": "SimpleToken", "startRegex": "[a-z]+",
                    "spelling": 'quote"slash\\??é\n%s',
                    "diagnostics": {"expected": {"code": "WORD", "message": "Word %s missing."}},
                }},
            }
            source = directory / "metadata.json"
            source.write_text(json.dumps(definition))
            subprocess.run([sys.executable, str(ROOT / "definitions/json2h.py"),
                            "--no-native-regex", str(source)], check=True, capture_output=True)
            translation_unit = directory / "check.cpp"
            translation_unit.write_text(r'''
#include <textparser.hpp>
#include <textparser-json.h>
#include <cstring>
#include "metadata.json.h"
static bool same(const char *a, const char *b) {
    return a && b ? std::strcmp(a, b) == 0 : a == b;
}
static bool same_template(const textparser_diagnostic_template &a,
                          const textparser_diagnostic_template &b) {
    return same(a.code, b.code) && same(a.message, b.message);
}
static bool same_diagnostics(const textparser_diagnostic_templates &a,
                             const textparser_diagnostic_templates &b) {
    return same_template(a.expected, b.expected) &&
           same_template(a.token_expected, b.token_expected) &&
           same_template(a.recovered, b.recovered);
}
int main(int argc, char **argv) {
    textparser_language_definition *loaded = nullptr;
    if (argc != 2 || textparser_json_load_language_definition_from_json_file(argv[1], &loaded)) return 1;
    bool matches = same_diagnostics(metadatatest_definition.diagnostics, loaded->diagnostics) &&
                   same(metadatatest_definition.tokens[0].spelling, loaded->tokens[0].spelling) &&
                   same_diagnostics(metadatatest_definition.tokens[0].diagnostics, loaded->tokens[0].diagnostics) &&
                   metadatatest_definition.tokens[1].spelling == nullptr;
    textparser_free_language_definition(loaded);
    return matches ? 0 : 2;
}
''')
            executable = directory / "check"
            command = shlex.split(os.environ.get("CXX", "c++")) + [
                "-std=gnu++23", "-Werror=missing-field-initializers", "-Werror=trigraphs",
                "-I", str(ROOT / "include"), str(translation_unit),
                "-L", str(ROOT / "bin"), "-Wl,-rpath," + str(ROOT / "bin"),
                "-ltextparser-json", "-ltextparser", "-o", str(executable),
            ]
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            subprocess.run([str(executable), str(source)], check=True)

    def test_generator_rejects_invalid_diagnostics(self):
        (ROOT / "tmp").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT / "tmp") as directory:
            source = Path(directory) / "invalid.json"
            definition = {
                "name": "Invalid", "version": 1, "caseSensitivity": True,
                "defaultFileExtensions": [], "defaultTextEncoding": "utf-8",
                "otherTextInside": False, "startTokens": [], "tokens": {},
            }
            for message in ("%n", "%d", "%1$s", "%", "%s %s", "", None, "a\0b"):
                with self.subTest(message=message):
                    definition["diagnostics"] = {"expected": {"code": "E", "message": message}}
                    source.write_text(json.dumps(definition))
                    result = subprocess.run([sys.executable, str(ROOT / "definitions/json2h.py"),
                                             "--no-native-regex", str(source)], capture_output=True)
                    self.assertNotEqual(result.returncode, 0)

    def test_schema_v2_typescript_parity_with_c_loader(self):
        (ROOT / "tmp").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT / "tmp") as directory:
            directory = Path(directory)
            source = ROOT / "definitions/typescript_definition.json"
            generated_h = directory / "typescript_definition.json.h"
            subprocess.run([sys.executable, str(ROOT / "definitions/json2h.py"),
                            str(source)], check=True, capture_output=True)
            translation_unit = directory / "check_parity.cpp"
            translation_unit.write_text(r'''
#include <textparser.hpp>
#include <textparser-json.h>
#include <typescript.h>
#include <cassert>
#include <cstring>
#include <typescript_definition.json.h>

static bool same(const char *a, const char *b) {
    return a && b ? std::strcmp(a, b) == 0 : a == b;
}

static bool same_diag(const textparser_diagnostic_templates &a, const textparser_diagnostic_templates &b) {
    return same(a.expected.code, b.expected.code) &&
           same(a.expected.message, b.expected.message) &&
           same(a.token_expected.code, b.token_expected.code) &&
           same(a.token_expected.message, b.token_expected.message) &&
           same(a.recovered.code, b.recovered.code) &&
           same(a.recovered.message, b.recovered.message);
}

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    textparser_language_definition *loaded = nullptr;
    int err = textparser_json_load_language_definition_from_json_file(argv[1], &loaded);
    if (err != 0 || !loaded) return 1;

    const auto &stat = typescript_definition;

    // 1. Language basics
    if (!same(stat.name, loaded->name) || stat.version != loaded->version ||
        stat.case_sensitivity != loaded->case_sensitivity ||
        stat.default_text_encoding != loaded->default_text_encoding ||
        stat.supported_bom != loaded->supported_bom ||
        stat.other_text_inside != loaded->other_text_inside ||
        !same_diag(stat.diagnostics, loaded->diagnostics)) return 2;

    // 2. Lexer
    if (!same(stat.initial_lexer_mode, loaded->initial_lexer_mode) ||
        stat.lexer_mode_count != loaded->lexer_mode_count ||
        stat.lexer_goal_count != loaded->lexer_goal_count) return 3;

    for (size_t i = 0; i < stat.lexer_mode_count; i++) {
        if (!same(stat.lexer_modes[i].name, loaded->lexer_modes[i].name)) return 4;
    }

    for (size_t i = 0; loaded->tokens[i].name != nullptr; i++) {
        if (!same(stat.tokens[i].name, loaded->tokens[i].name) ||
            stat.lexer_rules[i].priority != loaded->lexer_rules[i].priority ||
            stat.lexer_rules[i].is_trivia != loaded->lexer_rules[i].is_trivia ||
            !same(stat.lexer_rules[i].push_mode, loaded->lexer_rules[i].push_mode) ||
            stat.lexer_rules[i].pop_mode != loaded->lexer_rules[i].pop_mode ||
            !same(stat.lexer_rules[i].validator, loaded->lexer_rules[i].validator)) return 5;
    }

    // 3. Operators
    if (stat.operator_definition_count != loaded->operator_definition_count) return 6;
    for (size_t i = 0; i < stat.operator_definition_count; i++) {
        if (stat.operator_definitions[i].token_id != loaded->operator_definitions[i].token_id ||
            stat.operator_definitions[i].role != loaded->operator_definitions[i].role ||
            stat.operator_definitions[i].precedence != loaded->operator_definitions[i].precedence ||
            stat.operator_definitions[i].associativity != loaded->operator_definitions[i].associativity) return 7;
    }

    // 4. Recovery
    if (stat.maximum_diagnostics != loaded->maximum_diagnostics ||
        stat.maximum_skipped_tokens != loaded->maximum_skipped_tokens ||
        stat.maximum_recovery_attempts != loaded->maximum_recovery_attempts ||
        stat.recovery_sync_token_count != loaded->recovery_sync_token_count) return 8;

    // 5. Grammar
    if (!stat.grammar || stat.grammar->start_production != loaded->grammar->start_production ||
        stat.grammar->production_count != loaded->grammar->production_count ||
        !same(stat.grammar->source_complete_handler, loaded->grammar->source_complete_handler)) return 9;

    for (size_t i = 0; i < stat.grammar->production_count; i++) {
        const auto &sp = stat.grammar->productions[i];
        const auto &lp = loaded->grammar->productions[i];
        if (sp.id != lp.id || !same(sp.name, lp.name) || sp.kind != lp.kind ||
            sp.child_count != lp.child_count || sp.token_id != lp.token_id ||
            sp.referenced_production != lp.referenced_production ||
            !same(sp.predicate_name, lp.predicate_name) ||
            !same(sp.expected_description, lp.expected_description) ||
            sp.allow_automatic_semicolon != lp.allow_automatic_semicolon) return 10;
    }

    // 6. Execution with static definition
    textparser_t handle = nullptr;
    const char *code = "const x: number = 42;";
    if (textparser_openmem(code, (int)std::strlen(code), TEXTPARSER_ENCODING_UTF_8, &handle) != 0) return 11;
    textparser_typescript_register_validators(handle);
    if (textparser_parse(handle, &stat) != 0) return 12;
    for (size_t i = 0; i < stat.operator_definition_count; i++) {
        textparser_register_operator(handle, &stat.operator_definitions[i]);
    }
    textparser_match_result res = {};
    if (textparser_execute_language_grammar(handle, &stat, &res) != 0 || res.status != TEXTPARSER_MATCH_OK) return 13;
    textparser_close(handle);

    textparser_free_language_definition(loaded);
    return 0;
}
''')
            executable = directory / "check_parity"
            command = shlex.split(os.environ.get("CXX", "c++")) + [
                "-std=gnu++23", "-Werror=missing-field-initializers", "-Werror=trigraphs",
                "-I", str(ROOT / "include"), "-I", str(ROOT / "definitions"),
                "-I", str(ROOT / "src/validation"),
                str(translation_unit),
                "-L", str(ROOT / "bin"), "-Wl,-rpath," + str(ROOT / "bin"),
                "-ltextparser_typescript", "-ltextparser-json", "-ltextparser", "-o", str(executable),
            ]
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            subprocess.run([str(executable), str(source)], check=True)

    def test_generator_rejects_invalid_schema_v2_grammars(self):
        (ROOT / "tmp").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT / "tmp") as directory:
            directory = Path(directory)
            source = directory / "invalid.json"

            base_def = {
                "name": "InvalidV2", "version": 2, "caseSensitivity": True,
                "defaultFileExtensions": [], "defaultTextEncoding": "utf-8",
                "otherTextInside": False, "startTokens": ["A"],
                "lexer": {
                    "tokens": {"A": {"regex": "a"}},
                    "trivia": {}
                },
                "grammar": {
                    "start": "Root",
                    "productions": {
                        "Root": {"ref": "Root"}  # Direct left recursion
                    }
                }
            }

            # 1. Left recursion
            source.write_text(json.dumps(base_def))
            res = subprocess.run([sys.executable, str(ROOT / "definitions/json2h.py"),
                                 "--no-native-regex", str(source)], capture_output=True)
            self.assertNotEqual(res.returncode, 0)

            # 2. Nullable repeat
            base_def["grammar"]["productions"] = {
                "Root": {"repeat": {"optional": {"token": "A"}}}
            }
            source.write_text(json.dumps(base_def))
            res = subprocess.run([sys.executable, str(ROOT / "definitions/json2h.py"),
                                 "--no-native-regex", str(source)], capture_output=True)
            self.assertNotEqual(res.returncode, 0)

            # 3. Undefined ref
            base_def["grammar"]["productions"] = {
                "Root": {"ref": "NonExistent"}
            }
            source.write_text(json.dumps(base_def))
            res = subprocess.run([sys.executable, str(ROOT / "definitions/json2h.py"),
                                 "--no-native-regex", str(source)], capture_output=True)
            self.assertNotEqual(res.returncode, 0)

            # 4. allowASI on non-token
            base_def["grammar"]["productions"] = {
                "Root": {"sequence": [{"token": "A"}], "allowASI": True}
            }
            source.write_text(json.dumps(base_def))
            res = subprocess.run([sys.executable, str(ROOT / "definitions/json2h.py"),
                                 "--no-native-regex", str(source)], capture_output=True)
            self.assertNotEqual(res.returncode, 0)


if __name__ == "__main__":
    unittest.main()

