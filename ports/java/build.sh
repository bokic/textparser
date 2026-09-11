#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

OUT_DIR="$SCRIPT_DIR/bin"
mkdir -p "$OUT_DIR"

GIT_VER=$(git -C "$ROOT_DIR" describe --tags --dirty 2>/dev/null || git -C "$ROOT_DIR" describe --tags --abbrev=0 2>/dev/null || echo "1.1.0")

if command -v mvn >/dev/null 2>&1; then
    echo "Building and testing with Maven (version: $GIT_VER)..."
    mvn -f "$SCRIPT_DIR/pom.xml" -Drevision="$GIT_VER" clean package
    echo "Running Java Unit Tests via Maven output..."
    java -cp "$SCRIPT_DIR/target/classes:$SCRIPT_DIR/target/test-classes" com.textparser.TextParserTest
    
    JAR_FILE="$SCRIPT_DIR/target/textparser-${GIT_VER}.jar"
    echo "Java Maven build and tests completed successfully."
    echo "Jar output: $JAR_FILE"
else
    LIB_JAR="$SCRIPT_DIR/lib/gson-2.11.0.jar"
    if [ ! -f "$LIB_JAR" ]; then
        mkdir -p "$SCRIPT_DIR/lib"
        echo "Downloading Gson library..."
        curl -sSL -o "$LIB_JAR" https://repo1.maven.org/maven2/com/google/code/gson/gson/2.11.0/gson-2.11.0.jar
    fi

    echo "Compiling Java sources..."
    javac -cp "$LIB_JAR" -d "$OUT_DIR" \
        "$SCRIPT_DIR/src/com/textparser/TokenItem.java" \
        "$SCRIPT_DIR/src/com/textparser/Definition.java" \
        "$SCRIPT_DIR/src/com/textparser/TextParser.java" \
        "$SCRIPT_DIR/src/com/textparser/cli/Parse.java" \
        "$SCRIPT_DIR/src/com/textparser/cli/ParseDir.java" \
        "$SCRIPT_DIR/src/com/textparser/cli/Validate.java" \
        "$SCRIPT_DIR/src/com/textparser/cli/ValidateAll.java" \
        "$SCRIPT_DIR/src/com/textparser/cli/CompareAllLanguages.java" \
        "$SCRIPT_DIR/tests/TextParserTest.java"

    echo "Running Java Unit Tests..."
    java -cp "$OUT_DIR:$LIB_JAR" com.textparser.TextParserTest

    JAR_FILE="$OUT_DIR/textparser-${GIT_VER}.jar"
    echo "Packaging JAR file..."
    jar --create --file "$JAR_FILE" -C "$OUT_DIR" com

    echo "Java build and tests completed successfully."
    echo "Jar output: $JAR_FILE"
fi
