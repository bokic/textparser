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
- Differential verification against tree-sitter-typescript started
  (`tests/treesitter_compare/`): acceptance parity over the TypeScript fixture
  corpus plus a node-kind / tree-shape comparison for a 28-construct corpus.
  Result: TypeScript grammar coverage is at parity with tree-sitter-typescript
  for the corpus; the remaining CST work is a normalization/mapping layer, not
  pure renaming (the grammar emits TypeScript-compiler AST kinds plus engine
  scaffolding nodes `Repeat`/`Sequence`/`TypeContext`, unconditionally
  materialized type combinators, and operator-named Pratt roots; see
  `tests/treesitter_compare/README.md`). JSX attribute values that are arrow
  expressions, their commit-scoping during tag-kind backtracking, nested
  braces inside JSX expression containers (object literals, arrow block
  bodies), and `return`/`await`/`yield` legality inside flattened `function`
  expression bodies are resolved; no open textparser defects remain.

## 1.3 (Incremental parser: arena compaction) — FUTURE WORK

Status: not implemented. The arena refactor is already in place
(`textparser_arena` with a live `arena` and a per-call `scratch` arena in
`src/textparser.c`); this section describes the remaining memory-reclamation
work.

### Problem

The parser arena (`handle->arena`) is a chunked bump allocator with **no
per-node free**. In-place edits (`change len`, in-leaf resize) and alignment
"keep" cases allocate nothing. Structural edits (add/delete) bump-allocate the
replacement nodes and orphan the old ones, which stay resident until
`free_arena` (only reached by a full parse). Long editing sessions therefore
grow memory by the number of replaced nodes per structural edit. The scratch
arena is already reset at the start of every `textparser_parse_incremental`, so
it does not leak.

### Goal

Reclaim orphaned nodes **in place** (no second arena, no full tree copy) from an
idle editor callback, and make the operation **abortable from another thread**:
if it runs longer than ~1 ms it must be interruptible and leave the tree valid.

### Design

`int textparser_compact(textparser_t handle, const volatile atomic_bool *abort);`

All `textparser_token_item` slots are the same size, so a compacted tree's slots
can be addressed by rank. Compaction moves live nodes toward the front of the
existing chunks.

**Phase A — mark + rank (read-only on the tree; freely abortable)**
1. Traverse the live tree and mark each live slot.
2. Scan arena slots in increasing old-address order; assign live slot `k` the
   new slot `k` and record `old_slot -> new_slot` in a temporary rank map.

Assigning ranks in old-address order guarantees `new_addr(node) <= old_addr(node)`.

**Phase B — move + fixup (destructive; abortable per node)**
3. Iterate live slots in increasing old-address order; for each: copy the node
   to its new slot, rewrite its `prev`/`next`/`child`/`parent` from the rank
   map, and rewrite the inbound references (parent's `child`/`next`, children's
   `parent`, `prev`'s `next`, `next`'s `prev`).
4. Check `*abort` every ~4096 nodes in both phases.

Processing in increasing old-address order means a node's destination is always
a dead slot or an already-moved live slot, never a not-yet-moved live node.
Each move + pointer update leaves the tree fully valid, so aborting mid-Phase-B
is safe: stop, keep the partially compacted (still valid) tree, and retry later.
No rollback or journal is required.

### Memory

One rank entry per live slot (`uint32` suffices below 4G slots) — roughly 4% of
node storage, not a second tree. Allocate it from the scratch arena (reset
before/after) or `malloc`.

### Details and caveats

- **Heap post-processed nodes**: `make_unary_node` / `make_binary_node` /
  template group allocate with `calloc` and mark `text_flags & 0x80000000`
  (`free_post_processed_tokens` frees them). Compaction must copy them into the
  arena, clear that flag, and `free()` the original so the tree becomes
  uniformly arena-backed.
- **Preserve**: `id`, `token_id`, `len`, colors, flags, `decoded_value`,
  `cst_kind`, `category`, `user_data`/`free_user_data`.
- **After success**: reset the bump pointer to the end of the compacted region,
  rebuild lexer streams (`textparser_rebuild_lexer_streams`), and invalidate the
  grammar memo. Lengths are unchanged, so the line map may be kept.
- **Caveat**: compaction invalidates every caller-held `textparser_token_item*`;
  callers must re-fetch from the handle.
- **Scheduling**: call from an idle/timer callback when garbage exceeds some
  fraction of live size. The GTK demo is a candidate consumer.

## 1.4 (Cleanup)

- Code cleanup(deslobification). Cleanup AI slob, old architecture decisions and other.

## 1.5 (Incremental parser: sub-linear edit window) — FUTURE WORK

Status: not implemented. The lexer-snapshot rebuild item is already addressed:
the snapshot buffers are retained and in-leaf edits patch them in place
(`textparser_patch_lexer_streams_leaf`).

### Problem

Every incremental edit is O(document), not O(edit window). The dominant cost is
the reparse: `parent_container` is hard-wired to `nullptr`
(`src/textparser.c:4200`), so `sibling_list = handle->first_item`
(`src/textparser.c:4254`) and `dirty_first = sibling_list`
(`src/textparser.c:4287`). The reparse therefore re-tokenizes from offset 0 to
the container end on every edit. Anchoring at the root was a deliberate
correctness fix (a token's match can depend on text outside its span, e.g. the
JSON `Key` lookahead over its `:` sibling), so it cannot simply be reverted.
This also allocates the per-edit arena garbage tracked in §1.3. Three further
O(document) costs stack on top: `find_token_at_position_internal` (sibling walk
to the edit), the lexer snapshot rebuild for structural/AST edits, and
whole-tree `textparser_post_process` in AST mode.

### Goal

Make an edit cost O(edit window + log n) instead of O(document), for both raw
CSTs and post-processed ASTs, without regressing the correctness the root anchor
buys.

### Design (to be written before implementation)

1. **Bounded reparse window.** Re-introduce a forward resync that stops at the
   first token whose re-lexed shape matches the old tree, so the reparse is
   bounded by the edit rather than the container. Must preserve the
   lookahead-dependent reclassifications (JSON `Key`, CFML `OutputTagPair`, JS
   regex-vs-division) that forced the root anchor.
2. **Persistent order-statistic index over the CST** so the dirty leaf can be
   located and the lexer stream spliced by rank in O(log n) instead of a sibling
   walk.
3. **Position piece-table** (or per-segment bias) so a suffix offset shift is
   O(1) instead of an O(n) pass over the flat snapshot.
4. **Incremental `textparser_post_process`** scoped to the reparse window and
   the disambiguation state it can affect, instead of a whole-tree pass.

Each step needs its own design doc and differential tests before implementation;
the existing incremental differential guards (`incremental_tests.cpp`,
`lexer_stream_tests.cpp`) must stay at 0 mismatches.

