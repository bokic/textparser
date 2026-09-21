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

## Schema/header generator mismatch

`schema/textparser-schema.json` advertises `oneOrMore` as a grammar construct,
but `definitions/json2h.py` rejects it with `Construct must have exactly one core
key, found []`. Found during the CSS V2 migration. CSS uses the equivalent
`sequence: [item, {repeat: item}]` form; the shared schema/generator mismatch
remains to be fixed separately.

## V2 `astKind` metadata is ignored

The JSON loader and header generator accept `astKind`, but neither transfers it
to the compiled production; emitted CST kinds use production names. Found during
the JavaScript V2 migration when an `InitializedVariableDeclaration` production
with `astKind: "VariableDeclaration"` still emitted its original name in both
loading paths. The JavaScript definition uses explicit production names instead;
implementing or removing the advertised metadata is separate engine work.
