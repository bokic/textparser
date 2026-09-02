# tree-sitter-typescript differential verification

Differential CST verification of the textparser TypeScript grammar
(`definitions/typescript_definition.json`, source profiles 10a..10p) against the
reference **tree-sitter-typescript** grammar (`typescript` + `tsx` languages).
It answers, for a fixed corpus of TypeScript sources:

1. **Acceptance parity** - does each parser accept/reject the same files?
2. **Node kinds & tree shape** - how do the produced CST node kinds and their
   nesting relate, and where do they diverge?

This is the working tooling for ROADMAP 1.2 (Tree-sitter compatible token/node
schema) and complements the `tsc`-validated conformance corpus under
`tests/docker/fixtures/typescript` (10p).

## References

| Engine | Version / commit |
|---|---|
| textparser (C golden standard) | repo `git log -1` |
| tree-sitter-typescript grammar | `git -C build/_deps/tree-sitter-typescript log -1` (cloned at first build) |
| tree-sitter runtime | bundled in `build/_deps/treesitter-src` (ABI 14) |
| TypeScript reference | `tsc 7.0.2` (`npm i -D typescript@7` + `tsc --noEmit`) |

## Build & run

Requires the textparser project already built (`./build.sh` at the repo root),
which produces `bin/libtree-sitter.a` (used for the benchmark suite) and the
runtime headers under `build/_deps/`.

```sh
# builds ./dump (tree-sitter-typescript CST dumper) and ./cstdump
# (textparser TS-grammar CST dumper); fetches the grammar on first run into
# build/_deps/tree-sitter-typescript
bash tests/treesitter_compare/build.sh

# regenerates the two reports below + per-file raw CSTs under work/
python3 tests/treesitter_compare/compare.py
```

Generated artifacts:

* `fixtures_report.md` - parity matrix over
  `tests/docker/fixtures/typescript/{valid,invalid}` (17 files) plus a
  node-kind census for each parser (committed for reference, regenerable).
* `constructs_report.md` - 26 single-construct sources with the full CST of
  both parsers side by side (committed for reference, regenerable).
* `summary.json` - kind-count summary.
* `work/*.ts`, `work/*.tp.txt`, `work/*.ts.txt` - per-file raw outputs
  (generated locally, gitignored).

Both CST dumpers emit only the *structure* (kinds/nesting); terminal token text
is intentionally not compared.

## Results

### Acceptance parity

* All 5 invalid fixtures under `tests/docker/fixtures/typescript/invalid` are
  rejected by both parsers.
* 10 of 12 valid fixtures parse cleanly under both.
* Two committed *valid* fixtures diverge. Both were checked against real
  `tsc 7.0.2`:

  1. `declarations_classes.ts` contains
     `override readonly accessor autoAccessor = 2;`. `tsc` rejects it with
     **TS1243** (`'accessor' modifier cannot be used with 'readonly'
     modifier`) and tree-sitter-typescript also emits an ERROR node - so this
     is a **textparser over-acceptance**: the grammar accepts a modifier
     combination that both references reject (missing TS1243 legality
     diagnostic; see `BUGS.md`). The fixture/golden pair must move that line to
     the invalid corpus once TS1243 lands.
  2. `modules_imports.ts` contains `import defer * as ns from "./deferred";`.
     `tsc 7.0.2` reports only the semantic module-not-found error (syntax is
     fine) and textparser accepts it; current tree-sitter-typescript cannot
     parse `import defer`, so this is a **tree-sitter grammar lag**, not a
     textparser bug.

* The 26-construct corpus (`constructs_report.md`) is at parity for all
  constructs except the same two lines above plus one genuine textparser gap:
  **an arrow function used directly as a JSX/TSX attribute value**,
  e.g. `<div onClick={() => go()} />`, fails in textparser with `TS1005`
  (')' expected at the closing brace). `tsc` and tree-sitter both accept it.
  See `BUGS.md`.

### Node kinds & tree shape

The two engines do not use one naming convention: textparser produces
**TypeScript-compiler-style AST kinds** (`SourceFile`, `VariableStatement`,
`FunctionDeclaration`, `TypeReference`, keyword terminals as `ConstKeyword`,
...) while tree-sitter produces **snake_case parse-tree kinds**
(`program`, `lexical_declaration`, `function_declaration`, `predefined_type`,
...). Across the fixture corpus the engines materialize 281 vs 305 distinct
kinds respectively. A semantic equivalence therefore needs a
mapping/normalisation layer (ROADMAP 1.2), and the observed concrete gaps are:

1. **Engine scaffolding nodes** have no tree-sitter counterpart and show up as
   CST kinds: `Repeat`, `Sequence`, `TypeContext`, `Capture`,
   `BasePrimaryExpression`, `PostfixExpressionSuffix`, `BindingParameterList`
   wrappers, `TypeSuffix` chains. tree-sitter either uses one node per
   grammar rule (`formal_parameters`, `parenthesized_expression`,
   `member_expression`/`call_expression`/`subscript_expression`) or flattens
   lists (its separators are anonymous tokens, not wrapper nodes).
2. **Every operand/type carries combinator layers even when there is only one
   alternative.** A bare `: number` annotation materialises
   `ConditionalType > UnionType > IntersectionType > PostfixType >
   NumberKeyword`; tree-sitter and tsc only create `type_annotation >
   predefined_type`. Union/intersection/conditional/postfix combinators are
   emitted unconditionally instead of only for real 2+ member unions, `&`
   intersections, `? :` conditionals or `[]` suffixes.
3. **Operator nodes are named after the operator token.** Binary/assignment
   expressions fold into a node whose kind is the operator kind (`Plus`,
   `Assign`, `GreaterThan`, `LogicalAnd`...) with operands as children, while
   tree-sitter has a single `binary_expression`/`assignment_expression` kind
   and keeps the operator as a leaf token. This is the largest single
   divergence for editor/query compatibility.
4. **Token modelling.** textparser promotes almost every keyword, operator and
   punctuation token to a first-class CST leaf node (`ConstKeyword`, `Colon`,
   `Semicolon`, `Assign`, `LParen`...). tree-sitter keeps keywords/operators/
   punctuation as anonymous tokens (not queryable node kinds) and only names
   semantic terminals (`identifier`, `number`, `string`...). Conversely
   tree-sitter subdivides identifiers
   (`identifier`, `type_identifier`, `property_identifier`,
   `shorthand_property_identifier`, `private_property_identifier`,
   `statement_identifier`) that textparser mostly collapses to `Identifier`.
5. **Container conventions.** Braced statements/class/interface bodies appear
   as `BlockStatement`/`StatementList`, `ClassBody`, `InterfaceBody` with
   explicit `LBrace`/`RBrace` leaf nodes (tree-sitter: `statement_block`,
   `class_body`, `interface_body`, `{`/`}` anonymous). Parens similarly become
   explicit leaf nodes inside `BindingParameterList`, `Arguments`, and
   expression primaries (tree-sitter: `parenthesized_expression`,
   `formal_parameters`, `arguments`).
6. **`.d.ts` handling.** tree-sitter treats ambient declarations via its own
   `ambient_declaration` wrapper under `program`; textparser models the
   declarations directly (`DeclaredVariableStatement`, `declare` keyword
   leaves) with no ambient wrapper node, so whole-file subtree shapes differ.

Where the mapping is applied, statement/declaration/expression nesting is
otherwise consistent: e.g. a function declaration is
`FunctionDeclaration(Identifier, BindingParameterList, TypeAnnotation,
BlockStatement)` on one side and `function_declaration(identifier,
formal_parameters, type_annotation, statement_block)` on the other, with the
same child order and semantics.

See `constructs_report.md` for the full per-construct evidence and
`fixtures_report.md` for the corpus parity/census.
