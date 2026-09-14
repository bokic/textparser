#!/usr/bin/env python3
"""Generate bundled PHP function signatures from an official PHP source tree.

Usage: python3 src/validation/generate_php_functions.py /path/to/php-8.5.10
Conditional extension functions are included, regardless of the local PHP build.
"""
import argparse
import json
from pathlib import Path
import re


def arguments(value):
    """Split a macro's arguments without splitting quoted defaults or nesting."""
    parts, start, depth, quoted, escaped = [], 0, 0, False, False
    for index, char in enumerate(value):
        if escaped:
            escaped = False
        elif quoted and char == "\\":
            escaped = True
        elif char == '"':
            quoted = not quoted
        elif not quoted:
            if char == '(':
                depth += 1
            elif char == ')':
                depth -= 1
            elif char == ',' and depth == 0:
                parts.append(value[start:index].strip())
                start = index + 1
    if quoted or depth:
        raise ValueError('Unbalanced macro arguments')
    return parts + [value[start:].strip()]


def attr(kind, value=''):
    if 'OBJ' in kind:
        return 'OBJECT' if 'MASK' not in kind else 'ANY'
    value = value.strip()
    return {'IS_STRING': 'STRING', 'IS_LONG': 'INT', 'IS_DOUBLE': 'FLOAT',
            '_IS_BOOL': 'BOOLEAN', 'IS_ARRAY': 'ARRAY', 'IS_OBJECT': 'OBJECT',
            'IS_RESOURCE': 'RESOURCE', 'IS_CALLABLE': 'CALLABLE',
            'IS_MIXED': 'MIXED', 'IS_VOID': 'VOID', 'IS_NULL': 'NULL',
            'IS_FALSE': 'FALSE'}.get(value.replace('MAY_BE_', 'IS_'), 'ANY')


def signatures(source, blocks=None, aliases=None, result=None):
    """Collect arginfo signatures and function entries from one source file.

    Callers may share the ``blocks`` (arginfo name -> signature variants),
    ``aliases`` (#define name -> name) and ``result`` (function name -> signature
    variants) maps across files so function tables can reference arginfo declared
    in another header, as the hand-written SAPI tables do.
    """
    blocks = {} if blocks is None else blocks
    aliases = {} if aliases is None else aliases
    result = {} if result is None else result
    for match in re.finditer(r'^(ZEND_BEGIN_ARG\w+)\(([^\n]*)\)\s*\n(.*?)ZEND_END_ARG_INFO\(\)', source, re.M | re.S):
        kind, start, body = match.groups()
        fields = arguments(start)
        required = int(fields[3] if kind == 'ZEND_BEGIN_ARG_INFO_EX' else fields[2])
        returns = attr(kind, fields[3]) if kind != 'ZEND_BEGIN_ARG_INFO_EX' else 'ANY'
        params = []
        for macro, value in re.findall(r'^\s*(ZEND_ARG_\w+)\(([^\n]*)\)', body, re.M):
            args = arguments(value)
            params.append(('$' + args[1], len(params) < required,
                           'VARIADIC' in macro, attr(macro, args[2] if len(args) > 2 else '')))
        if required > len(params):
            raise ValueError('Required count exceeds parameter count: ' + fields[0])
        signature = (returns, params)
        variants = blocks.setdefault(fields[0], [])
        if signature not in variants:
            variants.append(signature)
    aliases.update(re.findall(r'^#define\s+(arginfo_\w+)\s+(arginfo_\w+)\s*$', source, re.M))
    for table in re.finditer(r'(?:static\s+)?const zend_function_entry\s+(?!class_)(\w+)\[\]\s*=\s*\{(.*?)\};', source, re.S):
        for macro, value in re.findall(r'^\s*(ZEND_\w+|PHP_\w+)\(([^\n]*)\)', table[2], re.M):
            args = arguments(value)
            if macro in ('ZEND_FE', 'PHP_FE'):
                name, info = args[:2]
            elif macro in ('ZEND_FALIAS', 'PHP_FALIAS'):
                name, info = args[0], args[2]
            elif macro in ('ZEND_RAW_FENTRY', 'PHP_RAW_FENTRY'):
                info = args[2]
                if args[0].startswith('ZEND_NS_NAME('):
                    name = '\\'.join(json.loads(part) for part in arguments(args[0][13:-1]))
                else:
                    name = json.loads(args[0])
            else:
                raise ValueError('Unsupported function entry: ' + macro)
            visited = set()
            while info in aliases:
                if info in visited:
                    raise ValueError('Alias cycle: ' + info)
                visited.add(info)
                info = aliases[info]
            variants = result.setdefault(name.lower(), [])
            for signature in blocks[info]:
                if signature not in variants:
                    variants.append(signature)
    return result


def generate(root):
    version = re.search(r'#define PHP_VERSION "([^"]+)"', (root / 'main/php_version.h').read_text())[1]
    blocks, aliases, functions = {}, {}, {}
    for directory in ('Zend', 'ext', 'main', 'sapi'):
        for path in sorted((root / directory).rglob('*_arginfo.h')):
            if 'zend_test' in path.parts or 'tests' in path.parts:
                continue
            signatures(path.read_text(), blocks, aliases, functions)
    # SAPI function tables are hand-written in .c files, not emitted into the
    # generated arginfo headers; collect them so CLI-only functions are known.
    for path in sorted((root / 'sapi').rglob('*.c')):
        if 'tests' in path.parts:
            continue
        source = path.read_text()
        if 'zend_function_entry' not in source:
            continue
        signatures(source, blocks, aliases, functions)
    lines = [f'/* Generated from official PHP {version} *_arginfo.h files and SAPI function tables.',
             ' * Regenerate with src/validation/generate_php_functions.py; do not edit.',
             f' * Source: https://www.php.net/distributions/php-{version}.tar.xz',
             ' * Includes conditional bundled extensions; does not imply runtime availability. */',
             '#pragma once', '#include "php_common.h"', '#include <stddef.h>', '']
    for index, (name, variants) in enumerate(sorted(functions.items())):
        for variant, (_, params) in enumerate(variants):
            lines.append(f'static const php_function_parameter_info fn_params_{index}_{variant}[] = {{')
            for parameter, required, variadic, kind in params:
                lines.append(f'    {{{json.dumps(parameter)}, {str(required).lower()}, {str(variadic).lower()}, PHP_ATTR_TYPE_{kind}}},')
            lines.extend(['    {NULL, false, false, PHP_ATTR_TYPE_ANY}', '};'])
        for variant in reversed(range(1, len(variants))):
            following = f'&fn_variant_{index}_{variant + 1}' if variant + 1 < len(variants) else 'NULL'
            lines.append(f'static const php_function_info fn_variant_{index}_{variant} = '
                         f'{{{json.dumps(name)}, PHP_ATTR_TYPE_{variants[variant][0]}, fn_params_{index}_{variant}, {following}}};')
    lines.append('static const php_function_info php_functions[] = {')
    for index, (name, variants) in enumerate(sorted(functions.items())):
        following = f'&fn_variant_{index}_1' if len(variants) > 1 else 'NULL'
        lines.append(f'    {{{json.dumps(name)}, PHP_ATTR_TYPE_{variants[0][0]}, fn_params_{index}_0, {following}}},')
    lines.extend(['};', 'static const int php_function_count = sizeof(php_functions) / sizeof(php_functions[0]);', ''])
    return '\n'.join(lines)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('php_source', type=Path)
    parser.add_argument('--output', type=Path, default=Path(__file__).with_name('php_functions.h'))
    args = parser.parse_args()
    args.output.write_text(generate(args.php_source))
