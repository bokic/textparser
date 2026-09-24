#!/usr/bin/env python3
"""Syntax-check the literal Perl grammar fixtures against an installed Perl 5.42.

Only repository-owned test snippets are compiled: perl -c can execute BEGIN/use.
Temporary files stay in the project's tmp directory. Historical given/when syntax
is intentionally excluded from a 5.42 run, where the switch feature was removed.
"""
import ast
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'tests/unittests/perl_v2_grammar_tests.cpp'
DEST = ROOT / 'tmp/perl-reference'


def main():
    version = subprocess.check_output(['perl', '-e', 'print $^V'], text=True)
    print(f'Reference: {version}')
    DEST.mkdir(parents=True, exist_ok=True)
    count, skipped, failures = 0, 0, []
    tests = re.findall(r'TEST_P\(PerlGrammarFixture, (\w+)\) \{(.*?)(?=\nTEST_P|\n\} // namespace)', SOURCE.read_text(), re.S)
    for test, body in tests:
        for block, tail in re.findall(r'for \(const char \*source : \{(.*?)\}\)\s*([^;]+);', body, re.S):
            rejected = 'TEXTPARSER_MATCH_NO' in tail
            for literal in re.findall(r'"(?:[^"\\]|\\.)*"', block, re.M):
                source = ast.literal_eval(literal)
                if 'given (' in source:
                    skipped += 1
                    continue
                prefix = 'use feature qw(say state); no warnings;\n'
                if 'class Point' in source:
                    prefix += "use feature 'class'; no warnings 'experimental::class';\n"
                if re.search(r'\bsub \w+ \(\$[a-z]|\bsub \(\$[a-z]', source):
                    prefix += "use feature 'signatures';\n"
                if source.startswith('try '):
                    prefix += "use feature qw(try defer);\n"
                path = DEST / f'{count:03}_{test}.pl'
                path.write_text(prefix + source + '\n')
                result = subprocess.run(['perl', '-c', str(path)], capture_output=True, text=True)
                count += 1
                if (result.returncode != 0) != rejected:
                    failures.append((path, rejected, result.stderr))
    for path, rejected, stderr in failures:
        print(f'{path}: expected {"rejection" if rejected else "acceptance"}\n{stderr}', file=sys.stderr)
    print(f'{count - len(failures)}/{count} fixtures agree; {skipped} historical fixtures skipped.')
    return bool(failures)


if __name__ == '__main__':
    sys.exit(main())
