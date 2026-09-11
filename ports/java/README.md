# textparser — Java port

Pure-Java port of the `textparser` C engine. It reproduces the C reference
behavior (see [`ARCHITECTURE.md`](../../ARCHITECTURE.md) §9) and is validated
against the C binary with a differential test harness.

## Implementation status

**The Java port implements the V1 (legacy regex) parser only.**

V1 is the `formatVersion: 1` engine driven by a definition's top-level
`tokens` / `startTokens` map. Implemented and verified against C:

- The seven token types: `Group`, `GroupOneChildOnly`,
  `GroupAllChildrenInSameOrder`, `Sequence`, `SimpleToken`, `StartStop`,
  `StartOptStop`.
- Anchored, line-bounded `findToken` and single-character-advance container
  loops (linear-time parsing).
- Speculative nested attempts with rollback and candidate fallback.
- Delimiter synthesis, `Unprocessed` leaves, hidden `Whitespace` leaves, and
  the delimiter pruning rule.
- `mergeSignIntoNumber` (literal `+`/`-` only), `deleteIfOnlyOneChild`,
  multi-line validation, and post-processing.
- `caseSensitivity`-aware regex flags and the PCRE2/`java.util.regex` common
  subset (including `\p{ID_Start}` / `\p{ID_Continue}` normalization).
- Parse errors as a message plus `position` / `length` span
  ([`ParseError`](src/com/textparser/ParseError.java)), matching
  `textparser_parse_error*`.
- **CLI-compatible `formatVersion: 2` normalization**: a v2 definition's
  `lexer.tokens` + `lexer.trivia` are flattened into the legacy token map as
  `SimpleToken`s with generated `startTokens`, exactly as the C CLI loader does.
  This is what makes the Java CLI match the C CLI on v2 (e.g. TypeScript) files.

What is still missing (V2 grammar engine, incremental parsing, native
handlers) is tracked in [`TODO.md`](TODO.md).

## Build & test

```bash
cd ports/java
./build.sh
```

The script uses Maven when available, otherwise `javac` + the bundled Gson jar.
It compiles the sources, runs `com.textparser.TextParserTest`, and packages a
JAR.

## CLI usage

```bash
# Parse a file to JSON
java -cp target/classes:lib/gson-2.11.0.jar \
     com.textparser.cli.Parse <definition.json> <file>

# Print a syntax-highlight byte map
java -cp ... com.textparser.cli.Parse <definition.json> <file> --format

# Parse stdin (format map)
java -cp ... com.textparser.cli.Parse <definition.json> --stdinformat
```

Other entry points: `cli.ParseDir`, `cli.Validate`, `cli.ValidateAll`,
`cli.CompareAllLanguages` (differential check against the C binary).

## Layout

```
src/com/textparser/
  TextParser.java   parser (V1 engine)
  Definition.java   definition model
  TokenItem.java    CST node + JSON serializer
  ParseError.java   parse failure with position/length
  cli/              command-line tools
tests/              TextParserTest (plain main-based test runner)
```
