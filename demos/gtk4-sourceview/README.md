# GTK 4 GtkSourceView example

A minimal C application with one window containing a scrollable GtkSourceView
editor with line numbers and live JSON syntax highlighting provided by textparser.

Requires a C compiler, CMake 3.16 or newer, pkg-config, and development packages
for GTK 4, GtkSourceView 5, and textparser (`gtk4`, `gtksourceview-5`, and
`textparser` in pkg-config). Building also requires Python 3 and textparser's
`textparser_json2h.py` converter on PATH.

Build and run:

```sh
./build.sh
./build/gtk4-sourceview
```

Hold Ctrl and scroll the mouse wheel up/down to enlarge/shrink the editor font
(6–48 pt, initially 12 pt). Scrolling without Ctrl scrolls the document normally.

The build script accepts extra CMake configuration arguments, for example:

```sh
./build.sh -DCMAKE_BUILD_TYPE=Release
```

`definitions/json.json` is adapted from textparser's JSON language definition.
It forbids unmatched text in all contexts, uses explicit string-content and
escape tokens, and supports case-sensitive JSON literals, UTF-8, negative
numbers, and exponents. Scalar JSON values are also accepted at the root.
CMake converts it to `build/generated/json.json.h` using textparser's converter
and compiles the resulting language definition directly into the executable.
No external JSON file or textparser JSON extension is needed at runtime.
Edit the definition and rebuild to regenerate the header and update highlighting
rules or colors. The usual GTK, GtkSourceView, and textparser shared libraries
are still required at runtime.

The editor starts with sample JSON. A persistent parser receives each insertion
or deletion immediately through `textparser_parse_incremental()`, using UTF-8
byte offsets and lengths. There is no typing delay. After an incremental edit,
the editor compares the token/ancestor IDs and starts at the old and new edit
endpoints and checks that child-token lengths cover their parent and that
required closing delimiters are present. If these checks fail, it immediately
falls back to a full parse. Edits to JSON punctuation, quotes, or escapes, and
edits involving parser errors also trigger a full parse. Safe edits retain the
incremental result; these conservative checks are not a general proof that an
incremental result equals a full parse.
Highlighting and diagnostics are repainted across the full buffer after each edit,
with byte positions converted to GTK character offsets. Highlighting is not a
strict JSON validity check.

Errors reported by textparser get red wavy underlines and a message below the
editor with the first error's line and column. End-of-document errors underline
the preceding character. For errors with only a byte position, the editor infers
an underline span up to whitespace or JSON punctuation; token errors use the
length supplied by the parser. Diagnostics refresh after edits and disappear when
the parser no longer reports them. Unprocessed tokens also count as errors in
the editor, even if the library returns success. This catches
leading/trailing garbage. Token recognition still does not enforce the complete
JSON grammar: missing commas and multiple root values can still be accepted.
The installed textparser 1.0.13 can produce inconsistent string-token ranges and
miss closing-delimiter errors during incremental editing. The fallback handles
these cases without delaying feedback.

Run GTK integration checks (requires an available graphical display):

```sh
./build.sh -DBUILD_EDITOR_TESTS=ON
ctest --test-dir build --output-on-failure
```
