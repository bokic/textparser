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
include a category field. Both the JSON runtime loader (`src/textparser-json.c`)
and the static header compiler (`definitions/json2h.py`) fully support schema-v2
declarative grammar tables, lexer modes, lexical goals, and Pratt operators.

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
- **♻️ Incremental, AST-aware re-parsing:** `textparser_parse_incremental` re-lexes only the edited region. It anchors at the root, resets the transient lexical state (mode stack / lexical goal) so the anchor is state-safe, and updates the immutable lexer snapshot with each token's reconstructed lexer mode. The snapshot buffers are retained across edits (no per-edit free/realloc) and an edit contained in a single leaf patches the snapshot in place; structural edits rebuild it. When the tree was previously passed to `textparser_post_process`, the engine flattens the synthesized AST wrappers, splices the edit, and re-derives expression grouping and cast/declaration/template disambiguation, so the AST stays consistent across edits. Trees that were never post-processed stay raw CSTs.
- **🧬 Multi-Language Ecosystem:** Native language ports (Rust, Python, Java) manage underlying C memory safely. Detailed inner logic and porting contracts are specified in [ARCHITECTURE.md](ARCHITECTURE.md).
- **🎨 Built-in `ccat` Utility:** A colorized alternative to the standard `cat` command for terminal code viewing.

---

## 📚 Supported Languages

`textparser` features rich tokenization and syntax parsing rules for:
* **System & General:** C, C++, C#, Java, Go, Rust, Swift, Zig, C3, V, Ada, Assembly (x86/ARM)
* **Web & Data:** HTML, CSS, JavaScript, TypeScript, JSON, XML, SQL, Markdown
* **Scripting:** Python, PHP, Bash/Shell

The PHP v2 migration targets **PHP 8.5** and is complete: the declarative grammar
lives at `definitions/php_definition.json` and is the default for `.php` files.
The profile declares whitespace and comments as lexer trivia
and distinguishes plain assignment from compound assignment. Dedicated C-engine
tests in `tests/unittests/php_grammar_tests.cpp` check complete source consumption,
basic statements and calls, arrays, arithmetic precedence, assignment associativity,
and missing-operand diagnostics. Outside PHP tags, all text (including HTML) is
`OutsidePHP` trivia. Opening tags push the PHP lexer mode; closing tags pop it.
Transitions work within braced statement bodies, and `?>` can terminate ordinary
statements. Line comments stop before closing tags; strings and block comments
retain embedded tag text. The profile supports `<?php`, `<?=`, and short `<?` tags
(short tags are always enabled in this profile). Outside trivia creates no grammar
CST nodes, so trailing trivia need not extend the final syntax node's source span.
See the [PHP migration checklist in ROADMAP.md](ROADMAP.md#remaining-work) for
the verification history.
The grammar covers pipes, first-class callables, `(void)` statements,
property hooks, asymmetric visibility, promoted final properties, DNF types,
typed/attributed constants, grouped imports, alternative control syntax,
anonymous classes, references, and heredoc/nowdoc strings. The 31-sample core
corpus and a 123-sample extended corpus pass both the C grammar and the official
PHP 8.5.10 lint check; their JSON-loaded and generated-header CST outputs match
exactly. The extension also added 26 malformed samples, unterminated-construct
checks, and a member-boundary recovery check, and fixed three grammar gaps found
along the way (`|=`/`^=` compound assignment, `new (expr)`, and `namespace;`). All
69 PHP unit tests pass (31 grammar/validation cases through each definition path,
plus 7 parser/validator tests). Grammar acceptance matches PHP 8.5.10 for
253 differential samples; the only differences are intentional (`PHP2008` arity
diagnostics on syntactically valid calls and the PHP-semantic rejection of
redeclaring a built-in function). Calls distinguish unpacked arguments (`...$args`) from standalone
callable placeholders (`...` followed by `)`, with optional trivia). Regression
coverage includes multiple unpacks, nested calls, named arguments after
unpacking, trailing commas, and malformed placeholders.

The Bash v2 grammar lives at `definitions/bash_definition.json` and is the default for `.sh`/`.bash` files;
it covers simple commands, assignments (`+=`, empty values, array initializers), pipelines (`|`, `|&`),
and/or lists (`&&`, `||`, `&`), redirections (including fd and `2>&1`),
`if`/`elif`/`else`, `for` (with/without `in`, and C-style `for ((...))`), `while`/`until`, `select`, `case`
(globs, `|` alternatives, `[!...]` classes, `-*`/`--opt=*` patterns), functions
(`name()`, `function name`, `function name()`), groups `{ }`, subshells `( )`,
brace expansion `{a,b}`/`{1..5}`, `$'...'` ANSI-C quoting, `[[ ]]`/`(( ))` (including `=~`), the parameter-expansion
operators (`${x%%}`, `${x##}`, `${x:-}`, `${x-c}`, `${x:1:2}`, `${x/a/b}`,
`${#x}`), command/process/backtick substitution, arithmetic, line continuations,
comments, here-docs (`<<`, `<<-`, `<< EOF`, multiple per line, `<<-` tab
stripping, blank lines trailing here-docs, and here-docs in command substitutions with subshells and functions),
`case` pattern clauses (with/without command bodies, multiple patterns `p1|p2`, trailing newlines before `;;`),
arithmetic operators (`+`, `-`, `*`, `/`) in word argument positions and backticks,
as well as POSIX command splitting for reserved words as arguments.
The grammar achieves a 100% clean parse rate across all 523 `bash -n`-valid `/usr/bin/*` shell scripts.
`tests/unittests/bash_v2_grammar_tests.cpp` exercises both unit grammar fixtures and end-to-end command lists.

`definitions/c_definition.json` provides a compiler-grade ISO C (C89 through C23) schema-v2 grammar, featuring granular keyword and operator tokens, an 18-level Pratt operator precedence hierarchy (handling ternary `? :`, assignment chains, comma operators, cast expressions, pointer member access `->`, and prefix/postfix increment and decrement), C23 standard attributes `[[...]]`, `_Static_assert`/`static_assert`, `typeof`/`typeof_unqual`, `_Generic` selection, bitfields, designated initializers, compound literals, and synchronization-token recovery. The legacy v1 C scanner is preserved as `definitions/c_legacy_definition.json` for legacy tokenization test compatibility. `tests/unittests/c_v2_grammar_tests.cpp` verifies both static header and dynamic JSON definition pipelines.

`definitions/cpp_definition.json` provides a compiler-grade ISO C++ (C++98 through C++23) schema-v2 grammar, featuring namespaces (including nested `A::B` and anonymous), classes, structs, inheritance, access specifiers (`public:`, `protected:`, `private:`), constructors with member initializer lists, destructors, operator overloading (`operator+`, `operator[]`, `operator()`, etc.), templates (class and function templates, variadic parameter packs, and default arguments), concepts and constraints (`concept`, `requires`), lambdas (captures, parameter lists, `mutable`, trailing return types), modern control flow (range-based `for`, init statements in `if`/`switch`, `try`/`catch`), coroutines (`co_await`, `co_yield`, `co_return`), structured bindings (`auto [x, y]`), C++ cast expressions (`static_cast`, `dynamic_cast`, `const_cast`, `reinterpret_cast`), spaceship operator `<=>`, pointer-to-member operators (`.*`, `->*`), memory management (`new`, `delete`), and diagnostic error recovery. The legacy v1 C++ scanner is preserved as `definitions/cpp_legacy_definition.json` for legacy test suite compatibility. `tests/unittests/cpp_v2_grammar_tests.cpp` verifies both static header and dynamic JSON definition pipelines.

`definitions/csharp_definition.json` provides a compiler-grade C# (C# 1.0 through C# 12) schema-v2 grammar, featuring file-scoped and block namespaces, classes, structs, records (`record`, `record class`, `record struct` with positional parameter lists), interfaces, enums with explicit underlying types, properties (auto-properties, expression-bodied `=>`, init-only accessors `init`, and accessor blocks `get`/`set`), methods, constructors with `base`/`this` initializers, destructors, events, indexers, generics and type parameter constraints (`where T : class, new()`), modern control flow (`foreach`, `switch` statements and switch expressions `=>`, `try`/`catch`/`finally` with `when` filters, `lock`, `using`), lambdas (typed and untyped parameters, expression/block bodies), pattern matching (`is` expressions, declaration and constant patterns), null-coalescing (`??`, `??=`), null-conditional (`?.`), null-forgiving (`!`), with-expressions (`cfg with { ... }`), global namespace qualifier (`global::`), attributes (`[Serializable]`, `[target: ...]`), verbatim (`@""`), interpolated (`$""`), and raw (`"""..."""`) string literals, numeric formats with digit separators (`_`), and synchronization-token diagnostic recovery. The legacy v1 C# scanner is preserved as `definitions/csharp_legacy_definition.json` for legacy test suite compatibility. `tests/unittests/csharp_v2_grammar_tests.cpp` verifies both static header and dynamic JSON definition pipelines.

`definitions/java_definition.json` provides a compiler-grade Java (Java 1.0 through Java 21+) schema-v2 grammar, featuring package declarations, single-type and static on-demand imports, modules (`module`, `open module`, `requires transitive`, `exports`, `opens ... to`, `uses`, `provides ... with`), classes, interfaces, enums (with constant arguments, enum constant bodies, and constructors), records (`record` with record headers and compact constructors), sealed classes and interfaces (`sealed`, `non-sealed`, `permits`), constructors, methods, fields, annotations and annotation types (`@interface`), generics with upper and lower bounds (`<T extends Comparable<? super T>>`, `List<? extends Number>`), modern control flow (`for`, enhanced `for`/foreach, `while`, `do`/`while`, `if`/`else`), switch statements and switch expressions (`->` rules, `yield`, pattern matching `case Type identifier when condition`), try-with-resources (multiple auto-closeable resources) and multi-catch (`catch (IOException | SQLException ex)`), lambdas (`(params) -> expr/block`), method references (`Type::method`, `expr::method`, `Type::new`), text blocks (`"""..."""`), literals (binary, hex, underscores, floats/doubles, char escapes), 14-level Pratt operator precedence, and synchronization-token recovery. The legacy v1 Java scanner is preserved as `definitions/java_legacy_definition.json` for legacy test suite compatibility. `tests/unittests/java_v2_grammar_tests.cpp` verifies both static header and dynamic JSON definition pipelines.

For the v2 profile, link `libtextparser_php` and call
`textparser_php_register_validators(handle)` before executing the language grammar.
The `php.legality` source-complete handler checks writable assignment/update
targets, comparison and ternary chains, interpolation syntax, and heredoc/nowdoc
indentation. It parses interpolation fragments with the same C PHP grammar,
including nested expressions and strings; the outer CST still represents strings
as tokens. Diagnostics use `PHP2001`–`PHP2009`, with `PHP2007` for the validation
nesting limit. Check diagnostics even when grammar execution returns a match.
`textparser_validate_php(handle)` exposes these diagnostics through the validation
API. The `php.legality` handler also checks built-in call arity (`PHP2008`),
skipping methods, static and dynamic calls, namespaced calls, first-class
callables, `new` expressions, and functions declared in the same source, and
rejects empty array elements in array literals (`PHP2009`) unless the array is a
destructuring target. The v2 profile is now the default definition: `textparser`,
`ccat`, and `php_validation_test` use it, and `test_php.c` registers the
validators and executes the grammar before validating. The legacy validator path
in `php.c` remains for callers that load a legacy PHP definition.
The legacy built-in signature table is now generated from the official PHP 8.5.10
source by `src/validation/generate_php_functions.py`, including parameter names,
optional/variadic flags, SAPI `additional_functions` tables for CLI-only functions
(`cli_get_process_title`), and alternative signatures for platform-dependent builds
(`ldap_connect` has two or five parameters). A call is accepted when any variant's
arity matches. Generator regressions are in `tests/test_php_functions.py`; arity
regressions are in `tests/unittests/php_tests.cpp`.
`./build.sh` regenerates the PHP v2 header for both-path regression testing.

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
`textparser` is cross-platform and fully supports both little-endian (e.g. x86_64, aarch64) and big-endian architectures (e.g. IBM s390x). UTF-16 and UTF-32 encodings use host-endian code units (`uint16_t` and `uint32_t`).

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

Developed with ❤️ by [Boris Barbulovski (bokic)](https://github.com/bokic).
