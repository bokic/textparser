#!/usr/bin/env sh
# Regenerate the TypeScript conformance golden files from the C engine.
#
# The C engine is the golden standard for the differential fixture corpus.
# Run this after intentionally changing the C grammar/legality behavior so the
# committed golden files under typescript/golden reflect the new engine output.
#
# Usage:  tests/docker/fixtures/regenerate_golden.sh
set -eu

cd "$(dirname "$0")/../../.."  # repo root
./build.sh
TEXTPARSER_REGENERATE_GOLDEN=1 ./bin/unittests \
    --gtest_filter='TypeScriptFixtureConformance.*'
