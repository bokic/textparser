# textparser 🚀

[![Language: C](https://img.shields.io/badge/Language-C-blue?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Language: Rust](https://img.shields.io/badge/Language-Rust-orange?logo=rust&logoColor=white)](https://www.rust-lang.org)
[![Language: Python](https://img.shields.io/badge/Language-Python-3776AB?logo=python&logoColor=white)](https://www.python.org)
[![Language: Java](https://img.shields.io/badge/Language-Java-ED8B00?logo=openjdk&logoColor=white)](https://www.java.com)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Arch Linux AUR](https://img.shields.io/badge/Arch_Linux-AUR-1793D1?logo=arch-linux&logoColor=white)](https://aur.archlinux.org/packages/textparser)

A lightning-fast, multi-language **Abstract Syntax Tree (AST) generator** and syntax highlighter. Architected with a high-performance **C core engine** powered by **PCRE2** and **JSON-C**, `textparser` provides native, zero-overhead bindings and ports for **Rust**, **Python**, and **Java**.

It serves as a robust foundation for building linters, static analysis tools, compilers, and terminal utilities like the built-in `ccat` clone.

### Definition-driven CST categories

The C API `textparser_node_get_category(node)` returns a `textparser_cst_category`
without requiring a parser handle. Schema-v2 productions and inline constructs
accept an optional `category`: `unknown`, `token`, `source_file`, `declaration`,
`statement`, `expression`, `type`, `jsx`, `pattern`, or `other`. For example:

```json
{"sequence": [{"token": "Identifier"}], "category": "declaration"}
```

Metadata applies to emitted containers, including anonymous containers and
sequences renamed by a named choice. Transparent references and choices that
return an existing child retain that child's category; token productions retain
the token fallback. Omitted or `unknown` metadata uses generic structural defaults:
leaves are `TOKEN`, non-synthetic nodes with children are `EXPRESSION`, and other
containers or missing tokens are `OTHER`. A null node returns `UNKNOWN`.
No category is inferred from the language name or CST kind spelling.

This replaces `textparser_typescript_cst_category_of(handle, node)`,
`textparser_typescript_cst_category`, and `TEXTPARSER_TS_CST_*` with
`textparser_node_get_category(node)`, `textparser_cst_category`, and
`TEXTPARSER_CST_*`. Numeric family values and TypeScript golden CST output are
preserved. Rebuild consumers because the public production and node structs now
include a category field. The JSON loader supports this schema-v2 metadata;
`json2h.py` still emits legacy definitions without grammar tables.

### Declarative grammar guards

Use `when` for non-consuming conditions, without registering a native callback:

```json
{"when": {"noLineTerminatorBefore": true}}
{"when": {"nextTokenIn": ["Dot", "Semicolon"], "allowEOF": true}}
{"when": {"nextToken": "Identifier", "nextTokenText": "meta"}}
```

Fields within a guard combine with AND. `nextToken` and `nextTokenIn` match token
kinds; `nextTokenText` matches exact, case-sensitive raw spelling, not a decoded
identifier. Text comparison supports UTF-8 and UTF-16/32 source input. Token guards
reject EOF unless `allowEOF` enables it for the token-kind condition; a text
condition still rejects EOF. Newline guards inspect trivia before the next token;
EOF is treated as having no preceding newline, preserving the previous C behavior.
`lineTerminatorBefore` is also supported; false inverts either newline condition.

Define file profiles under `grammar.sourceFileKinds`:

```json
"sourceFileKinds": {
  "jsx": [".tsx", ".jsx"],
  "javascript": [".js", ".jsx", ".mjs", ".cjs"]
}
```

`{"when": {"sourceFileKind": "jsx"}}` matches any suffix in that profile,
ignoring ASCII case. Unnamed sources match no profile. Negate a condition using
`{"not": {"when": {"sourceFileKind": "javascript"}}}`; alternatives can use
`choice`. Undefined profiles/tokens, empty lists, malformed fields, and conflicting
conditions fail definition loading. `native` remains available as a standalone
registered callback condition and cannot be mixed with generic fields.

The TypeScript definition now uses these guards for all six former native
predicates. TypeScript filename rules and type-argument follower tokens live in
JSON. Assignment/update-target validators are separate and remain unchanged.
C consumers must rebuild because `textparser_production` now includes a `guard`
pointer. Other schema-advertised guard fields, such as `feature` and
`languageVersionAtLeast`, are not implemented by this change.

### Declarative diagnostic messages

Tokens can supply a display `spelling`, and a language can define diagnostic
codes and message templates:

```json
"diagnostics": {
  "expected": {"code": "E_EXPECTED", "message": "Expected %s."},
  "tokenExpected": {"code": "E_TOKEN", "message": "'%s' expected."},
  "recovered": {"code": "E_RECOVERED", "message": "Recovered while parsing %s."}
}
```

For example, a lexer token can contain `"spelling": ";"`. Tokens may override
`diagnostics.expected`; productions and inline constructs may override
`diagnostics.expected` and `diagnostics.recovered`. Each template requires a
nonempty `code` and `message`.

Expected-error precedence is production override, token override, language
`tokenExpected` (when a spelling exists), then language `expected`. Recovery uses
the production's `recovered` override or the language default. With no configured
template, the existing `TEXTPARSER_EXPECTED`/`TEXTPARSER_RECOVERED` defaults apply;
a token spelling supplies the default `'<spelling>' expected.` message.

Templates accept at most one `%s` and any number of `%%` escapes. `%s` receives
the token spelling for expected-token errors, otherwise `expect`, the production
name, or `syntax element`. Recovery always uses the latter description. `%%`
emits a literal percent. Unsupported conversions, empty strings, embedded NULs,
and overrides at unsupported scopes are rejected. Formatting treats substituted
text literally and retains the existing 255-byte message limit.

`textparser_grammar_report_expected()`, grammar synchronization recovery, and
trailing unconsumed token diagnostic reporting are completely language-independent.
There are no language-name checks or hardcoded language logic in `src/textparser.c`.
Diagnostic templates, spellings, recovery synchronization tokens, and boundary
detection are driven entirely by language definitions (`definitions/*.json`).
Both the JSON loader and `json2h.py` preserve token and language metadata;
the generator still does not emit schema-v2 grammar tables.
Rebuild C consumers after the public token, production, and language struct changes.

### Decoupled semantic validation & Pratt operand validators

Pratt expression operand validation and AST early-error legality checks are decoupled
from the core parser into pluggable validators and language validation modules:

- **Pluggable Operand Validators:** Registered via `textparser_register_operand_validator(handle, name, fn, user_data)`.
  Pratt productions specify `"validateOperand": "typescript.assignmentTarget"` or `"typescript.updateTarget"`
  in JSON without embedding language-specific AST inspection in `src/textparser.c`.
- **Pluggable Token Validators:** Tokens in language definitions can specify `"validator": "validator_name"`
  (e.g., `"validator": "typescript.identifier"` on `Identifier` and `PrivateIdentifier`). The contextual
  lexer automatically invokes `textparser_validate_token()` during candidate rule evaluation.
- **Unicode Escape Validation:** Decoupled from `textparser.c` into `src/validation/typescript.c` and registered
  via `textparser_typescript_register_validators(handle)` as the `"typescript.identifier"` validator.
- **TypeScript Semantic Validation Module:** Implemented in `src/validation/typescript.c` and
  exported via `libtextparser_typescript.so` (header `src/validation/typescript.h`). Provides:
  - `textparser_typescript_register_validators(handle)`: registers token validators, Pratt operand validators, and the `"typescript.legality"` / `"source.complete"` event handlers.
  - `textparser_validate_typescript(handle)`: extracts validation diagnostics matching the `src/validation/` convention (`cfml`, `php`, `html`, `css`).
  - Standalone validation CLI: `typescript_validation_test`.
- **Event Lifecycle Dispatch:** Post-parse legality checks are triggered dynamically via grammar definition events (`"events": { "onSourceComplete": "typescript.legality" }`).

---

## ✨ Features

- **🌐 Massive Language Support:** Built-in regex-based grammars for modern and classic languages.
- **⚡ High Performance:** Core tokenization and AST construction written in highly optimized C.
- **🧬 Multi-Language Ecosystem:** Native language ports (Rust, Python, Java) manage underlying C memory safely. Detailed inner logic and porting contracts are specified in [ARCHITECTURE.md](ARCHITECTURE.md).
- **🎨 Built-in `ccat` Utility:** A colorized alternative to the standard `cat` command for terminal code viewing.

---

## 📚 Supported Languages

`textparser` features rich tokenization and syntax parsing rules for:
* **System & General:** C, C++, C#, Java, Go, Rust, Swift, Zig, C3, V, Ada, Assembly (x86/ARM)
* **Web & Data:** HTML, CSS, JavaScript, TypeScript, JSON, XML, SQL, Markdown
* **Scripting:** Python, PHP, Bash/Shell

---

## 🚀 Installation

### 📦 Linux Packages

#### **Arch Linux (AUR)**
```bash
yay -S textparser
# Or build from the latest git commit
yay -S textparser-git
```

#### **Ubuntu / Debian (PPA)**
```bash
sudo add-apt-repository ppa:bbarbulovski-gmail/textparser
sudo apt-get update
sudo apt-get install textparser
```

### 🐳 Docker
Pull and run the pre-configured container instantly:
```bash
docker pull bokic78/textparser
docker run --rm -v \$(pwd):/workspace bokic78/textparser --file /workspace/main.c
```

---

## 🛠️ Building from Source

### Prerequisites
Ensure you have `cmake`, `ninja` (optional but recommended), `libpcre2`, and `libjson-c` installed on your system.

### Compiling the C Core & CLI
```bash
git clone https://github.com/bokic/textparser.git
cd textparser
./build.sh
```

### Windows builds

The Windows scripts use LLVM/Clang and Ninja. Build dependencies and the
project for one architecture, or omit the architecture to build both `x64`
and `arm64`:

```bat
cd windows
build_deps.bat [x64|arm64]
build.bat [x64|arm64]
build_zip.bat [x64|arm64]
```

Architecture-specific binaries and ZIP archives are written separately, for
example to `bin\arm64` and `windows\textparser-<version>-arm64.zip`.

---

## 💡 Quick Usage Examples

### 💻 CLI Usage (AST Generation)
Generate a clean, structured JSON representation of a source file's AST:
```bash
textparser main.c --json
```

### 🎨 Colorized Cat (`ccat`)
View your code with automatic, high-performance syntax highlighting in the terminal:
```bash
ccat main.rs
```

---

## 🤝 Contributing

Contributions are what make the open-source community an amazing place to learn, inspire, and create. 

1. **Fork** the project.
2. **Create** your feature branch (`git checkout -b feature/AmazingFeature`).
3. **Commit** your changes (`git commit -m 'Add some AmazingFeature'`).
4. **Push** to the branch (`git push origin feature/AmazingFeature`).
5. **Open a Pull Request**.

## 📄 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

Developed with ❤️ by [Boris Barbulovski (bokic)](https://github.com).
