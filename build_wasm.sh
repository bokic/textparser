#!/bin/bash
set -e

# Check if emcmake is available
if ! command -v emcmake &> /dev/null; then
    # Try adding common installation path
    export PATH=$PATH:/usr/lib/emscripten
    if ! command -v emcmake &> /dev/null; then
        echo "Error: emcmake could not be found. Please install Emscripten SDK and activate it."
        exit 1
    fi
fi

BUILD_DIR="build-wasm"

mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure with CMake
# We disable CLI and Tests because we only want the libraries for now (or maybe a simple test harness).
echo "Configuring for WebAssembly..."
emcmake cmake .. \
    -DTEXTPARSER_VERSION_TAG="0.0.1" \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DBUILD_CLI=OFF \
    -DBUILD_CCAT=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_TESTING=OFF
    
# Build
echo "Building..."
emmake make -j$(nproc)

echo "Generating bindings..."
# Link everything into a single JS/Wasm module. Run from build-wasm dir.
# Libraries are in ../bin because of top-level CMake logic or install?
# Wait, list_dir of bin showed them.
emcc ../bin/libtextparser.a ../bin/libtextparser-json.a ../bin/libpcre2-8.a ../bin/libpcre2-16.a ../bin/libpcre2-32.a ../bin/libjson-c.a \
    -I ../include -I ../os -I _deps/json-c-src -I _deps/json-c-build \
    ../src/wasm_bind.c \
    -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "stringToUTF8", "lengthBytesUTF8"]' \
    -s EXPORTED_FUNCTIONS='["_wasm_load_language_from_json", "_wasm_load_language_from_json2", "_wasm_parse_text", "_wasm_get_token_count", "_malloc", "_free"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s NO_DISABLE_EXCEPTION_CATCHING \
    -o ../bin/textparser.js

echo "Build complete. Output in bin/textparser.js and bin/textparser.wasm"
