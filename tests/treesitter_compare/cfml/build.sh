#!/usr/bin/env bash
# Build the tree-sitter-cfml differential tooling in-place.
#
# The tree-sitter runtime headers/static library come from the system
# (pkg-config tree-sitter). The tree-sitter-cfml grammar (cfml/cfscript/
# cfquery sub-grammars from the cfmleditor monorepo) is expected at the
# pinned revision under build/_deps/tree-sitter-cfml (see ROOT/CMakeLists.txt
# or clone manually), kept for reuse.
set -eu

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
GR="$ROOT/build/_deps/tree-sitter-cfml"

if ! pkg-config --exists tree-sitter; then
    echo "error: system tree-sitter not found (pkg-config tree-sitter)" >&2
    exit 1
fi

if [ ! -f "$GR/cfml/src/parser.c" ]; then
    echo "error: tree-sitter-cfml grammar not found under $ROOT/build/_deps" >&2
    echo "       clone cfmleditor/tree-sitter-cfml at the pinned rev into" >&2
    echo "       $ROOT/build/_deps/tree-sitter-cfml" >&2
    exit 1
fi

CC="${CC:-cc}"

echo "building dump ..."
$CC -std=c11 -O1 \
    -I"$GR/cfml/src" -I"$GR/cfscript/src" -I"$GR/cfquery/src" -I"$GR/common" \
    "$HERE/dump.c" \
    "$GR/cfml/src/parser.c" "$GR/cfml/src/scanner.c" "$GR/cfml/src/tag.c" \
    "$GR/cfscript/src/parser.c" "$GR/cfscript/src/scanner.c" \
    "$GR/cfquery/src/parser.c" "$GR/cfquery/src/scanner.c" \
    $(pkg-config --cflags --libs tree-sitter) -o "$HERE/dump"

echo "building cstdump ..."
c++ -std=c++17 -I"$ROOT/include" "$HERE/cstdump.cpp" \
    -L"$ROOT/bin" -ltextparser -ltextparser-json \
    -Wl,-rpath,"$ROOT/bin" -o "$HERE/cstdump"

echo "done."