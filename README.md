# TextParser [![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/bokic/textparser)

TextParser is a high-performance, extensible text parsing library written in C. It uses regular expressions to define language grammars and generates a hierarchical Abstract Syntax Tree (AST) for parsed documents.

The `textparser --version` and `ccat --version` commands show one version when the executable and linked library match. When they differ, the output identifies the executable and library versions separately.

The project currently provides support for: Ada, ASM, Bash, C, C++, C3, CFML, C#, CSS, Fortran, Go, HTML, Jai, Java, JavaScript, JSON, Markdown (MD), MATLAB, Pascal, Perl, PHP, Python, R, Rust, Scratch, SQL, Swift, TypeScript, VB, Zig.
It has a flexible architecture making it easy to add new languages.

## Features

- **High Performance**: Written in optimized C for fast parsing(upto 100MB/s) of large codebases.
- **Small Footprint**: The library is designed to be small(<100KB for both parser and language definition) and easy to integrate into other projects.
- **Zero Dependencies for Built-in Languages**: All 30 built-in language grammars use pure C matchers with no external libraries. PCRE2 is only loaded at runtime when custom JSON grammar definitions require it.
- **Hierarchical AST/CST**: Generates a structured tree of tokens (`textparser_token_item`) representing the code structure.
- **Syntax Highlighting Support**: Tokens carry styling metadata (text color, background, font style) based on a modern dark theme, making it ideal for CLI syntax viewers, LSP servers, and code editors.
- **Extensibility**: Language definitions are decoupled from the core parsing logic, constructed with JSON, and can be loaded at compile time (by generated header file) or at runtime (by loading JSON file).
- **BOM Specification**: Each grammar can specify which Byte Order Marks it accepts (e.g., UTF-8, UTF-16).
- **Native Query Engine**: Query AST nodes using CSS-like selectors (`"Parent > Child"`, `"Ancestor Descendant"`, `"TypeA, TypeB"`).
- **Incremental Delta Parsing**: Efficiently re-parses modified sections of a buffer, reusing unchanged parts of the tree and reporting which areas need repainting.
- **Modern C23 & C++23 Standard**: Written for C23 and C++23, using features like `nullptr` and clean struct initialization across GCC, Clang, and MSVC.
- **Native C Fast Path**: All 30 built-in grammars use hand-written C matchers instead of regex, eliminating 650 regexes for maximum parsing speed. Custom JSON definitions can also plug in native C matchers.

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

## Installation

### Arch Linux

`textparser` is available on the Arch User Repository (AUR):

```bash
yay -S textparser
```

Or view the package details at [https://aur.archlinux.org/packages/textparser](https://aur.archlinux.org/packages/textparser).

### Fedora

`textparser` can be installed via Fedora Copr:

```bash
sudo dnf copr enable bokic/textparser
sudo dnf install textparser ccat libtextparser-devel
```

### macOS (Homebrew)

```bash
brew tap bokic/textparser
brew install textparser
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
        // Access 64-bit stable ID and flags
        uint64_t node_id = textparser_node_get_id(item);
        uint32_t flags = textparser_node_get_flags(item);
        // ... process item ...
    }
    
    return 0;
}
```

**Semantic Handler Lifecycle Example:**

```c
#include <textparser.h>

static textparser_action my_commit_handler(textparser_t parser, const textparser_event *event, void *user_data) {
    if (event->type == TEXTPARSER_EVENT_COMMIT && event->node) {
        // Attach application AST node or metadata
        textparser_node_set_user_data(event->node, my_ast_node_ptr, free_my_ast_node);
    }
    return TEXTPARSER_ACTION_ACCEPT;
}

// Register handler
textparser_register_handler(handle, "my_lang.onCommit", my_commit_handler, NULL);
```

**Lexer Modes, Goals & Registered Decoders:**

```c
#include <textparser.h>

// Transient mode transitions
textparser_push_mode(handle, "TemplateText");
const char *mode = textparser_get_current_mode(handle); // "TemplateText"
textparser_pop_mode(handle);

// Contextual lexical goals
textparser_set_lexical_goal(handle, "ExpressionStart");

// Custom decoders & validators
textparser_register_decoder(handle, "ecmascript.identifier", my_id_decoder, NULL);
textparser_register_validator(handle, "ecmascript.numericLiteral", my_num_validator, NULL);
```

**Scoped Contexts, Predicates & Speculative Parsing:**

```c
#include <textparser.h>

// Scoped Context flags (AllowAwait, InType, etc.)
textparser_context_set(handle, "AllowAwait", 1);
bool allow_await = textparser_context_is(handle, "AllowAwait");

// Semantic predicates
textparser_register_predicate(handle, "ts.isTypeArg", my_predicate_fn, NULL);
bool is_type = textparser_eval_predicate(handle, "ts.isTypeArg");

// Speculative parsing & branch rollback
void *checkpoint = NULL;
textparser_speculate_begin(handle, &checkpoint);

// If speculative branch fails:
textparser_speculate_rollback(handle, checkpoint);
// Or if speculative branch succeeds:
// textparser_speculate_commit(handle, checkpoint);
```

**Operator Precedence & Pratt Expression Engine:**

```c
#include <textparser.h>

// Register explicit operator definitions (prefix, infix, postfix, ternary)
textparser_operator_def add_op = {
    .token_id = 100,
    .role = TEXTPARSER_OP_INFIX,
    .precedence = 10,
    .associativity = TEXTPARSER_ASSOC_LEFT,
    .secondary_token_id = 0
};
textparser_register_operator(handle, &add_op);

// Parse expression with minimum precedence climbing
textparser_node *expr_node = NULL;
textparser_parse_pratt_expression(handle, 0, &expr_node);
```

**Multi-Diagnostic Reporting & Error Recovery:**

```c
#include <textparser.h>

// Report diagnostics with exact source spans and codes
textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR, "TS1005", "';' expected.", 12, 10);

size_t diag_count = textparser_get_diagnostic_count(handle);
for (size_t i = 0; i < diag_count; i++) {
    textparser_diagnostic diag;
    if (textparser_get_diagnostic(handle, i, &diag) == 0) {
        printf("[%s] Line %u: %s\n", diag.code, diag.line + 1, diag.message);
    }
}
textparser_clear_diagnostics(handle);
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

## Language Definition Schema & Migration

TextParser uses a JSON-based declarative schema (v1 legacy, v2 unified) to define language grammars, separating lexical tokens from grammar productions and error recovery policies.

- **Unified Schema Specification**: See [`schema/textparser-schema.json`](file:///home/boris/projects/textparser/schema/textparser-schema.json) for the full JSON Schema Draft 2020-12 definition.
- **Schema v2 fields**: `lexer` (`tokens`, `modes`, `goals`, `trivia`), `grammar` (`start`, `productions`), `recovery`, `features`, `operators`, `directives`.

### Non-Destructive Batch Migration (schema_v2/)

Migrate all legacy definitions to the unified v2 schema **without overwriting** the working build definitions:

```bash
# Outputs to definitions/schema_v2/ (default, non-destructive)
python3 scripts/migrate_definitions.py --all definitions/

# Custom output directory
python3 scripts/migrate_definitions.py --all definitions/ --output-dir /tmp/v2out/

# In-place migration (modifies originals — use with version control)
python3 scripts/migrate_definitions.py --all definitions/ --in-place

# Migrate a single file
python3 scripts/migrate_definitions.py definitions/cfml_definition.json /tmp/cfml_v2.json
```

> **Note**: The build system and `json2h.py` continue to use the legacy v1 format in `definitions/*.json`. The `schema_v2/` directory is for reference, tooling development, and future loader implementation.

### json2h.py Schema v2 Compatibility

[`definitions/json2h.py`](file:///home/boris/projects/textparser/definitions/json2h.py) supports both v1 and v2 schema formats:
- Tokens are resolved from `root["tokens"]` (v1) or `root["lexer"]["tokens"]` (v2).
- `startTokens` are resolved from `root["startTokens"]` (v1) or `root["grammar"]["start"]` (v2).
- All disambiguation tables (`mergeSignIntoNumber`, `regexVsDivision`, etc.) remain v1-format for now.

## Generating Definition Headers

To use a JSON language definition in C code at compile time, convert it into a C header file using [json2h.py](file:///home/boris/projects/textparser/definitions/json2h.py).

```bash
python3 definitions/json2h.py definitions/your_definition.json
```

Pass `--no-native-regex` to skip native matchers (useful for testing with regex strings only):

```bash
python3 definitions/json2h.py --no-native-regex definitions/your_definition.json
```

### Regenerating All Headers

```bash
cd definitions
./regenerate.sh
```

## Test Suite Coverage

The unit test suite (`bin/unittests`) covers 287+ tests across all language parsers and compiler-grade architecture features:

| Test File | Coverage Area |
|---|---|
| `schema_parity_tests.cpp` | JSON schema v2 structure, BOM parsing, definition parity |
| `arena_lifecycle_tests.cpp` | Arena checkpointing, node flags (`SYNTHETIC`, `MISSING`, `RECOVERED`), user_data destructors, lifecycle events |
| `lexer_modes_tests.cpp` | Transient mode stacks, contextual lexical goals, line-terminator predicates, decoders/validators |
| `speculation_tests.cpp` | Scoped contexts, semantic predicates, speculative parsing commit/rollback |
| `pratt_precedence_tests.cpp` | Explicit operator tables (prefix, infix, postfix, ternary), Pratt expression parsing |
| `diagnostic_recovery_tests.cpp` | Multi-diagnostic vector, error codes, source location (line/column), token sync recovery |
| `conformance_tests.cpp` | **Phase 7.2**: Nested 3-level speculation, mode rollback inside speculation, mode-stack boundary/overflow, lexical goal persistence, 5-severity multi-error loops, null-handle safety across all Phase 2–6 APIs, 100× open/close memory safety, 1000-diagnostic realloc stress, json2h migration parity |

## License

See `LICENSE` file for details.
