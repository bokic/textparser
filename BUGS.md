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

(No known open issues).
