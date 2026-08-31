# Roadmap

## 1.1 (Compiler-Grade Parser Architecture Overhaul - Breaking Clean Upgrade)

- **Unified Language Definition Schema (No Legacy v1 Dual-Stack Burden)**:
  - Clean, breaking upgrade replacing legacy v1 schema entirely.
  - One-time migration script to upgrade existing `definitions/*.json` to the new schema.
  - Strict separation of lexer tokens and grammar productions (EBNF declarative constructs: `ref`, `sequence`, `choice`, `optional`, `repeat`, `oneOrMore`, `separatedBy`, `lookahead`, `commit`).
- **Lexer Modes & Lexical Goals**:
  - Stack-based transient lexical modes (`modes`, `pushMode`, `popMode`) for complex embedded languages and template strings (CFML, PHP, Bash, JSX, TSX, Markdown).
  - Lexical goals for contextual token resolution (e.g. `/` regular expression vs. division, `<` JSX/generics vs. relational).
- **Trivia & Line-Terminator Awareness**:
  - Structured trivia preservation (whitespace, line comments, block comments).
  - Line-terminator predicates (`lineTerminatorBefore`, `noLineTerminatorBefore`, etc.) for restricted productions and declarative Automatic Semicolon Insertion (ASI).
- **Speculative Parsing & Checkpoints**:
  - Guarded and prioritized alternatives with commit points.
  - Checkpoint and rollback mechanism restoring tokens, modes, contexts, diagnostics, and arena allocations on branch failure.
- **Robust Error Recovery & Diagnostic Vector**:
  - Multi-diagnostic collection (severity, spans, generic recovery actions).
  - Synchronization tokens, synthetic/missing node insertion, token replacement, and forward-progress guarantees without diagnostic storms.
- **Strict Semantic Action & AST Lifecycle**:
  - Replaced ambiguous callbacks with side-effect-free `VALIDATE`, bottom-up `COMMIT`, `RECOVERY`, and `SOURCE_COMPLETE` lifecycle events.
  - Application-owned node attachments (`user_data`) and stable node IDs for clean CST-to-AST translation in compiler frontends (such as `tsc23`).
- **Native Decoders, Validators & Operator Roles**:
  - Registered decoders and validators for ECMAScript/Unicode identifiers, numeric literal forms, and string/template escapes.
  - Explicit operator roles and precedence tables (prefix, infix, postfix, ternary) replacing token name heuristics.
- **JSON Loader & Generator Parity**:
  - Formal JSON schema validation across both `textparser-json.c` and `json2h.py`.

## 1.2 (Validator/Tree-Sitter compatibility)

- Implement code validator for each computer language.
- Tree-sitter Compatible Token Schema
  - Align token types and node names with standard Tree-sitter conventions (`primitive_type`, `type_identifier`, `identifier`, `compound_statement`, `parameter_list`, etc.) for seamless editor theme and query compatibility.

## 1.3 (Cleanup)

- Code cleanup(deslobification). Cleanup AI slob, old architecture decisions and other.
