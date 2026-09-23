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
Productions and inline constructs can also specify `astKind` (e.g. `"astKind": "VariableDeclaration"`)
to override the emitted CST node kind independently of the production rule name. When omitted,
emitted CST kinds use the production rule name or structural defaults.

This replaces `textparser_typescript_cst_category_of(handle, node)`,
`textparser_typescript_cst_category`, and `TEXTPARSER_TS_CST_*` with
`textparser_node_get_category(node)`, `textparser_cst_category`, and
`TEXTPARSER_CST_*`. Numeric family values and TypeScript golden CST output are
include a category field. The JSON runtime loader (`src/textparser-json.c`),
the static header compiler (`definitions/json2h.py`), and the Java port loader
(`com.textparser.grammar.GrammarLoader`) fully support schema-v2 declarative grammar
tables, lexer modes, lexical goals, Pratt operators, and grammar constructs including
`oneOrMore` (desugared into sequence and repeat), with left recursion and nullable loop validation.

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
* **System & General:** C, C++, C#, Java, Go, Rust, Swift, Zig, C3, Jai, V, Ada, Assembly (x86/ARM)
* **Web & Data:** HTML, CSS, JavaScript, TypeScript, JSON, XML, SQL, Markdown
* **Scripting:** Python, PHP, Bash/Shell

The C3 schema-v2 migration targets **C3 0.8.4**, the
[latest stable release](https://github.com/c3lang/c3c/releases/tag/v0.8.4)
verified on 2026-09-21. `definitions/c3_definition.json` is the primary definition
for `.c3`, `.c3i`, and `.c3t`; the legacy scanner remains in
`definitions/c3_legacy_definition.json` for scanner regression tests. The obsolete
`definitions/schema_v2/c3_definition.json` draft has been removed.

The declarative grammar covers modules and imports, generic module parameters
and type arguments, aliases and function aliases, structs/unions, enums with
associated data, interfaces, bitstructs, fault definitions, attributes, functions,
methods, macros and lambdas, runtime and compile-time control flow, optional
operators, calls and named arguments, slices, initializers, and inline assembly.
Pratt operators use C3 precedence (including shifts binding more tightly than
addition), with right-associative assignment and ternaries. Nested block comments,
escaped/raw strings, hex floats, documentation attached to declarations, and
statement-boundary recovery have dedicated regression coverage. Obsolete postfix
`?` and the eight previously identified compiler-rejected forms are rejected.

`tests/unittests/c3_v2_grammar_tests.cpp` checks generated-header and JSON loading,
full input consumption, clean successful parses, malformed inputs, precedence,
recovery diagnostics, CST categories, and matching tree spans across both paths.
Run `python3 tests/c3_compare/verify_fixtures.py` to check the literal unit fixtures
against the installed `c3c` using syntax-only `-P` compilation. This is parser
coverage; name resolution, type checking, and other compiler semantic checks are
outside the grammar's scope.

The Zig schema-v2 migration targets **Zig 0.16.0**, matching the installed
`zig version`. `definitions/zig_definition.json` is the primary `.zig`
definition; `definitions/zig_legacy_definition.json` preserves the old scanner
for tokenization regression tests. The obsolete `definitions/schema_v2` Zig
draft has been removed.

The grammar covers container and root fields, functions and declaration
attributes, inferred and explicit error unions, pointer/slice/array sentinels and
qualifiers, function types, struct/tuple initializers, destructuring, labeled
blocks and switches, loop and error captures, inline assembly, and documentation
comments. Zig's shared type/value syntax is represented by common productions.
Arithmetic uses Pratt parsing; explicit expression layers enforce non-chained
comparisons and the shared precedence of bitwise operators, `catch` (including
its capture), and `orelse`. Recovery retains following block statements and
subsequent declarations. Strings and line strings are whole tokens: Zig has no
string interpolation or nested block comments requiring additional lexer modes.
Removed `usingnamespace`/`async`/`await` constructs are rejected; those words
remain usable as ordinary identifiers in 0.16.0.

`tests/unittests/zig_v2_grammar_tests.cpp` checks both generated C and JSON-loaded
definitions, complete input consumption, malformed syntax and literals,
precedence, recovery, and identical CST names, categories, spans, and diagnostics.
Run `python3 tests/zig_compare/verify_fixtures.py` for comparison with the installed
compiler (`zig fmt --stdin` for syntax, `zig ast-check` for invalid literals).
See `tests/zig_compare/README.md` for the verification scope. Name resolution,
type checking, and compiler semantic checks are outside this grammar's scope.

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

`definitions/rust_definition.json` provides a compiler-grade Rust (2015 through 2024 editions) schema-v2 grammar, featuring outer and inner attributes (`#[...]`, `#![...]`), modules (`mod foo;`, `mod foo { ... }`), visibility modifiers (`pub`, `pub(crate)`, `pub(in path)`), functions (with `async`, `const`, `unsafe`, `extern "C"`, where-clauses, return types `->`), structs (record, tuple, and unit structs), enums with explicit discriminants and tuple/record variants, unions, traits and trait implementations (`impl Trait for Type`, negative/unsafe impls), type aliases, constants, statics, control flow expressions (`if`/`else`, `match` with arms and guards, `while`, `for ... in ...`, labeled loops `'label: loop`), closures (with `move`, arguments, return types, block/expression bodies), pattern matching (identifiers, tuples, structs, slices, ranges `..`/`..=`, reference/ref mut, wildcard `_`, or-patterns), generics and lifetimes (`<'a, T: Clone + 'static>`), turbofish invocations (`::<Vec<_>>()`), method chaining and field access, macro invocations (`vec![...]`, `println!(...)`) and `macro_rules!`, raw string literals (`r#"..."#`, `r##"..."##`), byte strings (`b"..."`), character literals and escapes, full integer and float literal suffixes with digit separators (`0xDEAD_BEEF_u64`, `3.14159_f64`), a 12-level Pratt operator precedence table (handling assignment, ranges, logical, equality, relational, bitwise, shift, additive, and multiplicative operators), condition-specific expression contexts (`ConditionExpression`) preventing struct-literal brace ambiguities, and synchronization-token recovery. The legacy v1 Rust scanner is preserved as `definitions/rust_legacy_definition.json` for legacy test suite compatibility. `tests/unittests/rust_v2_grammar_tests.cpp` verifies both static header and dynamic JSON definition pipelines.

`definitions/python_definition.json` provides a compiler-grade Python (Python 3.0 through Python 3.12+) schema-v2 grammar, featuring module and package import structures (`import`, `from ... import`, relative imports `from . import`), functions and methods (with positional-only parameters `/`, keyword-only separators `*`, `*args`, `**kwargs`, default values, type annotations `: type`, and return annotations `->`), async functions (`async def`) and coroutines (`await`), classes and inheritance, decorators (`@expr`), type aliases (`type Alias[T] = ...`, PEP 695), type parameters on functions and classes, control flow statements (`if`/`elif`/`else`, `while`/`else`, `for`/`else`, `async for`), exception handling (`try`/`except`/`except*`/`else`/`finally`), context managers (`with`, `async with`), pattern matching (`match`/`case`, PEP 634, with sequence, mapping, class, capture, wildcard, and or-patterns), comprehensions (list, dict, set, generator), lambdas, walrus assignment (`:=`), triple-quoted multi-line strings (`"""..."""`, `'''...'''`) and string prefixes (`r`, `b`, `f`, `u`, `rb`, `fr`), numeric literals (binary, octal, hex, floats, scientific exponents, imaginary numbers `j`, underscores), a 14-level Pratt operator precedence table (supporting assignment and augmented assignment, conditional ternary `x if cond else y` via `middleTerminator`, boolean operators, comparisons, bitwise shifts and bitwise logic, arithmetic, unary operators, exponentiation `**`), and synchronization-token recovery. The legacy v1 Python scanner is preserved as `definitions/python_legacy_definition.json` for legacy test suite compatibility. `tests/unittests/python_v2_grammar_tests.cpp` verifies both static header and dynamic JSON definition pipelines.

`definitions/html_definition.json` provides a compiler-grade HTML5 schema-v2 grammar with six stack-based lexer modes (`default`, `Tag`, `ScriptTag`, `StyleTag`, `ScriptContent`, `StyleContent`) that enforce correct tokenization boundaries for tags, attributes, raw script/style bodies, and top-level document content. The grammar handles all standard HTML5 node types: `DOCTYPE` declarations (case-insensitive, with optional public/system identifiers), HTML comments (`<!-- ... -->`), `CDATA` sections (`<![CDATA[...]]>`), XML processing instructions (`<?...?>`), void elements (`area`, `base`, `br`, `col`, `embed`, `hr`, `img`, `input`, `link`, `meta`, `param`, `source`, `track`, `wbr`) recognised at higher lexer priority to avoid ambiguity with regular open tags, self-closing non-void elements (e.g. Web Component `<my-widget/>`), paired elements with nested child content, raw `<script>` and `<style>` elements whose bodies are captured as opaque `Script_Body`/`Style_Body` tokens preventing any inner markup from being tokenized. Attributes support standard quoted values (`"..."`, `'...'`), unquoted bare-word values, and boolean attributes without values. The `AttributeName` token regex covers framework-style attribute syntaxes: Angular event bindings `(click)="..."`, property bindings `[class.active]="..."`, structural directives `*ngIf` and `*ngFor`, Vue event shorthands `@submit.prevent` and `:href`, template references `#formRef`, and custom-element attributes such as `:user-id` and `data-*`/`aria-*`. Character entity references (`&amp;`, `&#65;`, `&#x1F600;`) are recognized as `Entity` tokens at document level. The `caseSensitivity: false` setting enables case-insensitive regex matching throughout (matching browser HTML parsing behaviour). Error recovery uses `Tag_Start`, `VoidTag_Start`, `ScriptTag_Start`, `StyleTag_Start`, `ClosingTag`, `Doctype`, and `Comment` as synchronization tokens, allowing the parser to skip malformed content and resume at the next recognizable structure. The legacy v1 HTML scanner is preserved as `definitions/html_legacy_definition.json` for legacy tokenization test and validator compatibility. `tests/unittests/html_v2_grammar_tests.cpp` verifies both static header and dynamic JSON definition load paths with corner-case tests covering empty documents, all node types, framework attributes, entities, self-closing and void elements, and syntax-error recovery.


`definitions/css_definition.json` is the primary CSS schema-v2 definition. Its
EBNF grammar builds stylesheet, at-rule, selector, declaration, function, and
balanced component-value nodes. Six stack-based lexer modes separate rule bodies,
parentheses, brackets, and single/double quoted strings. Coverage includes nested
rules and `&`, functional pseudo-selectors, namespace/attribute selectors, custom
properties, keyframe blocks, media/supports/container conditions, layers, font-face
and page rules, escaped identifiers, URLs, numeric dimensions, and comments as
trivia. Declaration recovery synchronizes at semicolons and closing braces;
otherwise-unrecognized characters remain visible as `Invalid` tokens.
`tests/unittests/css_v2_grammar_tests.cpp` checks both generated C definitions and
JSON loading, including complete consumption, malformed input, AST structure, and
mode restoration. This is a structural syntax grammar: property-specific value
semantics and function argument/selector legality are not fully validated.
The existing property/pseudo/at-rule validator and legacy scanner tests continue
to use `definitions/css_legacy_definition.json`, matching the HTML migration.

`definitions/javascript_definition.json` is the primary JavaScript/JSX schema-v2
definition, derived from the shared ECMAScript rules in the TypeScript grammar.
It covers declarations, destructuring/default/rest bindings, functions and arrows,
async/generator syntax, classes (heritage expressions, private fields, static
blocks, methods and accessors), control flow, modules and import attributes,
dynamic imports, `import.meta`, `new.target`, optional chaining, and modern
assignment operators. Contextual lexical goals distinguish regular expressions
from division; stack-based modes handle nested template interpolation and JSX.
The Pratt table provides operator precedence and associativity, and statement
recovery and automatic semicolon insertion preserve subsequent statements.
TypeScript type annotations, declarations, generic parameters/calls, assertions,
and non-null suffixes are excluded regardless of filename. JSX requires a `.jsx`
filename supplied through `textparser_set_filename` before grammar execution.

For the JavaScript grammar API, link `libtextparser_typescript` and call
`textparser_typescript_register_validators(handle)` after opening the input and
before parsing/executing the grammar. The definition deliberately reuses the
existing identifier, assignment/update-target, and source-legality callbacks
and their `TS` diagnostic codes. This is a syntax parser with those shared
legality checks, not a complete ECMAScript semantic validator (for example,
strict-mode rules and module binding resolution are outside its coverage).
As with other V2 definitions, the CLI still exposes normalized tokenization;
structured grammar execution uses `textparser_execute_language_grammar`.
`tests/unittests/javascript_v2_grammar_tests.cpp` exercises compiled and JSON-loaded
definitions, complete consumption, malformed input, recovery, CST categories,
and mode restoration. The old scanner remains in
`definitions/javascript_legacy_definition.json` for tokenization and incremental
scanner regression tests; the obsolete `definitions/schema_v2` draft is removed.

`definitions/go_definition.json` provides a compiler-grade Go (Go 1.0 through Go 1.22+) schema-v2 grammar, featuring package clauses, single and grouped imports (with package aliases, dot imports, and blank imports), constant declarations with iota enumerations, variable declarations with type inference and multiple bindings, type declarations (type definitions, type aliases, and Go 1.18+ generic type parameters and constraints `[T any, K ~int | string]`), struct types (with field tags, embedded fields, and generic arguments), interface types (with method sets and type element sets `~T`), functions and methods (with pointer and value receivers, variadic parameters `...T`, multiple return values, named return values, generics, and bodyless function declarations), control flow statements (`if`/`else` with optional short statement initializers, 3-clause C-style `for`, condition `for`, infinite `for`, and `range` loops), expression switch and type switch (`switch v := x.(type)`), `select` with send/receive channel communication, goroutines (`go func()`), deferred calls (`defer`), labeled statements, break/continue/goto/fallthrough, closures and function literals, composite literals (structs, arrays, slices, maps, nested values, and keyed elements), slice expressions (2-index and 3-index full slices `s[low:high:max]`), type assertions, full literal support (interpreted strings, raw multi-line strings, rune literals and character escapes, hex, octal, binary, floats, exponents, and imaginary numbers), automatic semicolon insertion (`allowASI: true`), condition-specific expressions (`ConditionExpression`) preventing struct-literal brace ambiguities, a 5-level Pratt operator precedence table matching the official Go language specification, and synchronization-token recovery. The legacy v1 Go scanner is preserved as `definitions/go_legacy_definition.json` for legacy test suite compatibility. `tests/unittests/go_v2_grammar_tests.cpp` verifies both static header and dynamic JSON definition pipelines.

`definitions/sql_definition.json` provides a comprehensive, multi-dialect SQL schema-v2 grammar (supporting ANSI SQL, PostgreSQL, MySQL, SQLite, T-SQL, and Oracle idioms). It features case-insensitive keyword and identifier matching (`caseSensitivity: false`), standard whitespace and SQL comments (`--` and `#` line comments, `/* ... */` block comments as trivia), string literals (single-quoted with `''` escape, double-quoted, backtick identifiers, and brackets `[...]`), typed/positional/named parameter variables (`?`, `$1`, `:param`, `@param`), DML statements (`SELECT`, `INSERT`, `UPDATE`, `DELETE`), common table expressions (CTEs via `WITH` and `WITH RECURSIVE`), joins (inner, left/right/full outer, cross, natural joins, and comma joins), set operations (`UNION [ALL]`, `INTERSECT [ALL]`, `EXCEPT [ALL]`, `MINUS`), grouping and windowing (`GROUP BY`, `HAVING`, `ORDER BY` with `ASC`/`DESC` and `NULLS FIRST`/`LAST`, `LIMIT`/`OFFSET`), DDL statements (`CREATE TABLE`, `ALTER TABLE`, `DROP TABLE`, `CREATE/DROP VIEW`, `CREATE/DROP INDEX`, `TRUNCATE TABLE`), transaction management (`BEGIN`, `START TRANSACTION`, `COMMIT`, `ROLLBACK`, `SAVEPOINT`), rich data types, a 7-level Pratt operator precedence table (handling boolean `OR`/`AND`/`NOT`, relational/equality operators, bitwise operators, string concatenation `||`, and arithmetic), full expression and predicate support (`IS [NOT] NULL`, `[NOT] IN (...)`, `[NOT] LIKE`, `[NOT] BETWEEN ... AND ...`, `CASE WHEN ... THEN ... ELSE ... END`, `CAST(... AS ...)`, and `EXISTS (...)`), and synchronization-token diagnostic recovery across statement boundaries at semicolons. The legacy v1 SQL scanner is preserved as `definitions/sql_legacy_definition.json` for legacy test suite compatibility. `tests/unittests/sql_v2_grammar_tests.cpp` verifies both static header and dynamic JSON definition pipelines.

`definitions/swift_definition.json` provides a compiler-grade Swift (Swift 5 through Swift 6) schema-v2 grammar, featuring module and submodule imports (with import kinds `import class`, `import struct`, etc.), declaration attributes (`@objc`, `@discardableResult`, `@main`, `@propertyWrapper`, etc.), constants and variables (`let`, `var`) with type annotations, initializers, and accessor blocks (`get`, `set`, `willSet`, `didSet`), functions and methods (with external argument labels, variadic parameters `...`, default argument values, `async`, `throws`, `rethrows`, and return types `->`), operator overloads (`static func +`), custom operator declarations (`operator infix ...`), classes, structs, actors, protocols (with inheritance, requirements, and associated types), extensions with generic where clauses, enums (with raw values, associated values, and recursive `indirect case`), control flow statements (`if`/`else`, `guard ... else`, 3-clause/condition `while`, `repeat ... while`, `for ... in ... where`, `switch`/`case` with expression and pattern matching), error handling (`do`/`catch ... where`), defer blocks, closures with capture lists (`[weak self, unowned delegate]`) and trailing closures, collections (arrays `[T]`, dictionaries `[K: V]`, empty dictionary `[:]`), key paths (`\Person.name`), multiline strings (`"""..."""`), raw strings (`#"..."#`, `###"..."###`), regex literals (`#/.../#`), a 14-level Pratt operator precedence table matching the Swift standard library precedence groups (assignment, ternary `? :`, nil coalescing `??`, logical OR/AND, comparison/identity `===`, range `...`/`..<`, bitwise shifts and logic, additive, multiplicative, unary prefix/postfix), and synchronization-token recovery at statement boundaries (`recoverSync`). The legacy v1 Swift scanner is preserved as `definitions/swift_legacy_definition.json` for legacy test suite compatibility. `tests/unittests/swift_v2_grammar_tests.cpp` verifies both static header and dynamic JSON definition pipelines.

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
