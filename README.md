# TextParser [![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/bokic/textparser)

TextParser is a high-performance, extensible text parsing library written in C. It uses regular expressions to define language grammars and generates a hierarchical Abstract Syntax Tree (AST) for parsed documents.

Successful parses publish immutable, handle-owned lexer token and trivia
streams through `textparser_get_lexer_tokens()` and
`textparser_get_lexer_trivia()`. Syntax tokens contain half-open source spans
and indexes into their leading trivia; trivia records whether its span contains
a line terminator. The arrays remain valid until the next parse, text update,
or parser close. Incremental parsing replaces the complete snapshot.

Speculative parsing uses a shared transactional parser state. Checkpoints now
restore source/token cursors, the complete mode stack and lexical goal, scoped
contexts, diagnostics, pending-event depth, speculation/recovery depth, node
IDs, and arena allocation watermarks. `textparser_get_parser_state()` exposes a
read-only state view for diagnostics and parser integration.

The executable grammar core supports manually constructed `TOKEN`, `REF`,
`SEQUENCE`, `CHOICE`, `OPTIONAL`, zero-or-more `REPEAT`, `LOOKAHEAD`, `NOT`,
`PREDICATE`, scoped `CONTEXT`, and `COMMIT` productions through
`textparser_execute_production()`. Productions consume the immutable lexer
stream, return a uniform `textparser_match_result`, roll back rejected branches,
bound recursive references, and reject zero-width repeat loops. Lookahead is
always rolled back, parser-aware predicates receive current/previous tokens and
the preceding-line-terminator state, contexts are restored after their child,
and a committed failure prevents a choice from trying later alternatives.

JSON language definitions can load those production kinds from
`grammar.productions`. Nested constructs are flattened into an owned runtime
table, token and production names are resolved to IDs, and
`textparser_execute_language_grammar()` runs the configured start production.
Loading rejects malformed constructs, missing names, undefined references,
nullable repeats, and recursive cycles reachable before consuming a token.
Native predicates use `{"when":{"native":"name"}}`, scoped integer or boolean
contexts use `withContext.set`, scoped contextual scanning uses
`{"withGoal":{"name":"goal","production":...}}`, and commit points use
`{"commit":true}`. Transactional `capture` scopes record the exact source text
matched by one production while its `then` production runs; nested
`matchCapture` constructs reparse a production and require its text to equal
the nearest same-named capture.
Schema-v2 `lexer.tokens` and `lexer.trivia` are normalized into the current
lexer table so their names can be referenced by the grammar.

Schema-v2 grammars use an on-demand contextual scanner. `lexer.initialMode`
selects the initial rule set, `lexer.modes` restricts tokens and trivia,
and token-level `pushMode`/`popMode` transitions are applied only when a token
is consumed. `lexer.goals` maps a normally eligible token rule to a contextual
replacement rule, allowing the same source offset to be scanned differently
for expression starts, continuations, templates, JSX, or type contexts. Scan
results are cached by source offset, mode, and goal; speculative rollback
restores the cursor and mode stack while retaining safe cache entries.
`textparser_lexer_peek()` and `textparser_lexer_consume()` expose this scanner,
and declarative `TOKEN` productions use it automatically for schema-v2 lexer
definitions.

Declarative grammars can parse expressions with a real Pratt production:
`{"pratt":{"primary":...,"postfix":...}}`. The primary and optional postfix
children remain ordinary grammar, so
literals, identifiers, parenthesized expressions, and language-specific atoms
can be composed normally, while calls and member-like suffixes can be folded
around the expression before token operators are considered. A postfix grammar
must consume input and cannot be nullable. Root-level `operators` entries register prefix,
postfix, infix, or ternary roles with explicit precedence and associativity;
ternary entries also name their middle terminator. Pratt parsing consumes the
contextual lexer, constructs nested operator CST nodes, supports the same token
in prefix and infix positions, and transactionally rolls back incomplete
expressions. `textparser_parse_pratt_expression()` executes a Pratt start
production directly when callers need an expression-only entry point.
Operator entries may declare `leftValidator` or `operandValidator` native
hooks. Pratt invokes them transactionally before folding infix/postfix or
prefix nodes, so a language can enforce structural operand restrictions
without turning precedence parsing back into a handwritten expression parser.

The schema-v2 TypeScript definition now provides the first incremental grammar
profile (10a): whitespace and comment trivia, identifiers, numeric and bigint
literals, quoted strings, no-substitution templates, regular-expression
literals, booleans, `null`, `undefined`, parentheses, prefix/postfix operators,
binary and assignment operators, and conditional expressions. Pratt parsing
switches between `ExpressionStart` and `ExpressionContinuation`, so `/.../`
is scanned as a regular-expression literal only where an operand can begin and
as division after an operand. Later increments add JSX/TSX, call/member
expression chains, array/object literals, and substitution templates.

The 10b TypeScript profile adds a reusable `Type` production and a `Type`
lexical goal. It covers keyword, reference and qualified types; nested generic
arguments and parameters; array, indexed-access, tuple and named/rest tuple
types; unions, intersections and conditional types; function and constructor
types; object members, call/construct/index signatures and mapped types; type
operators, literal types, predicates, queries, import types, and `infer`.
Contextual rescanning splits `>>` and `>>>` into individual closing angle
brackets while parsing nested generic types. Type annotations are connected to
declarations, while 10k connects types to expression-level assertions.

The 10c profile makes `SourceFile` the TypeScript grammar entry point and adds
blocks, expression and empty statements, `if`/`else`, `while`, `do`, classic
`for`, `for-in`, `for-of`, `continue`, `break`, `return`, `throw`, labels,
`switch`, `try`/`catch`/`finally`, `with`, and `debugger`. Automatic semicolon
insertion creates synthetic missing semicolon nodes at EOF, before `}`, and
across a line terminator. The built-in
`typescript.noLineTerminatorBefore` predicate enforces restricted productions,
and a line terminator prevents Pratt parsing from consuming postfix `++` or
`--`.

The 10d profile adds variable and parameter declarations, function overloads
and bodies, type aliases, interfaces, enums, namespaces/ambient modules, and
classes. Class syntax includes generic heritage clauses, constructors,
parameter properties, fields and private names, methods/accessors, index
signatures, modifiers, abstract members, and static blocks. Declaration
initializers use an assignment-expression Pratt entry so commas delimit
declarators correctly. Type annotations and generic clauses run under a scoped
`Type` lexical goal and restore expression scanning before initializers and
bodies.

The 10e profile adds static and side-effect imports, default/namespace/named
bindings, type-only imports and exports, `import = require(...)` aliases,
qualified import aliases, import attributes, named and star re-exports,
`export default`, `export =`, and `export as namespace`. The completed profile
also handles `import defer` namespace bindings, `type` as a default or named
binding, arbitrary string import/export names with valid local aliases,
`export import`, default interfaces, exported ambient variables, dynamic
`import(...)`, and exact `import.meta` recognition. Namespace and module bodies
accept the same statement grammar, including nested imports and exports;
ambient string-named modules and `declare global` augmentations are also
supported. Module resolution remains intentionally outside the syntax parser.

The 10f profile adds TSX scanning modes for opening tags, closing tags, JSX
text, and embedded TypeScript expressions. JSX is accepted as a Pratt primary
only when an expression operand can begin, leaving relational `<` unchanged
after an operand. The grammar covers named and member/namespace elements,
fragments, self-closing elements, boolean/string/expression attributes, spread
attributes, nested text and elements, spread children, and empty/comment-only
expression containers. Opening and closing element names are checked with the
transactional capture/equality productions, including nested same-named
captures, and failed TSX alternatives restore the complete lexer-mode stack.

The 10g profile adds grammar-defined Pratt suffixes and uses them for chained
calls, spread arguments, property and element access, optional property/call/
element chains, and TypeScript non-null assertions. `new` expressions preserve
the distinction between member access in the constructor target and calls on
the completed result, including nested `new`. Commit points after suffix
openers turn incomplete constructs into syntax errors without affecting
speculative rollback when no suffix begins. Generic call type arguments and
the remaining primary-expression families are reserved for later increments.

The 10h profile adds array literals with elisions, trailing commas, nested
values, and spread elements. Object literals support shorthand and named
properties, keyword/string/numeric/computed names, spread assignments,
generator/async/generic methods, typed parameters and returns, and getter/setter
accessors. Object-member scanning uses a scoped lexical goal so contextual
`async`, `get`, and `set` retain their structural roles while nested property
values restore the ordinary Pratt expression goals. Literal results participate
in the same call/member/index postfix chains as other primary expressions.

The 10i profile adds named and anonymous function expressions, generators,
async functions, and async generators with generic parameters and TypeScript
parameter/return annotations. Arrow functions support identifier,
parenthesized, generic, and async heads plus concise expression or block
bodies. Scoped function/arrow lexical goals preserve contextual `async` while
nested bodies restore normal expression scanning. Arrow heads are tried
transactionally ahead of parenthesized and JSX primaries, and failed candidates
restore the lexer goal and cursor. Line terminators are forbidden before `=>`
and between an async modifier and its function/arrow head. TSX-only generic
arrow restrictions remain part of the source-profile work in 10n.

The 10j-a profile adds recursive binding names and array/object binding
patterns. Variables, parameters, arrow heads, `for-in`/`for-of` declarations,
and catch bindings support nested destructuring, array elisions, shorthand and
renamed/computed object properties, rest bindings, trailing commas, defaults,
and outer TypeScript annotations. Pattern parsing remains speculative so a
classic `for` header can roll back and select the `for-in` or `for-of` form.

The 10j-b profile attaches `typescript.assignmentTarget` validators to simple
and compound assignments and to prefix/postfix updates. It accepts identifiers,
parenthesized and non-null targets, member/element access (including access on
call results), and recursively validated array/object assignment patterns with
defaults and terminal rest targets. Calls, literals, optional chains, methods,
non-target pattern values, compound destructuring, misplaced rest elements,
and invalid update operands are rejected before Pratt folds the operator node.

The 10k profile adds scanner modes for template heads, middles, tails, nested
substitutions, and nested templates. Template literals and tagged templates
participate in Pratt postfix chains, including generic tagged templates.
Expression type arguments support generic calls and instantiation expressions,
with a follow-token predicate preserving relational expressions such as
`left < right > value`. The same profile adds `as`, `satisfies`, `as const`,
and angle-bracket assertions. Lexer goal remapping now preserves transitions
from the source lexer rule, which allows remapped braces to maintain the
template mode stack while remaining ordinary brace tokens to the grammar.

The 10l profile completes the remaining declaration-oriented syntax increment.
It adds synchronous and `await using` declarations (including destructuring and
`for-of` bindings), class expressions, decorators on classes, members, and
parameters, decorated exports, constructor overload signatures, auto-accessor
fields, and computed class/interface member names. Type parameters accept the
modern `in`, `out`, and `const` modifiers. Class expressions use a scoped
`ClassHead` lexical goal so contextual member modifiers remain structural after
an expression begins. Modifier legality, decorator semantics, ambient-only
overload rules, and version/source-profile restrictions remain later semantic
and profile work rather than permissive syntax concerns.

The 10m profile completes the lexer-oriented increment. TypeScript identifiers
use Unicode `ID_Start`/`ID_Continue`, including validated `\\uXXXX` and
`\\u{...}` escapes, private identifiers, combining marks, ZWNJ, and ZWJ.
Numeric and bigint literals support base prefixes, separators, exponents, and
leading or trailing decimal points. Quoted strings accept escaped line
continuations, templates span lines and retain their substitution-mode
transitions, and a hashbang is accepted only at the beginning of a source file.
Malformed numeric literals, identifier escapes, misplaced hashbangs, and
unterminated strings/templates are consumed as error tokens and publish a
transactional `TS_LEXICAL` diagnostic instead of silently stopping the lexer.

The 10o-b profile publishes TypeScript syntax diagnostics with exact half-open
source spans. Expected punctuation uses `TS1005`, missing expressions use
`TS1109`, invalid assignment targets use `TS2364`, optional-chain assignments
use `TS2779`, and invalid update operands use `TS2357`. Unexpected tokens are
spanned rather than reported as zero-width; EOF failures remain zero-width at
the exact end offset. Furthest-failure selection prefers explicit delimiter
expectations over speculative expression candidates, diagnostics retain
zero-based line/column coordinates, and folded Pratt postfix nodes now expose
their complete left-to-right span.

The first 10o-c early-error increment performs a successful-source CST pass for
control-flow legality. Top-level `return`, unlabeled `break` outside an
iteration or `switch`, and unlabeled `continue` outside an iteration publish
TypeScript `TS1108`, `TS1105`, and `TS1104` diagnostics on the exact keyword
span. Nested functions establish a new control-flow boundary, so an enclosing
loop or switch cannot incorrectly authorize jumps from inside a function.
Class static blocks also reset function and jump contexts.

The 10o-c-b increment adds scoped label resolution for `break` and `continue`,
including chained iteration labels, non-iteration targets, duplicates, missing
targets, and jumps across function boundaries (`TS1107`, `TS1114`, `TS1115`,
and `TS1116`). Labels are compared directly from their source spans and are
scoped to the function that owns them. `yield` is now a first-class expression,
including bare and delegated `yield*` forms. The legality pass tracks the
nearest ordinary, async, or generator function—including methods, accessors,
object members, and arrow variants—and reports `TS1308` or `TS1163` when
`await` or `yield` occurs in the wrong function kind. Top-level `await` remains
accepted because module target and compiler-option legality are outside the
current parser configuration.

The 10o-c-c increment completes syntax-level contextual legality. Modifier
lists diagnose repeated accessibility and repeated individual modifiers
(`TS1028`/`TS1030`); class members reject `readonly` methods, `accessor`
methods, and `async` fields. Getter/setter checks enforce parameter counts,
type-parameter restrictions, and setter return-type restrictions. Declaration
files and explicit ambient declarations reject initializers, implementations,
accessors, executable statements, nested `declare`, and ambient `async` using
the corresponding TypeScript diagnostics. Valid top-level `.d.ts` declarations
remain accepted. Rules requiring symbol resolution, base-class knowledge,
compiler targets, or compiler options remain outside the syntax parser.

The 10n profile selects contextual source behavior from the parser filename.
`.ts`, `.mts`, and `.cts` use TypeScript syntax; `.tsx` additionally enables
JSX while disabling angle-bracket assertions; `.d.ts`, `.d.mts`, and `.d.cts`
use the non-JSX declaration-file profile; `.js`, `.mjs`, and `.cjs` disable
TypeScript-only type syntax; and `.jsx` combines JavaScript syntax with JSX.
The JSX scanner is therefore entered only for JSX-capable files. TSX generic
arrows require an unambiguous trailing comma, multiple parameters, or an
`extends` constraint, while the equivalent single-parameter form remains valid
in `.ts`. Source-profile predicates participate in normal speculative rollback,
so failed JSX, assertion, and generic-arrow alternatives restore lexer state.
Ambient declaration legality and profile-specific explanatory diagnostics are
deferred to the semantic/diagnostic work in 10o.

The 10o-a CST layer gives every materialized schema-v2 production and terminal
a stable string kind independent of generated numeric token IDs. Grammar nodes
also carry explicit half-open source spans; missing tokens retain exact
zero-width insertion positions, recovered nodes retain their skipped span, and
Pratt operator roots cover the complete expression. `textparser_get_cst_node_view`
returns kind, span, flags, and terminal status without exposing parser internals.
`textparser_typescript_cst_category_of` groups TypeScript nodes into source-file,
declaration, statement, expression, type, JSX, binding-pattern, token, and other
families. Existing parent/child/sibling accessors remain the traversal contract,
and the legacy `token_id` representation remains available for compatibility.

Grammar productions can now recover without losing source fidelity. Token
constructs with `allowASI` insert zero-width nodes flagged `MISSING` and
`SYNTHETIC` only at EOF, after a line terminator, or before a configured
synchronization token. `recover.insert` provides explicit missing-token
insertion, while `recoverUntil` or `recover.skip` plus `recover.synchronize`
skips a bounded number of unexpected tokens and returns a `RECOVERED` wrapper
whose children retain the skipped tokens. Failed alternatives retain only the
furthest expected-production diagnostic; checkpoint rollback discards
diagnostics and recovery attempts from abandoned speculation. Root `recovery`
settings bound diagnostics, skipped tokens, and recovery attempts.

The 10o-d TypeScript profile applies those recovery primitives at list
boundaries. Source statements synchronize at semicolons and closing braces,
class members at semicolons and closing braces, object-type/interface members
at semicolons, commas, and closing braces, and switch clauses at the next
`case`, `default`, or closing brace. Each successful skip preserves its tokens
under a `RECOVERED` CST node, emits the context-specific `TS1128`, `TS1068`,
`TS1131`, or `TS1130` diagnostic, and allows later list elements to parse.
Recovery is transactional and only commits when it reaches a usable resumption
point; terminal malformed input and a final semicolon with no following token
retain the original strict failure and furthest syntax diagnostic. The profile
limits a parse to 100 diagnostics, 100 recovery attempts, and 256 skipped
tokens per recovery operation.

The 10p differential conformance corpus locks the C engine as the golden
standard. A fixture corpus under
`tests/docker/fixtures/typescript/{valid,invalid}` exercises the TypeScript
profiles: valid fixtures must parse to completion, emit zero error
diagnostics, and reproduce a byte-for-byte canonical CST stored under
`tests/docker/fixtures/typescript/golden`; invalid fixtures must reproduce the
recorded match status and exact diagnostic list. Golden files are regenerated
from the C engine with `tests/docker/fixtures/regenerate_golden.sh` (or
`TEXTPARSER_REGENERATE_GOLDEN=1 ./bin/unittests
--gtest_filter='TypeScriptFixtureConformance.*'`). Regressing the C engine on
the fixed bug corpus adds a `TypeScriptLegalityRegression` suite. Bugs fixed
while wiring the corpus: for-loops (classic/for-in/for-of) now emit a named
`ForStatement` node so `break`/`continue` legality sees an iteration boundary;
a module-level or class-member `declare` no longer leaks ambient context into
sibling statements or members; a switch `StatementList` no longer recovers
across a `case`/`default` boundary (multi-clause and empty fall-through cases
parse); parenthesized contextual-keyword parameters such as `(async) => async`
parse as arrows; and a semicolon inserted by automatic-semicolon-insertion
across a line terminator is silent instead of reporting `TS1005`. The remaining
compound-assignment operators (`**=`, `%=`, `<<=`, `>>=`, `>>>=`, `|=`, `&=`,
`^=`, `??=`, `||=`, `&&=`) and namespaced JSX attributes (`<tag ns:attr="x" />`)
are implemented and covered by fixtures and corner-case tests; template
literal types with `${Type}` substitutions (`type T = \`user:${string}\``) now
parse, including nested, union, mapped, and conditional positions. Each
`BUGS.md` entry is validated against the real `tsc` compiler so only genuine
TypeScript gaps are tracked; the file currently holds the `accessor`+`readonly`
`TS1243` legality gap and the JSX-arrow-attribute acceptance gap, both found by
the tree-sitter differential verification below.

A 10q differential harness (`tests/treesitter_compare/`) verifies the
TypeScript grammar against the reference **tree-sitter-typescript** grammar
(typescript + tsx) over the conformance fixture corpus and a 26-construct
corpus. Acceptance is at parity with tree-sitter for the corpus: the invalid
fixtures are rejected by both parsers, and the only valid-fixture divergences
are a tree-sitter-typescript gap (`import defer`, valid in tsc/textparser) and
a textparser over-acceptance (`override readonly accessor x`, rejected by tsc
with `TS1243`). The node-kind/shape comparison shows the grammar emits
TypeScript-compiler AST kinds plus engine scaffolding nodes
(`Repeat`/`Sequence`/`TypeContext`), unconditionally materialized type
combinators (`ConditionalType`/`UnionType`/`IntersectionType`/`PostfixType`
around a single primitive), operator-named Pratt roots (`Plus`, `Assign`, ...),
and keyword/operator leaves - a normalization/mapping layer, not a pure
rename, is therefore the remaining ROADMAP-1.2 alignment work.


Declarative production lifecycle events are transactional. `onValidate` runs
immediately after a node is parsed and may accept, reject, or abort it;
`onRecovery` and bottom-up `onCommit` events are queued until the complete start
production succeeds, and checkpoints discard events from rejected alternatives.
`grammar.events.onSourceComplete` runs only when no significant token remains.
An event binding is either a handler-name string or
`{"handler":"name","configuration":...}`; object configuration is delivered
as a stable compact JSON string in `textparser_event.configuration`. Pending
events are cleared after publication and callback rejection or abort is exposed
through the grammar match status.

For C, post-processing uses declaration context in function parameter lists to
classify identifier-shaped types imported through headers. For example, in
`adv_regex_context *ctx`, `adv_regex_context` becomes `TypeName` while `ctx`
remains `Variable`. This inference does not preprocess headers or construct a
complete C typedef symbol table.
The `ccat` utility applies this post-processing before exporting highlighted
token ranges.

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
