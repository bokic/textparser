#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

OUT_DIR="$SCRIPT_DIR/bin"
mkdir -p "$OUT_DIR"

echo "Compiling Java sources..."
javac -d "$OUT_DIR" \
    "$SCRIPT_DIR/src/com/textparser/TokenItem.java" \
    "$SCRIPT_DIR/src/com/textparser/JsonUtils.java" \
    "$SCRIPT_DIR/src/com/textparser/Definition.java" \
    "$SCRIPT_DIR/src/com/textparser/TextParser.java" \
    "$SCRIPT_DIR/src/com/textparser/cli/Parse.java" \
    "$SCRIPT_DIR/src/com/textparser/cli/ParseDir.java" \
    "$SCRIPT_DIR/src/com/textparser/cli/Validate.java" \
    "$SCRIPT_DIR/src/com/textparser/cli/ValidateAll.java" \
    "$SCRIPT_DIR/src/com/textparser/cli/CompareAllLanguages.java" \
    "$SCRIPT_DIR/tests/TextParserTest.java"

echo "Running Java Unit Tests..."
java -cp "$OUT_DIR" com.textparser.TextParserTest

echo "Java build and tests completed successfully."
