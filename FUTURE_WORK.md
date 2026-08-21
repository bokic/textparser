
# FUTURE WORK & ARCHITECTURAL ROADMAP

---

## 1. Structural Statement Recognition & Speculative Backtracking

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

## 2. Post-Parsing Contextual Rule Disambiguation

### Motivation & Goals
Resolve lexical ambiguities that cannot be determined by single-pass regex matching alone.

### Key Enhancements
* **Contextual Fixup Rules**:
  * **JavaScript / Rust Regex vs. Division**: Disambiguate `/.../` based on whether the preceding token is an expression operand (division) or an operator/keyword (regular expression literal).
  * **C/C++ Cast vs. Function Call**: Disambiguate `(type)(x)` vs. `(func)(x)` by checking collected type definitions or pointer syntax.
  * **C++ Generics vs. Comparison Operators**: Validate `<...>` template arguments vs. binary `<` / `>` comparisons using balanced bracket and delimiter checks.

---

## 3. Three-Way Native C Regex Bypass (Zero-Dependency Lexing)

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

## 4. High-Speed Token Range Export for Editors (LSP & Highlight Buffers)

### Motivation & Goals
Provide a high-throughput, allocation-free API for editors and language servers to retrieve changed token ranges in a single linear pass.

### Key Enhancements
* **Single-Pass Flat Token Array**:
  * Populate a caller-provided reusable scratch buffer with flat token coordinates: `[start_line, start_col, end_line, end_col, token_type_id]`.
  * Avoid per-token heap allocations during viewport rendering and syntax recoloring.
* **Changed Range Invalidation**: Compute diff ranges between old and new trees quickly via pointer-equality checks on shared subtrees, restricting recoloring queries to modified lines only.

---

## 5. Tree-sitter Compatible Token Schema

### Motivation & Goals
Align token types and node names with standard Tree-sitter conventions (`primitive_type`, `type_identifier`, `identifier`, `compound_statement`, `parameter_list`, etc.) for seamless editor theme and query compatibility.
