# TextParser [![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/bokic/textparser)

TextParser is a high-performance, extensible text parsing library written in C. It uses regular expressions to define language grammars and generates a hierarchical Abstract Syntax Tree (AST) for parsed documents.

The project currently provides support for: Ada, ASM, Bash, C, C++, C3, CFML, C#, CSS, Fortran, Go, HTML, Jai, Java, JavaScript, JSON, Markdown (MD), MATLAB, Pascal, Perl, PHP, Python, R, Rust, Scratch, SQL, Swift, TypeScript, VB, Zig.
It has a flexible architecture making it easy to add new languages.

## Features

- **High Performance**: Written in optimized C for fast parsing(upto 100MB/s) of large codebases.
- **Small Footprint**: The library is designed to be small(<100KB for both parser and language definition) and easy to integrate into other projects.
- **Zero Dependencies for Built-in Languages**: The library has zero external library dependencies (`libtextparser.so` links strictly against standard `libc`). All 30 built-in language grammars are lexed via 100% native C matchers. External PCRE2 libraries are only loaded dynamically at runtime on demand via `os_dlopen` if custom un-bypassed JSON grammar definitions are loaded.
- **Concrete Syntax Tree (CST) & Gapless Trivia Retention**: Preserves 100% byte-for-byte source fidelity including delimiters, punctuation, whitespace, and unparsed text as explicit `UNPROCESSED` token nodes (`TEXTPARSER_TOKEN_ID_UNPROCESSED` = -2) such that $\sum \text{token.len} == \text{document\_length}$.
- **Relative Node Length & Dynamic Offsets**: Tokens store only relative node length (`len`); absolute document offsets are computed dynamically on demand via `textparser_get_token_position(token)`. This eliminates $O(N)$ position-invalidation cascades after edit locations during incremental parsing.
- **Hierarchical AST/CST**: Generates a structured tree of tokens (`textparser_token_item`) representing the code structure.
- **Syntax Highlighting Support**: Tokens track rich styling metadata (24-bit RGB text color, background, and font styling flags) based on a modern, high-contrast dark theme palette (distinct colors for keywords, identifiers, types/casts, comments, strings, numbers, booleans, operators, and preprocessors), making it ideal for CLI syntax viewers (`ccat`), LSP servers, and code editors.
- **Extensibility**: Language definitions are decoupled from the core parsing logic, constructed with JSON, and can be loaded at compile time (by generated header file) or at runtime (by loading JSON file).
- **Conditional Start Tokens (`overrideStartTokens`)**: Dynamic start token override rules based on file extension and regex pattern matching at document start (used e.g. for modern ColdFusion script components).
- **Context-Sensitive Token Replacement (`contextNestedTokens`)**: Tokens can dynamically specify context-sensitive child token lists based on enclosing parent token types in the parsing stack.
- **Non-Fatal Error Resynchronization**: Recovers gracefully from malformed syntax without aborting parsing, grouping contiguous invalid input into merged `AST_NODE_ERROR` nodes (`TEXTPARSER_TOKEN_ID_ERROR`).
- **BOM Specification (`SupportedBom`)**: Grammar-level specification of allowed Byte Order Marks (e.g., UTF-8, UTF-16-LE, UTF-16-BE).
- **Native Query Engine (`textparser_query`)**: High-performance C selector engine to query AST nodes using intuitive CSS-like selector syntax (`"Parent > Child"`, `"Ancestor Descendant"`, `"TypeA, TypeB"`).
- **Sign Merging (`mergeSignIntoNumber`)**: Per-definition rule (enabled for all arithmetic languages, e.g. C, Java, JavaScript, Python, CFML, ...) that absorbs a leading `+`/`-` sign into the following number token (e.g. `x = -1` → `Number("-1")`) while leaving true binary subtraction untouched (`10-10` → `Number(10) Operator(-) Number(10)`). The merge is decided in the parse pass by the preceding context (unary only when the sign is *not* preceded by an operand), requires sign/number adjacency, only applies to literal `+`/`-` (never e.g. `!3`), and also handles a sign that is the last child of an operator group (`12 +-43` → `Number(12) Operator(+) Number(-43)`). Configured via `signTokens`, `numberTokens`, and `operandTokens` in the JSON definition.
- **Thread-Safe Regex Engine (`adv_regex.c`)**: PCRE2 compile contexts (`pcre2_compile_context_8/16/32`) are bound to the `textparser_t` handle via `adv_regex_context` instead of global state. The three-width PCRE2 API surface (8/16/32 bit) is abstracted behind a `pcre2_api_t` vtable; a single `adv_regex_find_pattern_impl()` function handles all widths without code duplication.
- **Standalone AST Post-Processing & Pratt Parsing (`textparser_post_process`)**: Opt-in 2nd-pass AST transformation that applies grammar-driven **Pratt Parsing (Top-Down Operator Precedence)** to pivot flat expression token sequences into hierarchical binary/unary expression trees with configurable binding power and associativity (`left`/`right`), and performs `deleteIfOnlyOneChild` unwrapping for static analysis tools without breaking token pointer snapshot stability for interactive incremental text editor sessions (`textparser_parse_incremental`).
- **Contextual Rule Disambiguation (`regexVsDivision`, `templateDisambiguation`, `castDisambiguation`)**: Resolves syntactic and lexical ambiguities across languages through lookbehind and AST restructuring passes:
  - **JavaScript / TypeScript Regex vs. Division**: Disambiguates `/pattern/flags` vs. arithmetic division (`/`) by verifying that preceding non-trivia tokens are non-operands (or control-flow conditions), correctly parsing `a / b / c` as division and `return /pattern/i;` or `if (x) /abc/` as regex literals.
  - **C++ Generics / Templates vs. Relational Operators**: Validates template argument brackets `<...>` vs. relational `<` and `>` comparisons, bundling matching template parameter subtrees into `TemplateGroup` nodes.
  - **C / C++ Type Cast vs. Call Expression**: Differentiates cast expressions `(type)(expr)` from function invocations and grouped expressions `(func)(arg)` by inspecting inner parenthesized tokens against defined type keywords and pointer/reference qualifiers.
- **Structural Statement Recognition & Speculative Backtracking (`Sequence`)**: Grammar-driven composite statement parsing that combines ordered sequence tokens with zero-heap arena checkpoint snapshots (`textparser_checkpoint_t`). Allows language grammars (C, Rust, Go, TypeScript, Zig, CFML, etc.) to define complex multi-token statements (declarations, type annotations, assignment statements) and attempt candidate branches speculatively, seamlessly rolling back upon mismatch without the overhead of massive GLR state tables.
- **Incremental Delta Parsing (`textparser_parse_incremental`)**: Efficiently re-parses modified buffers in interactive environments (like text editors and IDEs) via delta edit chunks (`edit_offset`, `old_len`, `new_text`, `new_len`), automatically splicing internal buffer memory, reusing unaffected CST branches, and reporting dirty repaint coordinates (`textparser_dirty_range`).
- **High-Speed Flat Token Range Export (`textparser_export_tokens`, `textparser_export_tokens_range`, `textparser_export_tokens_lines`)**: Allocation-free, high-throughput C & C++ API designed for editors (LSP servers, QScintilla, VS Code highlight buffers) to export flattened token ranges `[start_pos, length, start_line, start_col, end_line, end_col, token_id, ...]` in a single sequential pass into a caller-provided scratch buffer. Supports full documents, byte range queries, or line-bounded queries with $O(1)$ amortized line/column resolution.
- **Modern C23 & C++23 Standard**: Engineered natively for ISO C23 (`ISO/IEC 9899:2024`) and C++23 standards (`set(CMAKE_C_STANDARD 23)`, `set(CMAKE_CXX_STANDARD 23)`), utilizing native `nullptr` keywords and C23 clean struct initialization across GCC, Clang, and MSVC compilers.
- **API Documentation & Error Diagnostics**: Public headers (`textparser.h`, `textparser-json.h`) feature comprehensive Doxygen-style documentation, standardized error enumeration (`enum textparser_error`), and string conversion helpers (`textparser_strerror`, `textparser_json_strerror`) for rich diagnostics across CLI tools.
- **Native C Regex Bypass & Zero-Dependency Fast Path (`search_function_gen`)**: Token rules support direct native C matcher functions (`startRegexFunction` and `endRegexFunction` conforming to `textparser_fast_regex_fn`). Generated language definitions automatically integrate with `include/search_function_gen.h` and `src/search_function_gen.c` using the `_gen_{lang}_{token}_{start|end}` naming convention, bypassing regular expressions with $O(1)$ native multi-encoding dispatch across 3-way representations (8-bit Latin-1 / UTF-8, 16-bit UTF-16, and 32-bit UTF-32). Full 100% native coverage is implemented across **all 30 built-in grammars**: **`c`**, **`cpp`**, **`cfml`**, **`json`**, **`html`**, **`css`**, **`python`**, **`javascript`**, **`rust`**, **`typescript`**, **`java`**, **`csharp`**, **`php`**, **`go`**, **`sql`**, **`bash`**, **`c3`**, **`zig`**, **`swift`**, **`pascal`**, **`perl`**, **`fortran`**, **`ada`**, **`asm`**, **`matlab`**, **`r`**, **`jai`**, **`vb`**, **`scratch`**, and **`md`** (650 / 650 total regexes eliminated, **100.0% zero-regex native C lexing**).
- **Zero-Dependency Shared Library & Lazy PCRE2 Dynamic Loading**: `libtextparser.so` / `textparser.dll` has zero static dependencies on external regular expression libraries (`readelf -d libtextparser.so` reports strictly `libc.so.6`). PCRE2 is only dynamically loaded on demand via `os_dlopen` / `os_dlsym` if custom runtime JSON definitions with un-bypassed patterns are explicitly loaded by the user.
- **Python Tooling**: Includes Python scripts (`ports/python/`) for prototyping, validation against the reference C parser, generation of C header files (`definitions/json2h.py`), and other parser verification tools.
- **Rust Implementation & Tooling**: Native Rust implementation (`ports/rust/`) including library crate (`TextParser`), CLI binaries (`parse`, `parsedir`, `validate`), and unit test suite validated against the reference C output.
- **Java Implementation & Tooling**: Standalone Java implementation (`ports/java/`) including core parser (`TextParser`), CLI entrypoints (`Parse`, `ParseDir`, `Validate`, `ValidateAll`), zero-dependency JSON engine, and unit test suite validated against the reference C output.
- **WebAssembly Bindings**: Compiled with Emscripten into WebAssembly (`ports/webassembly/`) with JavaScript wrapper library (`TextParserWasm`) for client-side web application consumption.

## Project Structure

- **`src/`**: Core C library implementation (`textparser.c`, `textparser-json.c`, `adv_regex.c`, `adv_regex.h`, `logger.h`).
- **`include/`**: Public header files (`textparser.h`, `textparser-json.h`).
- **`cli/`**: Command-line tool for testing, debugging, and demonstrating the library.
- **`definitions/`**: Language definitions (e.g., CFML, JSON).
- **`ports/`**: Multi-language ports and bindings:
  - **`ports/python/`**: Python bindings, prototypes, and validation tools.
  - **`ports/rust/`**: Rust library crate, CLI tools (`parse`, `parsedir`, `validate`), and test suite matching the Python parser implementation.
  - **`ports/java/`**: Standalone Java implementation, CLI tools (`Parse`, `ParseDir`, `Validate`), build script (`build.sh`), and unit test suite.
  - **`ports/webassembly/`**: WebAssembly build setup (`textparser_wasm.c`, `build.sh`), JS wrapper API (`textparser_wrapper.js`), and Node/browser unit tests (`test_wasm.js`).
- **`tests/`**: Unit and integration tests, including `tests/compat/` for legacy parser validation.
- **`ccat/`**: Syntax highlighting CLI utility (color cat).

## Build Instructions

### Prerequisites

- CMake (version 3.15 or higher)
- Ninja build system
- A C/C++ compiler (GCC or Clang)
- PCRE2 library (`pcre2-8`, `pcre2-16`, `pcre2-32`) & JSON-C
  - Ubuntu/Debian: `sudo apt install libpcre2-dev libjson-c-dev`
  - Arch Linux: `sudo pacman -S pcre2 json-c`
  - macOS: `brew install pcre2 json-c pkg-config ninja`

### Building

You can use the provided build script for a quick start on Linux/macOS:

```bash
./build.sh
```

On Windows, use the batch scripts in `windows/`:

```cmd
cd windows
build_deps.bat
build.bat
```

Alternatively, build using standard CMake commands:

```bash
cmake -B build -G Ninja
cmake --build build
```

Artifacts (libraries and executables) will be output to the `bin/` directory.

## Testing

To run the full test suite after building:

```bash
ctest --test-dir build --output-on-failure
```

Or execute the unit test binary directly:

```bash
bin/unittests
```

On Windows:

```cmd
bin\unittests.exe
```

## Performance

Parsing performance is tracked on every push to `master` using the [Google Benchmark](https://github.com/google/benchmark) framework against the SQLite 3.53.0 source tree (312 `.c` + 42 `.h` files, ~13.7 MB).

📊 **[View benchmark charts](https://bokic.github.io/textparser/benchmarks/)**

### Search-scope optimization

Token matching distinguishes between three scopes:

- **Offset-0 only** — anchored (`PCRE2_ANCHORED`) start-token checks at the current position. For tokens flagged `multiLine: false`, the regex subject window is clamped to the end of the current line (`\n` or `\r`), since a single-line token can never span a newline. This is the dominant search type and the main performance lever for runtime-loaded definitions.
- **Same-line end search** — for `StartStop` tokens with `otherTextInside: false` *and* nested tokens, the closing-token search is anchored at the current position only (the parser loop guarantees the end token sits there), avoiding a full scan to end-of-file. For `StartStop` tokens with `otherTextInside: true` *and* `multiLine: false` (single-line strings), the closing-token search is bounded to the current line, falling back to a full scan only when the end token is absent there so the *"Token spans multiple lines but multi_line flag is not set!"* validation can still fire.
- **To-EOF end search** — kept for `otherTextInside: true` multi-line tokens (e.g. strings) that legitimately span newlines, and for `StartStop` tokens without nested tokens (e.g. `#...#` sharp expressions) whose end must be located by scanning ahead.

All bounds are computed in encoding-aware units, so they behave identically for LATIN1, UTF-8, UTF-16 and UTF-32.

### UTF-validity check elimination

The single largest cost for runtime-loaded definitions was PCRE2 re-validating the **entire subject** for valid UTF-8 on *every* `pcre2_match` call (`PCRE2_UTF` set without `PCRE2_NO_UTF_CHECK`) — roughly 8µs per call on a ~20 KB window. The parser now validates the whole document once per parse (in the appropriate encoding: UTF-8/16/32) and passes `PCRE2_NO_UTF_CHECK` to every subsequent match when the text is valid. If the text contains invalid UTF-8/16/32, the flag is not used and the previous per-call checked behavior is preserved exactly.

Combined with the search-scope work above, this took a runtime-loaded CFML parse of a 39 KB file from ~1360 ms to ~10 ms (over 100x), with identical parse results.

For debugging the search behavior, set the environment variable `TEXTPARSER_TRACE_SEARCH=1` to log every regex attempt (`start`/`end`, byte position, window size, scope, whether it matched, and per-token match time) to stderr:

```bash
TEXTPARSER_TRACE_SEARCH=1 bin/textparser file.cfm --mute
```

## Installation

### Arch Linux

`textparser` is available on the Arch User Repository (AUR):

```bash
yay -S textparser
```

Or view the package details at [https://aur.archlinux.org/packages/textparser](https://aur.archlinux.org/packages/textparser).

### macOS (Homebrew)

Install from the local formula repository:

```bash
brew install --build-from-source ./MacOS/textparser.rb
```

### Windows

Binary releases are available on the project [releases](https://github.com/bokic/textparser/releases) page.

### Docker

Ready-to-run images are published to Docker Hub:

```bash
docker pull bokic78/textparser:latest
```

The image is Alpine-based (musl), contains the `textparser` CLI (entry point) and the `ccat` syntax highlighting utility, and supports both `linux/amd64` and `linux/arm64`. Mount your files and run:

```bash
# Parse a file
docker run --rm -w /work -v "$PWD":/work:ro bokic78/textparser ./file.cfm

# Emit the token tree as JSON
docker run --rm -w /work -v "$PWD":/work:ro bokic78/textparser ./file.json --json

# Use ccat
docker run --rm -w /work -v "$PWD":/work:ro --entrypoint ccat bokic78/textparser ./file.c
```

To build the image locally:

```bash
docker build -t textparser .
```

## Usage

### CLI Tool

The `textparser` CLI tool parses files and visualizes the resulting token tree.

```bash
# Parse a file using automatically detected language rules
bin/textparser path/to/file.cfm

# Parse a file using a custom runtime JSON definition
bin/textparser path/to/file.json --definition definitions/json_definition.json
```

### C Library Integration

To use TextParser in your C project, include `textparser.h` and link against `libtextparser`. When compiling with C++, include `textparser.hpp` instead of `textparser.h`.

**Basic Example:**

```c
#include <textparser.h>
#include <stdio.h>

// Assume 'my_lang_definition' is defined elsewhere
extern const textparser_language_definition my_lang_definition;

int main() {
    textparser_defer(handle); // Auto-cleanup (defined when compiling with C compiler)

    // Open a file
    int err = textparser_openfile("example.txt", TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_BOM_ALL, &handle);
    if (err) {
        fprintf(stderr, "Failed to open file\n");
        return 1;
    }

    // Parse using the language definition
    err = textparser_parse(handle, &my_lang_definition);
    if (err) {
        fprintf(stderr, "Parse error\n");
        return 1;
    }

    // Iterate through tokens
    for (textparser_token_item *item = textparser_get_first_token(handle); item != NULL; item = item->next) {
        // ... process item ...
    }
    
    return 0;
}
```

**C++ RAII Wrapper Example:**

```cpp
#include <textparser.hpp>
#include <iostream>

extern const textparser_language_definition my_lang_definition;

int main() {
    textparser::Parser parser;
    if (parser.openfile("example.txt", TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_BOM_ALL) == 0) {
        if (parser.parse(&my_lang_definition) == 0) {
            for (textparser_token_item *item = parser.get_first_token(); item != nullptr; item = item->next) {
                // ... process item ...
            }
        }
    }
    return 0; // Automatically calls textparser_close on scope exit
}
```

## Language Definition Example

TextParser uses a JSON-based format to define language grammars. This allows defining complex syntax rules using regular expressions and hierarchical token structures.

Here is an example of what a JSON definition looks like (based on `definitions/json_definition.json`):

```json
{
  "name": "json",
  "version": 1.0,
  "startTokens": ["Object", "Array"],
  "tokens": {
    "Object": {
      "type": "StartStop",
      "startRegex": "{",
      "endRegex": "}",
      "textColor": "0xffd700",
      "nestedTokens": ["Key", "String", "Number", "ValueSeparator"]
    },
    "String": {
      "type": "StartStop",
      "startRegex": "\"",
      "endRegex": "\"",
      "textColor": "0xce9178",
      "nestedTokens": ["StringEscape"]
    },
    "Number": {
      "type": "SimpleToken",
      "startRegex": "\\d+(?:\\.\\d+)?",
      "textColor": "0xb5cea8"
    }
  }
}
```

## Generating Definition Headers

To use a JSON language definition in C code at compile time, convert it into a C header file using the Python utility [json2h.py](file:///home/boris/projects/textparser/definitions/json2h.py).

### Generating a Specific Header

Run [json2h.py](file:///home/boris/projects/textparser/definitions/json2h.py) located in the `definitions/` directory:

```bash
python3 definitions/json2h.py definitions/your_definition.json
```

This generates a C header file (e.g., `definitions/your_definition.json.h`) containing the C struct and tags enum.

By default, tokens whose `startRegex`/`endRegex` have a matching generated native C matcher in `include/search_function_gen.h` get a `.startRegexFunction` / `.endRegexFunction` function pointer (the "Native C Regex Bypass"). Pass `--no-native-regex` to skip those native function calls so the parser uses the regex strings directly instead:

```bash
python3 definitions/json2h.py --no-native-regex definitions/your_definition.json
```

With `--no-native-regex`, `.startRegexFunction` and `.endRegexFunction` are always set to `NULL` while `.start_regex`/`.end_regex` are preserved. This is useful for testing/validating the pure-regex code path against the native function bypass.

### Regenerating All Headers

Run the helper script [regenerate.sh](file:///home/boris/projects/textparser/definitions/regenerate.sh) from the `definitions/` directory:

```bash
cd definitions
./regenerate.sh
```

## License

See `LICENSE` file for details.
