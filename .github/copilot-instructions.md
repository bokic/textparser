# Copilot / AI agent instructions for textparser

Quick reference to get productive with this repo. Read this before editing or generating code.

- **Big picture:** 
 * Parsing library with logic that lives in `src/` as is built as a shared C library `libtextparser` (see [src/CMakeLists.txt](src/CMakeLists.txt) and [src/textparser.c](src/textparser.c)).
 * A small JSON-backed language definition loader `libtextparser-json` and CLI tools live under `src/` and `cli/`.
 * Language definitions are in `definitions/` in json format, that can be loaded at runtime or generate c headers at compile time.
 * Python parser variant is in `python/` and mirror the C parsing logic for rapid prototyping ([python/textparser.py](python/textparser.py)).

- **Build / test / dev commands:**
  - Preferred quick build: run `./build.sh` (runs `cmake -B build -G Ninja` and `cmake --build build`, and copies `compile_commands.json`). See [build.sh](build.sh).
  - Binaries are emitted to the top-level `bin/` (CMake `CMAKE_RUNTIME_OUTPUT_DIRECTORY` is set to project root). After build run `bin/unittests` or `bin/textparser` directly.
  - To run the Python parser: `python3 python/parse.py definitions/json_definition.json <somefile.cfm/cfc>` (see `python/parse.py`).
  - After installation, the library can be used via `find_package(textparser)` or `pkg-config --cflags --libs textparser`.

- **Key patterns & conventions:**
  - C API: public functions are declared in `include/textparser.h`. Use `textparser_defer()` and cleanup helpers defined there for RAII-style cleanup.
  - Token/language model: tokens are described by `textparser_language_definition` and `textparser_token` structs. Token lists use a sentinel `TextParser_END` and nested token arrays — mirror these when editing or generating new language defs (see [include/textparser.h](include/textparser.h)).
  - Regex subsystem: uses PCRE2 and supports multiple code-unit widths (8/16/32). Regex compilation is cached per-handle (`adv_regex.c`). When generating new regex usage, follow the adv_regex_* helpers (see [src/adv_regex.c](src/adv_regex.c)).
  - Build system: CMake with pkg-config lookups for `libpcre2` and `json-c`. Generates and installs `textparserConfig.cmake`, `textparserConfigVersion.cmake`, and `textparser.pc` for downstream consumption.
  - **C++ Standard**: The project uses **C++23** for all C++ code (tests, examples, bindings). Do not suggest older C++ standards.
  - **Qt Deprecation**: Avoid using Qt core classes (QFile, QString, etc.) for file and string handling. Prefer standard C++23 (`std::filesystem`, `std::string`, `std::expected`, etc.) or internal C helpers.

- **Where to add tests & examples:**
  - Unit tests live under `tests/unittests/` and use GoogleTest (see [tests/unittests/unittests.cpp](tests/unittests/unittests.cpp)). New parser behavior should include a small, focused unit test there.
  - JSON/CFML definitions are used as test fixtures by including generated headers in `tests/` and `cli/`.

- **Codegen / contributions guidance for AI:**
  - Prefer small, focused changes: add a new token → update `definitions/*.json`, regenerate headers (the repo contains `definitions/json2h.py` and `regenerate.sh`), then add a unit test under `tests/unittests/` and update `CMakeLists.txt` if a new source file is required.
  - When editing regexes prefer the existing `adv_regex` helpers rather than direct pcre2 calls.
  - Keep public API stable: changes to `include/textparser.h` require bumping version properties in `src/CMakeLists.txt`.

- **Common pitfalls to avoid:**
  - Don’t assume binaries are under `build/` — runtime outputs are configured to `bin/` at project root.
  - Regex compilation errors are printed to stdout/stderr in `adv_regex.c`; run small reproducer inputs and unit tests after changing regex strings.

If anything here is unclear or you'd like more detail (examples of a token change, test harness, or a small refactor), tell me which area and I will expand or iterate.
