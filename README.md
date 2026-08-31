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

## Language Definition Schema & Migration

TextParser uses a JSON-based declarative schema to define language grammars, separating lexical tokens from grammar productions and error recovery policies.

- **Unified Schema Specification**: See [`schema/textparser-schema.json`](file:///home/boris/projects/textparser/schema/textparser-schema.json) for the full JSON schema definition.
- **Migration Tool**: Migrate legacy definition files to the unified schema using [`scripts/migrate_definitions.py`](file:///home/boris/projects/textparser/scripts/migrate_definitions.py):
  ```bash
  python3 scripts/migrate_definitions.py definitions/your_definition.json
  # or batch migrate all definitions:
  python3 scripts/migrate_definitions.py --all definitions/
  ```

## Generating Definition Headers

To use a JSON language definition in C code at compile time, convert it into a C header file using the Python utility [json2h.py](file:///home/boris/projects/textparser/definitions/json2h.py).

### Generating a Specific Header

Run [json2h.py](file:///home/boris/projects/textparser/definitions/json2h.py) located in the `definitions/` directory:

```bash
python3 definitions/json2h.py definitions/your_definition.json
```

This generates a C header file (e.g., `definitions/your_definition.json.h`) containing the C struct and tags enum.

By default, tokens with a matching native C matcher get a fast-path function pointer. Pass `--no-native-regex` to skip the native matchers so the parser uses regex strings directly (useful for testing):

```bash
python3 definitions/json2h.py --no-native-regex definitions/your_definition.json
```

With `--no-native-regex`, native function pointers are set to `NULL` while the regex strings are preserved.

### Regenerating All Headers

Run the helper script [regenerate.sh](file:///home/boris/projects/textparser/definitions/regenerate.sh) from the `definitions/` directory:

```bash
cd definitions
./regenerate.sh
```

## License

See `LICENSE` file for details.
