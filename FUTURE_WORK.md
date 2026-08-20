
# FUTURE WORK & ARCHITECTURAL ROADMAP

---

## 1. Concrete Syntax Tree (CST) & Full Token Retention [COMPLETED]

### Motivation & Goals
Transform `textparser` into a 100% lossless Concrete Syntax Tree (CST) engine that records every token, delimiter, punctuation mark, and leading trivia/padding so that the source code can be reproduced byte-for-byte from the tree.

* **Syntax Trivia & Unprocessed Node Retention** *(Implemented)*:
  * Preserves delimiters, punctuation, whitespace, and plain unparsed text as explicit **Unprocessed** token nodes (`TEXTPARSER_TOKEN_ID_UNPROCESSED` = -2), ensuring gapless, byte-for-byte fidelity ($\sum \text{token.len} == \text{document\_length}$).
* **Relative Sizing (`len` only) & Dynamic Position** *(Implemented)*:
  * Stores only relative node length (`len`) inside token structures; removed absolute `position` field to prevent $O(N)$ position-invalidation cascades during incremental parsing.
  * Computes absolute offsets on demand during traversal via `textparser_get_token_position(token)`.
* **Tree-sitter Compatible Token Schema**: Align token types and node names with standard Tree-sitter conventions (`primitive_type`, `type_identifier`, `identifier`, `compound_statement`, `parameter_list`, etc.) for seamless editor theme and query compatibility.

---

## 2. Pratt Parsing for Mathematical & Logical Expression Trees

### Motivation & Goals
Replace flat token sequences in mathematical and logical expressions with exact binary/unary expression trees using **Pratt Parsing (Top-Down Operator Precedence)**.

### Key Enhancements
* **Dynamic Precedence Configurations**:
  * Define `operator_precedence` tables in JSON grammars.
  * Map operator table indices directly to numerical binding power weights on internal token structures.
* **Associativity Controls**: Support explicit `left` and `right` associativity (e.g., left-associative exponentiation `^` in Adobe ColdFusion).
* **Unary vs. Binary Operator Disambiguation**: Differentiate prefix operators (unary `+`/`-`, logical `NOT`/`!`) from infix/postfix operators based on stream position.

---

## 3. Structural Statement Recognition & Speculative Backtracking

### Motivation & Goals
Recognize high-level constructs (like variable declarations, function definitions, and type casts) without the massive multi-megabyte state tables required by GLR parsers.

### Key Enhancements
* **Composite Declaration Tokens**:
  * Define structural tokens (e.g., `"Declaration"`) in grammar JSON requiring type specifiers, variable identifiers, and terminators (`=`, `;`, `,`).
  * Capture modifiers (`const`, `static`, `unsigned`, etc.) and pointer/array qualifiers within the parent declaration node.
* **Speculative Parsing with Checkpoints**:
  * Parse statements using ordered candidate branches with lightweight cursor checkpoints (e.g., try `Declaration` first; if it fails, backtrack and try `ExpressionStatement`).
  * Keep backtracking local to the statement boundary (typically only a few tokens).

---

## 4. Post-Parsing Contextual Rule Disambiguation

### Motivation & Goals
Resolve lexical ambiguities that cannot be determined by single-pass regex matching alone.

### Key Enhancements
* **Contextual Fixup Rules**:
  * **JavaScript / Rust Regex vs. Division**: Disambiguate `/.../` based on whether the preceding token is an expression operand (division) or an operator/keyword (regular expression literal).
  * **C/C++ Cast vs. Function Call**: Disambiguate `(type)(x)` vs. `(func)(x)` by checking collected type definitions or pointer syntax.
  * **C++ Generics vs. Comparison Operators**: Validate `<...>` template arguments vs. binary `<` / `>` comparisons using balanced bracket and delimiter checks.

---

## 5. Three-Way Native C Regex Bypass (Zero-Dependency Lexing)

### Motivation & Goals
Eliminate `libpcre2` dependency entirely by having LLMs/codegen translate JSON regex patterns into direct native C matching functions.

### Key Enhancements
* **3 Specialized Matcher Functions per Token**:
  1. **`match_latin1` (1-byte Latin-1 / ASCII)**: Direct pointer arithmetic, branchless lookup tables (LUTs), and SIMD byte scans.
  2. **`match_utf8` (Variable-width UTF-8)**: Fast 1-byte ASCII inline path with multi-byte Unicode code point decoding for non-ASCII characters.
  3. **`match_utf16` (2-byte UTF-16 / UCS-2)**: Native 16-bit (`uint16_t`) character scanning for seamless interoperability with Windows, Java, JavaScript, and .NET memory buffers without transcoding.
* **Zero-Cost Dispatch**: Inspect buffer encoding once per parse session and bind the corresponding function pointer array for $O(1)$ dispatch.
* **Automated Differential Fuzzing**: Validate every generated C matcher against PCRE2 reference output across millions of random and edge-case inputs before committing.

---

## 6. High-Speed Token Range Export for Editors (LSP & Highlight Buffers)

### Motivation & Goals
Provide a high-throughput, allocation-free API for editors and language servers to retrieve changed token ranges in a single linear pass.

### Key Enhancements
* **Single-Pass Flat Token Array**:
  * Populate a caller-provided reusable scratch buffer with flat token coordinates: `[start_line, start_col, end_line, end_col, token_type_id]`.
  * Avoid per-token heap allocations during viewport rendering and syntax recoloring.
* **Changed Range Invalidation**: Compute diff ranges between old and new trees quickly via pointer-equality checks on shared subtrees, restricting recoloring queries to modified lines only.
