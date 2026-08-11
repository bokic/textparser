# TextParser [![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/bokic/textparser)

TextParser is a high-performance, extensible text parsing library written in C. It uses regular expressions to define language grammars and generates a hierarchical Abstract Syntax Tree (AST) for parsed documents.

The project currently provides support for: Ada, ASM, Bash, C, C++, C3, CFML, C#, CSS, Fortran, Go, HTML, Jai, Java, JavaScript, JSON, MATLAB, Pascal, Perl, PHP, Python, R, Rust, Scratch, SQL, Swift, TypeScript, VB, Zig.
It has a flexible architecture making it easy to add new languages.

## Features

- **High Performance**: Written in optimized C for fast parsing of large codebases.
- **Small Footprint**: The library is designed to be small and easy to integrate into other projects.
- **Minimal Dependencies**: The library has minimal dependencies (only PCRE2 library for regex matching).
- **Regex-Based Grammars**: Define language syntax using flexible regular expressions.
- **Hierarchical AST**: Generates a structured tree of tokens (`textparser_token_item`) representing the code structure.
- **Syntax Highlighting Support**: Tokens track metadata like color, background, and flags, making it suitable for building syntax highlighters and editors.
- **Extensibility**: Language definitions are decoupled from the core parsing logic, constructed with JSON, and can be loaded at compile time (by generated header file) or at runtime (by loading JSON file).
- **Conditional Start Tokens (`overrideStartTokens`)**: Dynamic start token override rules based on file extension and regex pattern matching at document start (used e.g. for modern ColdFusion script components).
- **Context-Sensitive Token Replacement (`contextNestedTokens`)**: Tokens can dynamically specify context-sensitive child token lists based on enclosing parent token types in the parsing stack.
- **Non-Fatal Error Resynchronization**: Recovers gracefully from malformed syntax without aborting parsing, grouping contiguous invalid input into merged `AST_NODE_ERROR` nodes (`TEXTPARSER_TOKEN_ID_ERROR`).
- **BOM Specification (`SupportedBom`)**: Grammar-level specification of allowed Byte Order Marks (e.g., UTF-8, UTF-16-LE, UTF-16-BE).
- **Native Query Engine (`textparser_query`)**: High-performance C selector engine to query AST nodes using intuitive CSS-like selector syntax (`"Parent > Child"`, `"Ancestor Descendant"`, `"TypeA, TypeB"`).
- **Operator Precedence & Pratt Parsing (`operator_precedence`)**: Top-Down Operator Precedence algorithm integrated into the C parser to pivot flat operator token sequences into structured binary/unary expression trees with configurable binding power and associativity (`left`/`right`).
- **Incremental Parsing (`textparser_parse_incremental`)**: Efficiently re-parses modified document regions using parser state snapshots (`textparser_parser_state`), optimized for real-time text editors and IDE integrations. *Note: Post-parse AST node unwrapping rules such as `delete_if_only_one_child` are currently ignored during incremental parsing to avoid stack frame mutation overhead.*
- **Python Tooling**: Includes Python scripts for prototyping, validation of the core algorithm, generation of C header files (`json2h.py`), and other parser verification tools.

## Project Structure

- **`src/`**: Core C library implementation and private headers (`textparser.c`, `adv_regex.c`, `adv_regex.h`, `logger.h`).
- **`include/`**: Public header files (`textparser.h`).
- **`cli/`**: Command-line tool for testing, debugging, and demonstrating the library.
- **`definitions/`**: Language definitions (e.g., CFML, JSON).
- **`python/`**: Python bindings, prototypes, and validation tools (`validate_cfml.py`).
- **`tests/`**: Unit and integration tests.
- **`ccat/`**: Syntax highlighting CLI utility (color cat).

## Build Instructions

### Prerequisites

- CMake (version 3.15 or higher)
- Ninja build system
- A C/C++ compiler (GCC or Clang)
- PCRE2 library (`pcre2-8`)
  - Ubuntu/Debian: `sudo apt install libpcre2-dev`
  - Arch Linux: `sudo pacman -S pcre2`
  - macOS: `brew install pcre2`

### Building

You can use the provided build script for a quick start:

```bash
./build.sh
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

To use TextParser in your C project, include `textparser.h` and link against `libtextparser`.

**Basic Example:**

```c
#include <textparser.h>
#include <stdio.h>

// Assume 'my_lang_definition' is defined elsewhere
extern const textparser_language_definition my_lang_definition;

int main() {
    textparser_defer(handle); // Auto-cleanup

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

### Regenerating All Headers

Run the helper script [regenerate.sh](file:///home/boris/projects/textparser/definitions/regenerate.sh) from the `definitions/` directory:

```bash
cd definitions
./regenerate.sh
```

## License

See `LICENSE` file for details.
