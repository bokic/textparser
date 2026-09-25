#!/usr/bin/env python3
"""Check literal C3 grammar fixtures with the installed reference compiler.

Run from the repository root: python3 tests/c3_compare/verify_fixtures.py
Uses syntax-only (-P) compilation; no fixture code is executed.
"""
import ast
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TESTS = ROOT / 'tests/unittests/c3_grammar_tests.cpp'
STRING = r'"(?:[^"\\]|\\.)*"'


def decode(text):
    return ''.join(ast.literal_eval(s) for s in re.findall(STRING, text))


def fixtures():
    text = TESTS.read_text()
    for name, body in re.findall(
            r'TEST_P\(C3GrammarFixture, (\w+)\) \{(.*?)^\}', text, re.M | re.S):
        rejected = name.startswith('rejects_') or name.startswith('recovers_')
        for source in re.findall(r'const char \*source =\s*((?:' + STRING + r'\s*)+);', body):
            yield name, decode(source), rejected
        for array in re.findall(r'const char \*sources\[\] = \{(.*?)\};', body, re.S):
            for source in re.findall(STRING, array):
                yield name, decode(source), rejected
        for source in re.findall(r'parse_source\(\s*((?:' + STRING + r'\s*)+)', body):
            yield name, decode(source), rejected


def main():
    version = subprocess.run(['c3c', '--version'], text=True, capture_output=True, check=True)
    print(version.stdout.splitlines()[0])
    count = failures = 0
    for name, source, rejected in fixtures():
        result = subprocess.run(['c3c', 'compile-only', '-P', '-'],
                                input='module fixture;\n' + source,
                                text=True, capture_output=True, timeout=20)
        count += 1
        if (result.returncode == 0) == rejected:
            failures += 1
            print(f'MISMATCH {name}: expected {"rejection" if rejected else "acceptance"}')
            print(source)
            print(result.stderr)
    print(f'{count - failures}/{count} fixture expectations match the C3 compiler.')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
