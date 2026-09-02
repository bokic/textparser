#!/usr/bin/env bash
# Build the tree-sitter-typescript differential tooling in-place.
#
# Requires an already built textparser in <repo>/bin/ (run ./build.sh at the
# repo root first). The tree-sitter runtime headers/static library come from
# the repo build directory (libtree-sitter.a built for the benchmark suite).
# The tree-sitter-typescript grammar is cloned on first use into
# <repo>/build/_deps/tree-sitter-typescript and kept for reuse.
set -eu

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
BIN="$ROOT/bin"
INC_RUNTIME="$ROOT/build/_deps/treesitter-src/lib/include"
GR="$ROOT/build/_deps/tree-sitter-typescript"

if [ ! -f "$BIN/libtree-sitter.a" ]; then
    echo "error: $BIN/libtree-sitter.a not found - build the textparser project first (./build.sh)" >&2
    exit 1
fi
if [ ! -f "$INC_RUNTIME/tree_sitter/api.h" ]; then
    echo "error: tree-sitter runtime headers not found under $ROOT/build/_deps" >&2
    echo "       rebuild the project so the benchmark dependency is fetched." >&2
    exit 1
fi

if [ ! -d "$GR" ]; then
    echo "fetching tree-sitter-typescript grammar into $GR ..."
    git clone --depth 1 https://github.com/tree-sitter/tree-sitter-typescript "$GR"
fi

for d in typescript tsx; do
    [ -f "$GR/$d/src/parser.c" ] || { echo "error: missing $GR/$d/src/parser.c" >&2; exit 1; }
    [ -f "$GR/$d/src/scanner.c" ] || { echo "error: missing $GR/$d/src/scanner.c" >&2; exit 1; }
done

CC="${CC:-cc}"

echo "building dump ..."
$CC -std=c11 -O1 -I"$INC_RUNTIME" \
    -I"$GR/typescript/src" \
    "$HERE/dump.c" \
    "$GR/typescript/src/parser.c" \
    "$GR/typescript/src/scanner.c" \
    "$GR/tsx/src/parser.c" \
    "$GR/tsx/src/scanner.c" \
    "$BIN/libtree-sitter.a" -lm -o "$HERE/dump"

echo "building cstdump ..."
c++ -std=c++17 -I"$ROOT/include" "$HERE/cstdump.cpp" \
    -L"$BIN" -ltextparser -ltextparser-json \
    -Wl,-rpath,"$BIN" -o "$HERE/cstdump"

echo "done."
