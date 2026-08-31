# TextParser [![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/bokic/textparser)

TextParser is a high-performance, extensible text parsing library written in C. It uses regular expressions to define language grammars and generates a hierarchical Abstract Syntax Tree (AST) for parsed documents.

The project currently provides support for: Ada, ASM, Bash, C, C++, C3, CFML, C#, CSS, Fortran, Go, HTML, Jai, Java, JavaScript, JSON, Markdown (MD), MATLAB, Pascal, Perl, PHP, Python, R, Rust, Scratch, SQL, Swift, TypeScript, VB, Zig.

## Features

- **High Performance**: Written in optimized C for fast parsing (up to 100MB/s) of large codebases.
- **Small Footprint**: Less than 100KB for both parser and language definition, easy to integrate.
- **Zero Dependencies for Built-in Languages**: All 30 built-in language grammars use pure C matchers with no external libraries. PCRE2 is only loaded at runtime for custom JSON grammar definitions.
- **Hierarchical AST/CST**: Generates a structured tree of tokens (`textparser_token_item`) representing the code structure.
- **Syntax Highlighting Support**: Tokens carry styling metadata (text color, background, font style) suited for CLI syntax viewers, LSP servers, and code editors.
- **Extensibility**: Language definitions are decoupled from the core logic, defined with JSON, and can be loaded at compile time or at runtime.
- **Native Query Engine**: Query AST nodes using CSS-like selectors (`"Parent > Child"`, `"Ancestor Descendant"`, `"TypeA, TypeB"`).
- **Incremental Parsing**: Efficiently re-parses only modified sections of a buffer.
- **Modern C23 & C++23**: Compatible with GCC, Clang, and MSVC.

## Project Structure

- **`src/`**: Core C library (`textparser.c`, `textparser-json.c`).
- **`include/`**: Public headers (`textparser.h`, `textparser-json.h`).
- **`cli/`**: Command-line tool for parsing and debugging.
- **`ccat/`**: Syntax highlighting CLI utility (color cat).
- **`definitions/`**: Language definitions (JSON format).
- **`ports/`**: Bindings and ports for Python, Rust, Java, and WebAssembly.
- **`tests/`**: Unit and integration tests.

## Build Instructions

### Prerequisites

- CMake 3.15+, Ninja, GCC or Clang
- PCRE2 & JSON-C:
  - Ubuntu/Debian: `sudo apt install libpcre2-dev libjson-c-dev`
  - Arch Linux: `sudo pacman -S pcre2 json-c`
  - macOS: `brew install pcre2 json-c pkg-config ninja`

### Building

```bash
./build.sh
```

On Windows, use the batch scripts in `windows/`. Artifacts are output to `bin/`.

## Testing

```bash
bin/unittests
```

## Installation

### Arch Linux (AUR)

```bash
yay -S textparser
```

### Fedora (Copr)

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

Binary releases are available on the [releases](https://github.com/bokic/textparser/releases) page.

### Docker

```bash
docker pull bokic78/textparser:latest

# Parse a file
docker run --rm -w /work -v "$PWD":/work:ro bokic78/textparser ./file.cfm

# Syntax highlight with ccat
docker run --rm -w /work -v "$PWD":/work:ro --entrypoint ccat bokic78/textparser ./file.c
```

## Usage

### CLI Tool

```bash
# Parse a file
bin/textparser path/to/file.cfm

# Parse using a custom runtime definition
bin/textparser path/to/file.json --definition definitions/json_definition.json
```

### C Library

```c
#include <textparser.h>

extern const textparser_language_definition my_lang_definition;

int main() {
    textparser_defer(handle);

    if (textparser_openfile("example.txt", TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_BOM_ALL, &handle) != 0)
        return 1;

    if (textparser_parse(handle, &my_lang_definition) != 0)
        return 1;

    for (textparser_token_item *item = textparser_get_first_token(handle); item != NULL; item = item->next) {
        // process token
    }

    return 0;
}
```

### C++ RAII Wrapper

```cpp
#include <textparser.hpp>

extern const textparser_language_definition my_lang_definition;

int main() {
    textparser::Parser parser;
    if (parser.openfile("example.txt", TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_BOM_ALL) == 0)
        if (parser.parse(&my_lang_definition) == 0)
            for (auto *item = parser.get_first_token(); item; item = item->next) { /* process */ }
    return 0; // auto-cleanup on scope exit
}
```

## Language Definitions

Language grammars are defined as JSON files in `definitions/`. To compile a JSON definition into a C header:

```bash
python3 definitions/json2h.py definitions/your_definition.json
```

To regenerate all headers:

```bash
cd definitions && ./regenerate.sh
```

A migration tool converts definitions to the unified v2 schema format (output goes to `definitions/schema_v2/` by default, leaving originals untouched):

```bash
python3 scripts/migrate_definitions.py --all definitions/
```

See [`schema/textparser-schema.json`](schema/textparser-schema.json) for the full schema specification.

## License

See `LICENSE` file for details.
