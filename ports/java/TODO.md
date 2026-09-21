# textparser — Java port TODO

Work still required to reach full parity with the C reference. See
[`BUGS.md`](../../BUGS.md) for the same items framed as known parity gaps, and
[`ARCHITECTURE.md`](../../ARCHITECTURE.md) §3–8 for the authoritative design.

## Parity Status & Observable Scope

* **CLI Parity (100% complete)**: The Java port currently achieves 100% parity with
  the C CLI across all supported languages (matching **238/238** test fixtures in
  `tmp/jsync/walk2.py`).
* Both the C CLI and Java CLI normalize `formatVersion: 2` definitions into a flat
  token stream (see `ARCHITECTURE.md` §9.1 item 8).
* **Where the gap exists**: The V2 declarative grammar engine is not reachable
  through the CLI. It is only observable through programmatic APIs
  (`textparser_execute_language_grammar`, `textparser_execute_production`), grammar
  unit tests (`*_grammar_tests.cpp`), and the `cstdump` CST inspection tool.

---

## Not implemented

- **The V2 declarative grammar engine** (`textparser_execute_language_grammar` /
  `execute_production`): contextual lexer modes/goals, packrat grammar executor,
  Pratt expression parsing, recovery/ASI, CST node views, lifecycle events, and
  TypeScript-specific post-processing.
- **Incremental parsing** (`textparser_parse_incremental` / dirty ranges).
- Native decoders, validators, predicates, and handler registration.

---

## Complexity Assessment for V2 Grammar Engine

Fixing the V2 grammar engine parity gap is a **major undertaking (High to Very High complexity)**:
* **Code size**: Porting roughly **~5,400 lines of complex C code** (~40% of the C core
  in `src/textparser.c` and `src/textparser-json.c`).
* **Estimated Java footprint**: **~3,500–5,000 lines of Java code**, which will
  more than double or triple the current Java codebase (~1,965 lines total).
* **Architecture Subsystems to Implement**:
  1. **Contextual Lexer & Modes** (`src/textparser.c` L7130–7912):
     - Mode stack (`pushMode`/`popMode`) up to depth 64 and goal-oriented lexical rules.
     - Dynamic token triggers and token-to-goal remapping.
     - Priority-ordered token scanning and trivia stream management (`lexer_peek`/`lexer_consume`).
  2. **Packrat Recursive-Descent Grammar Executor** (`src/textparser.c` L7913–10254):
     - 15 production kinds (`sequence`, `choice`, `optional`, `repeat`, `lookahead`,
       `commit`, `terminal`, `non-terminal`, `delimited`, `guard`, `dynamic_infix`, etc.).
     - Multi-dimensional memoization table keyed by
       `(production_id, token_index, context_hash, lexical_goal)` with cache invalidation.
     - Transactional snapshotting (savepoint / checkpoint / commit / rollback) that
       snapshots lexer cursor, mode stack, context variables, diagnostics vector, and tree nodes.
  3. **Pratt Expression Engine** (`src/textparser.c` L10255–10351):
     - Top-down operator precedence parsing for prefix, postfix, infix, and ternary operators.
     - Binding power/precedence levels, left/right associativity, and operand validation.
  4. **Diagnostic Error Recovery & ASI** (`src/textparser.c` L10352–10452):
     - Panic-mode synchronization with recovery tokens (`recover_until_token`).
     - Synthetic token insertion (e.g., Automatic Semicolon Insertion).
     - Furthest-failure tracking and diagnostics vectors.
  5. **JSON Schema V2 Grammar Loader** (`src/textparser-json.c` L1240–1500):
     - Deserialization of `grammar`, `operators`, `lexer.modes`, `lexer.goals`, and `recovery` sections.
     - Graph validation: detecting direct/indirect left recursion, nullable loops, and unresolved production references.

---

## V2 grammar engine phased plan

Suggested execution in 7 distinct phases:

1. **Model + JSON loader** [COMPLETED] — `Production` (15 kinds), `GrammarDefinition`,
   `LexerMode`, `LexerGoal`, `ContextualLexerRule`, `OperatorDef`,
   `MatchResult`, and the CST `Node` fields (`node_flags`, `cst_kind`,
   `source_start/end`, `decoded_value`, `user_data`). Loaded the `lexer`,
   `grammar`, `operators`, and `recovery` sections; resolved names to IDs and
   validated undefined refs / nullable loops / left recursion via 3-color DFS cycle check.
   Covered with comprehensive tests in `GrammarLoaderTest.java`.
2. **Contextual lexer** — mode stack (`pushMode`/`popMode`), lexical goals and
   goal→token remapping, priority-ordered token + trivia scanning,
   `lexer_peek`/`lexer_consume`, line-terminator flags, native predicates.
3. **Grammar executor** — packrat recursive descent over the production kinds;
   memoization keyed by (production, token index, context hash, lexical goal)
   with shift/invalidate on edits; transactional
   checkpoint/commit/rollback snapshotting cursor, mode stack, goal, contexts,
   diagnostics, arena/allocations, pending events.
4. **Pratt engine** — prefix/postfix/infix/ternary roles, precedence and
   associativity, secondary token, operand validation.
5. **Recovery + diagnostics** — panic-mode synchronization, missing-token
   insertion, ASI, furthest-failure diagnostics, and the configured limits.
6. **TypeScript specifics** — CST category classification, token spelling,
   identifier-escape validation, header/modifier/legality checks, and the
   cast/declaration/template disambiguation post-passes.
7. **Public API + integration** — `execute_language_grammar`,
   `execute_production`, `parse_pratt_expression`, `recover_until_token`, CST
   node views, node accessors, decoder/validator/predicate/handler
   registration, lifecycle events, and CLI wiring so `formatVersion: 2`
   definitions run the grammar.

### Verification

Port the C suites (`json_grammar_tests`, `grammar_executor_tests`,
`pratt_precedence_tests`, `speculation_tests`, `diagnostic_recovery_tests`,
`lexer_stream_tests`, `typescript_tests`, `conformance_fixture_tests`) and
compare against the golden fixtures in `tests/docker/fixtures/typescript/` and
`tests/treesitter_compare/cstdump`.
