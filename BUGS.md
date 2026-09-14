# Known parity gaps (Java port vs. C golden standard)

This list tracks behavioral differences found while syncing the Java port with
the C engine. Items are removed once fully fixed.

The differential walk over the repository (`tmp/jsync/walk2.py`, 238 files)
matches **238/238** files exactly. The Java parser mirrors C's algorithm
(anchored/line-bounded `findToken`, single-character advance, speculative
rollback, `caseSensitivity`-aware regex flags, `formatVersion: 2` lexer
normalization, literal `+`/`-` sign merging), reports parse failures as a
`ParseError` carrying the same message + `position`/`length` span as the C
accessors, and materializes hidden `Whitespace` leaves in the in-memory CST the
way C does (hidden again at the JSON boundary).

## 1. The `formatVersion: 2` grammar engine is not implemented

Only the CLI-compatible normalization is implemented (see `ARCHITECTURE.md`
§9.1 item 8). The declarative grammar executor, contextual lexer modes/goals,
Pratt expression engine, recovery rules and CST views exposed by
`textparser_execute_language_grammar` / `textparser_execute_production` are not
ported. This is only observable through that API (unit tests and `cstdump`),
not through the CLI.

## C contextual lexer: non-default initial mode differs between peek and consume

With an empty mode stack, `textparser_lexer_peek` uses the definition's
`initial_lexer_mode`, but `textparser_lexer_consume` rescans using
`textparser_get_current_mode`, which returns `default`. A definition whose initial
mode is not named `default` can therefore peek a token successfully and then
abort while consuming it.

Reproduction: load the PHP v2 definition, set its initial mode to `PHP`, parse
`$user->name`, and execute `InterpolatedVariable`. The peek finds `Variable`, but
execution returns `TEXTPARSER_MATCH_ABORT`. Explicitly pushing `PHP` before
execution avoids the mismatch. PHP interpolation validation uses this workaround;
the general core issue remains for a separate task.

## `textparser_parse` ignores v2 lexer trivia priority

`textparser_parse` (the token stream behind the CLI, `ccat`, and the C++
`TextParser` helper) scans the generated legacy-style `tokens` array in token-id
order rather than by the contextual lexer's `priority`. For a v2 definition a
lower-id non-trivia token can therefore shadow a higher-id trivia token:
`LineComment` (id 138) and `BlockComment` lose to `MulOperator` (id 82), so
`// c` and `/* c */` are emitted as operators. The contextual lexer used by
`textparser_execute_language_grammar` honors priority and treats them as trivia
correctly. Pre-existing for TypeScript and now visible for PHP after its v2
definition became the default; comment colorization and `textparser --tokens`
are affected, while grammar validation is not.
