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
if(NOT WIN32)
    target_link_libraries(benchmark_cfml PRIVATE
        PkgConfig::libpcre2-8
        PkgConfig::libpcre2-16
        PkgConfig::libpcre2-32
    )
endif()

# ── SQLite C/H benchmark (Google Benchmark) ──────────────────────────────────
add_executable(benchmark_sqlite
    benchmark_sqlite.cpp
)
target_include_directories(benchmark_sqlite PRIVATE
    ../../include
    ../../definitions
)
target_compile_definitions(benchmark_sqlite PRIVATE
    SQLITE_SRC_DIR="${SQLITE_INNER_DIR}"
)
if(NOT MSVC)
    target_compile_options(benchmark_sqlite PRIVATE -Wno-pedantic)
endif()
target_link_libraries(benchmark_sqlite PRIVATE
    libtextparser
    benchmark::benchmark_main
)
if(NOT WIN32)
    target_link_libraries(benchmark_sqlite PRIVATE
        PkgConfig::libpcre2-8
        PkgConfig::libpcre2-16
        PkgConfig::libpcre2-32
    )
endif()
