# Incremental, Non-Recursive Syntax Parser Design (`textparser`)

This document defines the architectural specification to transition `textparser` from a recursive, full-document parser to an iterative, incremental syntax highlighter.

---

## 1. Core Architecture & State Representation

### Recursive vs. Iterative State
* **Legacy (Recursive)**: Relies on the C call stack. The state of active/nested rules is implicitly managed by recursive function frames.
* **Proposed (Iterative)**: Replaces the call stack with a manually managed **contiguous dynamic array (flat stack)** of active parser scopes.
  ```c
  typedef struct {
      int token_id;
      int state_step;      // Manual program counter inside the state machine (e.g. 0: start, 1: inner loop)
      size_t start_pos;
  } ParserStackFrame;
  ```

### State Storage: $O(N)$ Pointer Map vs. $O(D)$ Scope Array
The single most significant change in the entire refactoring is how parser state is represented and stored:
* **Legacy Character Map**: An array of size $N$ (text length) containing pointers to active tokens: `const textparser_token_item *state[N]`. Memory cost: **$O(N)$** ($\approx 8\text{ MB}$ per $1\text{ MB}$ file).
* **New Active-Scope Stack**: A compact array of open nested token IDs: `int active_scopes[D]`, where $D$ is the nesting depth. Memory cost: **$O(D)$** ($\approx 8\text{--}20\text{ bytes}$ per cached boundary, as nesting depth $D$ is typically $1\text{--}5$).

---

## 2. Incremental Parsing for Syntax Highlighting (Arbitrary Offsets)

When syntax highlighting a file in a text editor, a full parse on every keystroke is extremely inefficient ($O(N)$ complexity). Instead, the parser processes arbitrary text segments (substrings/ranges) that can start and end at any offset (often sub-line or individual token boundaries).

### The Start State / End State Concept at Arbitrary Offsets
Instead of caching states only at line boundaries, states can be cached at **arbitrary offsets** or **token boundaries** throughout the document.

```
[Text Offset 0]  ---> (State: Empty)           ---> (Parse Range [0, 50])   ---> [State at Offset 50: Empty]
[Text Offset 50] ---> (State: Empty)           ---> (Parse Range [50, 80])  ---> [State at Offset 80: Inside Comment]
[Text Offset 80] ---> (State: Inside Comment)  ---> (Parse Range [80, 110]) ---> [State at Offset 110: Inside Comment]
```

### How Incremental Re-parsing Works on Edit:
1. When a user edits a segment of text starting at offset $S$, we retrieve the cached **Start State** corresponding to the token boundary immediately preceding $S$.
2. The active parent scopes (e.g. whether we are inside a string, block comment, or function body) are restored into the dynamic array stack from the retrieved state.
3. We invoke `textparser_parse_incremental(handle, definition, state, start_pos, end_pos)` to parse only the modified substring.
4. **Early Exit Optimization**: Once the parser reaches `end_pos`, if the current stack state matches the cached state at that offset from the previous parse, we **stop** immediately. The surrounding text does not need to be re-parsed.

---

## 3. Detailed Pros & Cons of the Proposed Approach

### Pros
* **Sub-millisecond Performance ($O(1)$ average)**: By only re-parsing changed segments and halting when states match, edits are handled instantly, even in files with millions of lines.
* **Low Memory Footprint**: No need to build or retain a full AST for the entire file. You only need to keep the state stack for the start of each line, which is usually very shallow (typically 0-5 frames deep).
* **Stack Overflow Prevention**: Recursive descent on deeply nested or malformed structures (e.g., millions of open brackets) can cause C stack overflows. A manual heap-allocated stack can safely grow or return an out-of-memory error.
* **Serialization/Multithreading Friendly**: The state stack is a simple data structure. It can be easily serialized, cached to disk, or passed across threads for parallel background highlighting.

### Cons & Implementation Challenges
* **Complexity in C**: C lacks coroutines or generator structures. The parser logic in `parse_token_group` and `parse_token_start_stop` must be fully rewritten as a state machine where local variables are manually packed/unpacked from stack frames.
* **Regex Engine Limitations**: If a regular expression matches across line boundaries (e.g., multi-line match), resuming from a partial line offset might cause the regex engine to fail or behave differently. The regexes must be evaluated starting from the current character offset with appropriate boundaries.

---

## 4. Alternative Architectural Approaches

| Approach | Pros | Cons | Recommendation |
| :--- | :--- | :--- | :--- |
| **Proposed Stack-Based Manual State Machine** (TextMate / VS Code Style) | Extremely fast for syntax highlighting, low memory usage, simple line-based resumption. | Hard to implement/maintain in C, does not easily produce a full AST for compilers. | **Highly Recommended** for syntax highlighting. |
| **Concrete Syntax Tree (CST) Mutation** (Tree-sitter Style) | Keeps a fully valid AST at all times, handles complex language semantics. | Extremely complex to build, requires grammar compilation pipeline. | Overkill if only syntax highlighting is needed. |
| **Coroutine / Fiber-based Suspendable Parser** | Clean code structure (keeps recursive functions intact), easy to write. | Non-standard C, requires platform-dependent context switching assembly or libraries. | Not recommended for portable C libraries. |

---

## 5. Sketch of the Non-Recursive Parser Loop

Here is how the main non-recursive parse loop would look:

```c
int textparser_parse_incremental(
    textparser_t handle, 
    const textparser_language_definition *definition,
    const ParserState *start_state,
    ParserState *out_end_state,
    size_t start_pos,
    size_t end_pos
) {
    // contiguous flat dynamic array stack
    ParserStack stack = { .frames = malloc(sizeof(ParserStackFrame) * 16), .size = 0, .capacity = 16 };
    if (start_state) restore_stack(&stack, start_state);

    size_t pos = start_pos;
    while (pos < end_pos || stack.size > 0) {
        if (stack.size == 0) {
            pos = textparser_skip_whitespace(handle, pos);
            if (pos >= end_pos) break;

            int tok_id = find_next_token(handle, definition->starts_with, pos);
            if (tok_id != TextParser_END) push_frame(&stack, tok_id, pos);
            else pos += textparser_char_len(handle, pos);
            continue;
        }

        ParserStackFrame *frame = &stack.frames[stack.size - 1];
        const textparser_token *def = &definition->tokens[frame->token_id];

        switch (def->type) {
            case TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN: {
                size_t len = match_simple_token(handle, frame->token_id, pos);
                if (len > 0) {
                    trigger_highlight_callback(handle, frame, pos, len);
                    pos += len;
                }
                stack.size--; // Pop
                break;
            }
            case TEXTPARSER_TOKEN_TYPE_START_STOP: {
                if (frame->state_step == 0) {
                    pos += match_start_regex(handle, frame->token_id, pos);
                    frame->state_step = 1;
                } else if (frame->state_step == 1) {
                    size_t end_len = match_end_regex(handle, frame->token_id, pos);
                    if (end_len > 0) {
                        pos += end_len;
                        stack.size--; // Pop
                    } else {
                        int child_id = find_next_nested_token(handle, def->nested_tokens, pos);
                        if (child_id != TextParser_END) push_frame(&stack, child_id, pos);
                        else pos += textparser_char_len(handle, pos);
                    }
                }
                break;
            }
        }
    }
    save_stack(out_end_state, &stack);
    free(stack.frames);
    return 0;
}
```

---

## 6. Analysis of the Proposed `textparser_parse_incremental` API Signature

You suggested the following signature:
```c
EXPORT_TEXTPARSER int textparser_parse_incremental(
    textparser_t handle, 
    const textparser_language_definition *definition, 
    textparser_parser_state *state
);
```

Here is a technical analysis of this signature, its strengths, and its potential design issues.

### A. Strengths & Elegance
1. **API Consistency**: It fits perfectly with the design patterns established by `textparser_parse(handle, definition)`.
2. **Implicit Context Lookup**: By using `textparser_parser_state` (which maps file offsets to their matching AST token items), the parser can inspect `state->state[start_offset - 1]` to identify the active parent token group or start-stop context.
3. **No New Public Structs**: It avoids introducing a separate `ParserStack` type to the public header, reducing API surface complexity.

---

### B. Critical Considerations & Refinement Areas

#### 1. The Offset Shift Problem (Text Modification)
When a user types or deletes text, the document's length changes.
* **Problem**: If the user inserts `5` characters at position `100`, the position offsets of all tokens after index `100` in the AST are shifted forward by `5`. The indices in the pre-existing `textparser_parser_state` map are also shifted and invalidated.
* **Implication**: If you pass the old `state` directly, the offsets inside it will be desynchronized from the new `handle->text_addr`.
* **Solution**: You need a utility function to apply index offsets before parsing resumes:
  ```c
  EXPORT_TEXTPARSER void textparser_state_shift(
      textparser_parser_state *state, 
      size_t edit_offset, 
      ptrdiff_t length_delta
  );
  ```

#### 2. Determining the Segment Boundaries
If the signature only accepts `handle`, `definition`, and `state`, it is missing explicit parameters to specify **where** the edit occurred and **how far** the parser needs to go.
* **Option A (Implicit boundary search)**: The parser compares the old text stream with the new text stream to find the bounds. This is computationally expensive.
* **Option B (Explicit parameters - Recommended)**: Add `start_pos` and `end_pos` parameters:
  ```c
  EXPORT_TEXTPARSER int textparser_parse_incremental(
      textparser_t handle, 
      const textparser_language_definition *definition, 
      textparser_parser_state *state,
      size_t start_pos,
      size_t end_pos
  );
  ```
  This makes it clear which segment of text the caller expects the parser to process.

---

### C. Re-implementing the Core `textparser_parse` API
A major advantage of this architecture is that the existing full-document parse function can simply delegate to the incremental parser with an empty state:

```c
EXPORT_TEXTPARSER int textparser_parse(
    textparser_t handle, 
    const textparser_language_definition *definition
) {
    // A full parse is simply an incremental parse from 0 to the end of the text
    // using a clean (null/empty) start state
    size_t total_units = textparser_get_total_units(handle);
    return textparser_parse_incremental(handle, definition, NULL, 0, total_units);
}
```

This ensures:
1. **DRY (Don't Repeat Yourself)**: You maintain only one core parsing engine.
2. **Backwards Compatibility**: Existing callers of `textparser_parse` do not need any code changes.

---

## 8. Impact on Codebase Complexity

Evaluating whether this architectural shift makes the codebase more complex requires looking at three distinct dimensions:

### 1. API Surface Complexity: **Low** (Actually Decreases)
By wrapping the legacy full parse in the new incremental parser:
* No new complex parameters or state structs need to be exposed to public users.
* Reusing the existing `textparser_parser_state` means the library's consumer-facing interface remains extremely compact and clean.

### 2. Codebase Redundancy & Size: **Neutral**
Because we can completely delete the old recursive parser functions (such as `parse_token_group` and `parse_token_start_stop`), the net size of the codebase remains roughly equivalent. We replace the recursive helper functions with iterative ones rather than adding them on top of the old functions.

### 3. Implementation Logic Complexity: **Low to Medium**
* **The State Machine Verbosity**: In recursive descent, nesting and scope context are handled automatically by the CPU's execution stack and compiler-generated function frames. In an iterative state machine, the developer must manually emulate this:
  * Pushing new scopes to a manual stack.
  * Maintaining an explicit `state_step` to switch-case inside loops (since C lacks native generators/coroutines to resume execution state mid-function).
* **AST Splicing is Eliminated**: Because incremental parsing is used solely for extracting highlight spans and not for mutating a persistent AST, we do not need to stitch nodes or adjust parent/child links in memory. This reduces implementation complexity significantly and protects against pointer corruption bugs.

### Conclusion
While the public API and overall project volume remain remarkably simple and clean, the **internal logical complexity of the core parse engine is very manageable**. This is a highly favorable trade-off to achieve scalable, high-performance, real-time syntax highlighting for large files.

---

## 9. Clarification on Stack State: Replacing Linked Lists with Flat Dynamic Arrays

Rather than maintaining stacks via linked lists of nodes (which involve frequent `malloc`/`free` calls and pointer-chasing overhead), the stack state inside `textparser_parse_incremental` will be implemented using a **contiguous flat dynamic array** (`realloc`-backed buffer).

### Advantages of the Contiguous Dynamic Array Stack:
1. **Cache Locality**: Accessing the stack frames is extremely fast because they reside in contiguous memory, minimizing CPU cache misses during the inner loop.
2. **Trivial Memory Management**: Pushing and popping simply increments or decrements an index count (e.g. `stack->size++` / `stack->size--`). Allocations only happen when the stack size exceeds capacity, where we double the capacity via `realloc`.
3. **Simple State Serialization**: A flat array is extremely easy to copy and serialize (via a simple `memcpy`) compared to serializing/deserializing a linked list of states, which makes caching state boundaries at arbitrary offsets highly performant.

---

## 10. Detailed Complexity Analysis (Time, Space, and Cognitive)

Implementing this dynamic-array stack-based algorithm introduces several distinct types of complexity that are analyzed below.

### A. Algorithmic Time Complexity

#### 1. Full Parse: **$O(N)$** (Unchanged)
A full document parse has the same time complexity as the recursive engine. We iterate through the text of length $N$ sequentially. While the asymptotic upper bound remains the same, the **constant factor is lower** because:
* We eliminate function call overhead (stack frame setup, register pushing/popping) from recursive function calls.
* We benefit from contiguous memory layout in our stack frames, boosting CPU pipeline efficiency.

#### 2. Incremental Parse: **$O(M)$ where $M \ll N$** (Massive Improvement)
When an edit is made:
* Instead of parsing the entire text $N$, we parse only the modified segment $M$ (which can be a single line, a group of lines, or a sub-line substring).
* **Early Stabilization**: Once we reach the end offset of the modified segment, if the current parser stack state matches the cached state at that offset, we halt. Thus, the runtime for editing is proportional to the size of the edit $O(M)$, which is virtually instant ($O(1)$ relative to file size).

---

### B. Algorithmic Space Complexity

#### 1. Active Stack Depth: **$O(D)$** (Extremely Low)
The memory used by the active parse stack is proportional to the nesting depth $D$ of the scopes (e.g., matching brackets, nested group tokens). 
* In typical source files, code nesting depth $D$ rarely exceeds $20$ levels. 
* A dynamic array stack of capacity $32$ is sufficient for almost all source files, taking up less than **$1$ KB** of heap memory.

#### 2. State Cache at Key Offsets: **$O(K \cdot d)$** (Minimal Overhead)
To support incremental parsing starting at arbitrary offsets, the editor caches the state stack at key document offsets (e.g., at the start of lines or at specific block boundaries).
* $K$ = Number of cached state offsets.
* $d$ = Stack depth at those offsets (typically $0$ to $3$ elements).
* **Overhead**: Caching states at key boundaries requires storing small integer arrays representing open tokens. For a large file, this overhead is negligible (around **$40$ to $100$ KB**).

---

### C. Cognitive / Implementation Complexity

The main cost of this shift is **human/code complexity**. You are trading clean C call structures for manual control mechanisms:

1. **Explicit Program Counter Emulation**:
   Since C doesn't support generators/resumable functions, you must replace C's automatic function return mechanism with manual stack pops and custom state variable transitions (e.g. `state_step`).
2. **Buffer Resizing Overhead**:
   Implementing dynamic resizing for the array stack requires checking capacities, doubling allocations (`realloc`), and handling out-of-memory errors safely inside the parsing loop.
3. **No AST Pointer Splicing Overhead**:
   Since the incremental parser does not mutate the persistent AST tree (which is instead rebuilt on a background thread), there is **zero overhead** or pointer management code needed to splice or restructure parent/child pointers on edits. The incremental parser simply reads the start state, runs the matching rules, fires highlight callbacks for the colorizer, and updates the state.

---

## 11. The Primary Architectural Shift: State Storage Representation

The single most significant change in the entire refactoring is **how parser state is represented and stored** in the system.

### Comparison of State Storage Models

| Dimension | Legacy `textparser_parser_state` (Character-Mapping Array) | New Proposed State (Active-Scope Dynamic Array) |
| :--- | :--- | :--- |
| **Data Structure** | A flat array mapping every character index to its active token item: `const textparser_token_item *state[text_size]` | A tiny dynamic array storing the stack of active parent token IDs: `int active_scopes[nesting_depth]` |
| **Space Complexity** | **$O(N)$** (Large memory footprint proportional to text length) | **$O(D)$** where $D \ll N$ (Negligible footprint proportional to nested scope depth) |
| **Typical Memory Size** | **$8$ MB** of pointers for a $1$ MB file (64-bit architecture) | **$< 20$ bytes** (typically $1$ to $5$ integers) |
| **Role in Parser Resumption** | Serves as a lookup map to retrieve the active token pointer at any byte position. | Serves as the lightweight state key used to restore/repopulate the stack when resuming. |

This shift from a **per-character pointer map** to a **per-offset dynamic scope stack** is the core change that makes state caching viable, extremely memory-efficient, and easy to pass between syntax highlight loops and background workers.
