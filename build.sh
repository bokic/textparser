#!/usr/bin/env bash
set -e

BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_TESTS="${BUILD_TESTS:-ON}"
GENERATOR="${GENERATOR:-Ninja}"

command -v cmake >/dev/null 2>&1 || { echo >&2 "cmake is required but not installed. Aborting."; exit 1; }

if [ "$GENERATOR" = "Ninja" ]; then
    command -v ninja >/dev/null 2>&1 || { echo >&2 "ninja is required for Ninja generator but not installed. Falling back to Unix Makefiles."; GENERATOR="Unix Makefiles"; }
fi

cmake -B build -G "$GENERATOR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DBUILD_TESTS="$BUILD_TESTS"
cmake --build build

if [ -f build/compile_commands.json ]; then
    cp build/compile_commands.json .
fi
