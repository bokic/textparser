
# FUTURE WORK & ARCHITECTURAL ROADMAP

---

## 1. Three-Way Native C Regex Bypass (Zero-Dependency Lexing)

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

## 2. Tree-sitter Compatible Token Schema

### Motivation & Goals
Align token types and node names with standard Tree-sitter conventions (`primitive_type`, `type_identifier`, `identifier`, `compound_statement`, `parameter_list`, etc.) for seamless editor theme and query compatibility.
