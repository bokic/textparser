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

(No known open Java parity issues).

## Compiler fixture verification

- `tests/c3_compare/verify_fixtures.py` stops extracting a `sources[]` array at
  the first `};`, including when those characters occur inside a C++ fixture
  string. Later entries can therefore be omitted from compiler verification.
  The Zig comparator uses a string-aware array pattern; the existing C3 script
  still needs the equivalent fix and a rerun of its reference checks.
