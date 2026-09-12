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


if __name__ == "__main__":
    unittest.main()
