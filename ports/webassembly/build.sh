#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=== Building TextParser WebAssembly Module ==="

# Check emcc availability
command -v emcc >/dev/null 2>&1 || { echo >&2 "emcc is required but not installed or not in PATH. Aborting."; exit 1; }

# Ensure definitions are generated
(cd "$ROOT_DIR/definitions" && ./regenerate.sh)

TMP_DIR="$ROOT_DIR/tmp"
mkdir -p "$TMP_DIR"

PCRE2_BUILD_DIR="$TMP_DIR/pcre2-build"
PCRE2_SRC_DIR="$TMP_DIR/pcre2-10.42"

if [ ! -f "$PCRE2_BUILD_DIR/libpcre2-8.a" ]; then
    echo "Downloading and building PCRE2 for WebAssembly..."
    mkdir -p "$TMP_DIR"
    if [ ! -d "$PCRE2_SRC_DIR" ]; then
        (cd "$TMP_DIR" && curl -LO https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.42/pcre2-10.42.tar.gz && tar -xzf pcre2-10.42.tar.gz)
    fi
    mkdir -p "$PCRE2_BUILD_DIR"
    (cd "$PCRE2_BUILD_DIR" && emcmake cmake "$PCRE2_SRC_DIR" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release -DPCRE2_BUILD_PCRE2_8=ON -DPCRE2_BUILD_PCRE2_16=ON -DPCRE2_BUILD_PCRE2_32=ON -DPCRE2_BUILD_PCRE2GREP=OFF -DPCRE2_BUILD_TESTS=OFF && emmake make -j$(nproc))
fi

JSONC_BUILD_DIR="$TMP_DIR/json-c-build"
JSONC_SRC_DIR="$TMP_DIR/json-c-json-c-0.17-20230812"

if [ ! -f "$JSONC_BUILD_DIR/libjson-c.a" ]; then
    echo "Downloading and building json-c for WebAssembly..."
    mkdir -p "$TMP_DIR"
    if [ ! -d "$JSONC_SRC_DIR" ]; then
        (cd "$TMP_DIR" && curl -LO https://github.com/json-c/json-c/archive/refs/tags/json-c-0.17-20230812.tar.gz && tar -xzf json-c-0.17-20230812.tar.gz)
    fi
    mkdir -p "$JSONC_BUILD_DIR"
    (cd "$JSONC_BUILD_DIR" && emcmake cmake "$JSONC_SRC_DIR" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF && emmake make json-c -j$(nproc))
fi

echo "Compiling WebAssembly module..."
emcc -std=c23 \
    -D_POSIX_C_SOURCE=200809L \
    -D_GNU_SOURCE \
    -O3 \
    -s WASM=1 \
    -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall","UTF8ToString","stringToUTF8","lengthBytesUTF8","getValue","setValue"]' \
    -s EXPORTED_FUNCTIONS='["_malloc","_free"]' \
    -s EXPORT_NAME="TextParserModule" \
    -s MODULARIZE=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -I"$ROOT_DIR/include" \
    -I"$ROOT_DIR/os" \
    -I"$PCRE2_BUILD_DIR" \
    -I"$PCRE2_SRC_DIR/src" \
    -I"$JSONC_BUILD_DIR" \
    -I"$JSONC_SRC_DIR" \
    "$ROOT_DIR/src/textparser.c" \
    "$ROOT_DIR/src/adv_regex.c" \
    "$ROOT_DIR/os/os_posix.c" \
    "$ROOT_DIR/src/textparser-json.c" \
    "$SCRIPT_DIR/textparser_wasm.c" \
    "$PCRE2_BUILD_DIR/libpcre2-8.a" \
    "$PCRE2_BUILD_DIR/libpcre2-16.a" \
    "$PCRE2_BUILD_DIR/libpcre2-32.a" \
    "$JSONC_BUILD_DIR/libjson-c.a" \
    -o "$SCRIPT_DIR/textparser.js"

echo "WebAssembly build complete: $SCRIPT_DIR/textparser.js, $SCRIPT_DIR/textparser.wasm"
