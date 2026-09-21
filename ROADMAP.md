# Roadmap

## 1.1 (Compiler-Grade Parser Architecture Overhaul - Breaking Clean Upgrade)

> **Status**: The v2 compiler-grade parser engine (declarative EBNF runtime, Pratt precedence parsing, lexer modes, lexical goals, speculative checkpoints, structured trivia, AST lifecycle, and diagnostic recovery) is implemented in the C core runtime (`src/textparser.c`). TypeScript/TSX, PHP, CFML, JSON, Bash, C, C++, C#, Java, Rust, Python, HTML, and CSS use primary v2 definitions (`definitions/{typescript,php,cfml,json,bash,c,cpp,csharp,java,rust,python,html,css}_definition.json`) with full EBNF grammars, stack-based lexer modes, and error recovery. The legacy scanners for CFML, JSON, Bash, C, C++, C#, Java, Rust, Python, HTML, and CSS are retained as `definitions/{cfml,json,bash,c,cpp,csharp,java,rust,python,html,css}_legacy_definition.json` for the legacy tokenization tests and the core lexer suites.

### Remaining Work:
- **Migrate Remaining Languages to v2 Grammar Schema** (17 remaining; `typescript`, `php`, `cfml`, `json`, `bash`, `c`, `cpp`, `csharp`, `java`, `rust`, `python`, `html`, `css` done):
  - Author real EBNF grammars, stack-based lexer modes (`pushMode`/`popMode`), contextual lexical goals, and Pratt operator precedence tables for other languages (ada, asm, c3, fortran, go, jai, javascript, matlab, md, pascal, perl, r, scratch, sql, swift, vb, zig).
  - Define language-specific error recovery synchronization tokens (`recoverSync`) and native validators.
- **Eliminate Legacy v1 Dual-Stack Burden**:
  - Complete the clean, breaking upgrade replacing legacy v1 definitions with schema v2 entirely.
  - Remove legacy v1 scanner generation paths (`src/search_function_gen.c`) once all languages are migrated.




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
