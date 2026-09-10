"""Compile generated headers and verify their recovery defaults."""

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


if __name__ == "__main__":
    unittest.main()
