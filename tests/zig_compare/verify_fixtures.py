#!/usr/bin/env python3
"""Check literal Zig grammar fixtures with the installed reference compiler.

Run from the repository root: python3 tests/zig_compare/verify_fixtures.py
Uses zig fmt for syntax and zig ast-check for literal validation.
No fixture code is executed and no compiler cache is needed.
"""
import ast
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TESTS = ROOT / 'tests/unittests/zig_v2_grammar_tests.cpp'
STRING = r'"(?:[^"\\]|\\.)*"'


def decode(text):
    return ''.join(ast.literal_eval(s) for s in re.findall(STRING, text))


def fixtures():
    text = TESTS.read_text()
    for name, body in re.findall(
            r'TEST_P\(ZigGrammarFixture, (\w+)\) \{(.*?)^\}', text, re.M | re.S):
        rejected = name.startswith('rejects_') or name.startswith('recovers_')
        for source in re.findall(r'const char \*source =\s*((?:' + STRING + r'\s*)+);', body):
            yield name, decode(source), rejected
        for array in re.findall(r'const char \*sources\[\] = \{((?:\s*' + STRING + r'\s*,?)+)\s*\};', body, re.S):
            for source in re.findall(STRING, array):
                yield name, decode(source), rejected
        for source in re.findall(r'parse_source\(\s*((?:' + STRING + r'\s*)+)', body):
            yield name, decode(source), rejected


def main():
    version = subprocess.run(['zig', 'version'], text=True, capture_output=True, check=True)
    print('Zig ' + version.stdout.strip())
    if version.stdout.strip() != '0.16.0':
        raise SystemExit('These grammar fixtures target installed Zig 0.16.0.')
    count = failures = 0
    for name, source, rejected in fixtures():
        command = ['zig', 'ast-check'] if name == 'rejects_invalid_literals' else ['zig', 'fmt', '--stdin']
        result = subprocess.run(command, input=source,
                                text=True, capture_output=True, timeout=20)
        count += 1
        if (result.returncode == 0) == rejected:
            failures += 1
            print(f'MISMATCH {name}: expected {"rejection" if rejected else "acceptance"}')
            print(source)
            print(result.stderr)
    print(f'{count - failures}/{count} fixture expectations match the Zig compiler.')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
