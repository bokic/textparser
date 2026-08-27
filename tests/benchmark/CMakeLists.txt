cmake_minimum_required(VERSION 3.15)
project(textparser_benchmark LANGUAGES CXX)

include(FetchContent)

# ── Google Benchmark ──────────────────────────────────────────────────────────
FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.4
    GIT_SHALLOW    TRUE
)
set(BENCHMARK_ENABLE_TESTING   OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

# Suppress -Wc2y-extensions triggered by __COUNTER__ in benchmark headers on newer Clang (19+)
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wno-c2y-extensions HAS_C2Y_WARNING_FLAG)
if(HAS_C2Y_WARNING_FLAG)
    target_compile_options(benchmark PRIVATE -Wno-c2y-extensions)
    target_compile_options(benchmark_main PRIVATE -Wno-c2y-extensions)
endif()

# ── SQLite source download & extraction ──────────────────────────────────────
set(SQLITE_VERSION   "3.53.0")
set(SQLITE_ZIP_URL   "https://github.com/sqlite/sqlite/archive/refs/tags/version-${SQLITE_VERSION}.zip")
set(SQLITE_ZIP_PATH  "${CMAKE_CURRENT_BINARY_DIR}/sqlite-${SQLITE_VERSION}.zip")
set(SQLITE_SRC_DIR   "${CMAKE_CURRENT_BINARY_DIR}/sqlite-src")

if(NOT EXISTS "${SQLITE_SRC_DIR}")
    message(STATUS "Downloading SQLite ${SQLITE_VERSION}...")
    file(DOWNLOAD "${SQLITE_ZIP_URL}" "${SQLITE_ZIP_PATH}"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
        TLS_VERIFY ON)
    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_STATUS_CODE)
    if(NOT DOWNLOAD_STATUS_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download SQLite: ${DOWNLOAD_STATUS}")
    endif()
    message(STATUS "Extracting SQLite ${SQLITE_VERSION}...")
    file(ARCHIVE_EXTRACT
        INPUT   "${SQLITE_ZIP_PATH}"
        DESTINATION "${SQLITE_SRC_DIR}")
    message(STATUS "SQLite extracted to ${SQLITE_SRC_DIR}")
endif()

# Resolve the inner directory name (e.g. sqlite-version-3.53.0)
file(GLOB SQLITE_INNER_DIR LIST_DIRECTORIES true "${SQLITE_SRC_DIR}/sqlite-*")
list(GET SQLITE_INNER_DIR 0 SQLITE_INNER_DIR)

# ── PCRE2 ─────────────────────────────────────────────────────────────────────
if(NOT WIN32)
    find_package(PkgConfig REQUIRED)
    foreach(width 8 16 32)
        pkg_search_module(libpcre2-${width} REQUIRED IMPORTED_TARGET libpcre2-${width}>=10.42)
    endforeach()
endif()

# ── CFML benchmark (existing) ─────────────────────────────────────────────────
add_executable(benchmark_cfml
    benchmark.cpp
)
target_include_directories(benchmark_cfml PRIVATE
    ../../include
    ../../definitions
)
if(NOT MSVC)
    target_compile_options(benchmark_cfml PRIVATE -Wno-pedantic)
endif()
target_link_libraries(benchmark_cfml PRIVATE
    libtextparser
    cfml
)
if(WIN32)
    target_link_directories(benchmark_cfml PRIVATE ../../bin)
    target_link_libraries(benchmark_cfml PRIVATE
        pcre2-8.lib
        pcre2-16.lib
        pcre2-32.lib
    )
else()
    target_link_libraries(benchmark_cfml PRIVATE
        PkgConfig::libpcre2-8
        PkgConfig::libpcre2-16
        PkgConfig::libpcre2-32
    )
endif()

# ── Tree-sitter runtime and C grammar ───────────────────────────────────────
FetchContent_Declare(
    treesitter
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
    GIT_TAG        v0.24.7
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  lib
)
FetchContent_MakeAvailable(treesitter)

FetchContent_Declare(
    treesitter_c
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-c.git
    GIT_TAG        v0.23.4
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(treesitter_c)
if(NOT treesitter_c_POPULATED)
    # tree-sitter-c ships a generated parser.c. Do not enter its CMake project,
    # which tries to regenerate the file and requires the tree-sitter CLI.
    cmake_policy(PUSH)
    if(POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif()
    FetchContent_Populate(treesitter_c)
    cmake_policy(POP)
endif()

add_library(tree-sitter-c STATIC
    ${treesitter_c_SOURCE_DIR}/src/parser.c
)
target_include_directories(tree-sitter-c PRIVATE
    ${treesitter_c_SOURCE_DIR}/src
)
target_compile_options(tree-sitter-c PRIVATE -w)

# ── SQLite C/H benchmark (Google Benchmark) ──────────────────────────────────
add_executable(benchmark_sqlite
    benchmark_sqlite.cpp
)
target_include_directories(benchmark_sqlite PRIVATE
    ../../include
    ../../definitions
    ${treesitter_SOURCE_DIR}/lib/include
)
target_compile_definitions(benchmark_sqlite PRIVATE
    SQLITE_SRC_DIR="${SQLITE_INNER_DIR}"
    C_DEF_JSON_PATH="${CMAKE_SOURCE_DIR}/definitions/c_definition.json"
)
if(NOT MSVC)
    target_compile_options(benchmark_sqlite PRIVATE -Wno-pedantic)
endif()
target_link_libraries(benchmark_sqlite PRIVATE
    libtextparser-json
    tree-sitter
    tree-sitter-c
    benchmark::benchmark_main
)
if(WIN32)
    target_link_directories(benchmark_sqlite PRIVATE ../../bin)
    target_link_libraries(benchmark_sqlite PRIVATE
        pcre2-8.lib
        pcre2-16.lib
        pcre2-32.lib
    )
else()
    target_link_libraries(benchmark_sqlite PRIVATE
        PkgConfig::libpcre2-8
        PkgConfig::libpcre2-16
        PkgConfig::libpcre2-32
    )
endif()
