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

## C engine issues (static/dynamic analysis, 2026-11-18)

Found with ASan/UBSan/LSan, cppcheck, clang-tidy and manual review. Repro
harnesses live in `tmp/analysis/repro/` (`*.c`, build against `bin/`).
"Confirmed" = reproduced; "inspection" = from reading the code only.

### Critical

- **Grammar memo use-after-free (confirmed, `memo.c`).** `handle->grammar_memo`
  is only cleared in `textparser_close` and invalidated on the legacy
  incremental path. The v2 path (`textparser_parse_contextual`, used by every
  bundled language) frees the arena but keeps memo entries whose
  `result.node` points into it. `textparser_execute_production` never resets
  the memo either. Sequence parse -> `textparser_execute_language_grammar` ->
  `textparser_parse_incremental` (or `textparser_parse`) -> execute grammar
  again triggers heap-use-after-free at `textparser.c:10341`. Even without the
  UAF, hits are keyed by `token_index` only, so stale results and
  `source_length` are reused after any edit. The GTK4 demo follows exactly this
  pattern; JSON is spared only because its productions are not memoizable.

### High

- **Quadratic v2 lexing (confirmed, `perf.c`).** `textparser_contextual_scan_one`
  linearly walks the `handle->lexer_cache` linked list before every scan; during
  a forward parse the lookup always misses, so tokenizing is O(tokens^2).
  Release build: 160 KB JSON array = 117 s, vs 0.12 s with the lookup disabled
  (80 KB = 14.4 s vs 0.064 s).
- **Quadratic grammar execution (confirmed, `gperf.c`).** `textparser_memo_lookup`
  is a linear scan of one linked list holding every memoized result. 8000
  `var vN = N;` statements: 56 s; with memo disabled time is linear. Fix both
  this and the lexer cache with a hash keyed by the lookup fields (don't just
  remove the caches: without the lexer cache grammar re-lexes heavily).
- **One invalid UTF-8 byte disables lexing of the whole document (confirmed,
  `badbyte.c`, `trunc.c`).** The subject passed to PCRE2 always extends to EOF,
  and PCRE2 validates the entire subject, so every match before a bad byte
  fails and the whole file becomes `Unprocessed`. Common for legacy CFML files
  with stray Latin-1 bytes. Candidate fix: compile with `PCRE2_MATCH_INVALID_UTF`
  (PCRE2 >= 10.34) or bound the subject to the valid prefix.
- **Stale thread-local `PCRE2_NO_UTF_CHECK` (confirmed, `utf.c`).**
  `adv_regex_thread_utf8_valid` is set by the last parse on the thread and then
  applied to unrelated subjects, e.g. caller text passed to
  `textparser_regex_match_pattern`, or a handle whose text was never validated.
  Invalid UTF-8 matched under NO_UTF_CHECK is undefined behaviour in PCRE2.
  Same call: no match before an unrelated parse, match after.
- **`textparser_post_process(&root)` desyncs `handle->first_item` (confirmed,
  `pp.c`, LSan in `IncrementalParsing.ProcessedAstIncrementalMatchesFullParse`).**
  The documented usage (`root = textparser_get_first_token(h);
  textparser_post_process(&root, def)`, also `ccat/main.c:221`) cannot update
  the handle. When the top-level head gets wrapped (e.g. CFML `1 + 2 * 3`),
  the handle keeps a stale interior node: `textparser_get_first_token` returns
  the wrong node, `textparser_close` leaks the synthesized root, and incremental
  parsing operates on the wrong tree. Needs a handle-level API.

### Medium

- **`out_range` never written for v2 definitions (confirmed by inspection of the
  only write sites, 4348/4916).** `textparser_parse_incremental` returns from
  the v2 branch (4304) without setting the documented dirty range, so callers
  read uninitialized data.
- **Stale line map after v2 incremental edits (confirmed, `lines.c`).** The v2
  branch returns before `handle->lines` is invalidated; `textparser_export_tokens*`
  then reports wrong line/col (e.g. pos 3 -> line 0 col 3, should be line 2 col 0).
- **Unprocessed fallback can extend past EOF (confirmed, `trunc.c`).**
  `textparser_parse_contextual` uses unclamped `textparser_char_len()`; text
  ending in a truncated UTF-8 lead byte yields a token with `pos+len > text_size`
  (`[1]\xE2` -> last token len 3 at pos 3 in a 4-byte buffer). Legacy parser has
  the same pattern.
- **`textparser_state_new` is O(n^2) (inspection).** It calls
  `textparser_get_token_position` (walks all previous siblings and ancestors) for
  every token. `find_token_at_position_internal` also recomputes positions
  per level.
- **Generated search functions leak per thread and ignore `is_caseless` (inspection,
  LSan).** `search_function_gen.c` keeps a `_Thread_local` regex context and
  compiled pattern per token that is never freed (~8.5 MB reported by LSan, grows
  with thread churn). The cache is keyed by encoding only, so the first call's
  caselessness is baked in.

### Low (legacy recursive parser / robustness)

The recursive `parse_token_*` path only runs for definitions without
`lexer_rules`; all bundled definitions are v2, so these affect third-party
definitions only.

- `textparser_parse_token`: when `recursion_depth >= MAX_RECURSION_DEPTH` it
  jumps to `exit:` which decrements a counter it never incremented. The counter
  (`int`, never reset between parses) drifts down, loosening the stack guard.
- The legacy packrat memo also caches transient failures ("Maximum recursion
  depth exceeded!", "Parse complexity limit exceeded!") that depend on parser
  state rather than text. Its key `(token, offset, parent_token_id)` also
  ignores the ancestor chain used by `context_nested_tokens` and the previous
  sibling used by regex disambiguation inside groups. And a memo hit returns
  NULL where the first attempt returned a partial `error_child`, so error
  recovery differs between the first and later attempts.
- Callback `TYPE_END` is skipped on several `goto exit` success paths
  (group ending at EOF or on unrecognized token; START_OPT_STOP reaching EOF or
  missing end), while `TYPE_START` was already delivered for start/stop tokens.
- `parse_token_group_all_children_in_same_order` uses `token_def->nested_tokens`
  and the language-level `definition->other_text_inside`, while `find_token`
  uses the effective (context) nested tokens and other parsers use the
  token-level flag.
- `delete_if_only_one_child` returns `ret->child` even when that first child is
  trivia (whitespace/unprocessed) preceding the single semantic child.
- `arena_alloc`: after a checkpoint rollback it advances into the next existing
  chunk without checking that chunk can hold `size` (oversized requests get a
  dedicated, exactly-sized chunk). Latent: current callers only allocate small
  fixed sizes from the main arena.
- `adv_regex_find_pattern_impl` accepts zero-length matches at a non-zero
  offset when `rc == 1` (`ov[1] > 0`), while the capture path rejects empty
  matches; `rc == 0` (more than 15 groups) is silently treated as no match.
  `load_pcre2_dyn` leaks the dl handle when a symbol is missing.
- `json_parse_token_id_array` (17 uses: legacy nested/start tokens, sign-merge,
  regex disambiguation, operators) maps unknown names to `TextParser_END`, which
  silently truncates the END-terminated list at the typo. Lexer modes are
  validated correctly.
- `textparser_hash_contexts` uses a 32-bit hash without a full compare in the
  memo key; a collision returns another context's result. Its doc comment says
  64-bit.
- `make_unary_node`/`make_binary_node` return `op_token` on calloc failure,
  dropping operands from the tree.
- `textparser_parse_contextual` ignores `push_mode`/`pop_mode` failures (unknown
  mode or stack overflow), and treats `scan_one` errors (`rc < 0`) as one
  unlexable character, silently desyncing modes.
