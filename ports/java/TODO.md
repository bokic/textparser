# textparser — Java port TODO

Work still required to reach full parity with the C reference. See
[`BUGS.md`](../../BUGS.md) for the same items framed as known parity gaps, and
[`ARCHITECTURE.md`](../../ARCHITECTURE.md) §3–8 for the authoritative design.

## Not implemented

- **The V2 declarative grammar engine** (`textparser_execute_language_grammar` /
  `execute_production`): contextual lexer modes/goals, the packrat grammar
  executor, Pratt expression parsing, recovery/ASI, CST node views, lifecycle
  events, and the TypeScript-specific post-processing. A v2 definition is
  therefore parsed as a flat token stream, not a grammar tree.
- **Incremental parsing** (`textparser_parse_incremental` / dirty ranges).
- Native decoders, validators, predicates, and handler registration.

## V2 grammar engine plan

Roughly ~5,400 lines of C to port (~40% of the C core). Suggested phase order:

1. **Model + JSON loader** — `Production` (15 kinds), `GrammarDefinition`,
   `LexerMode`, `LexerGoal`, `ContextualLexerRule`, `OperatorDef`,
   `MatchResult`, and the CST `Node` fields (`node_flags`, `cst_kind`,
   `source_start/end`, `decoded_value`, `user_data`). Load the `lexer`,
   `grammar`, `operators`, and `recovery` sections; resolve names to IDs and
   validate undefined refs / nullable loops / left recursion.
2. **Contextual lexer** — mode stack (`pushMode`/`popMode`), lexical goals and
   goal→token remapping, priority-ordered token + trivia scanning,
   `lexer_peek`/`lexer_consume`, line-terminator flags, native predicates.
3. **Grammar executor** — packrat recursive descent over the production kinds;
   memoization keyed by (production, token index, context hash, lexical goal)
   with shift/invalidate on edits; transactional
   checkpoint/commit/rollback snapshotting cursor, mode stack, goal, contexts,
   diagnostics, arena, pending events.
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
