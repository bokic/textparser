cmake_minimum_required(VERSION 3.15)
project(my_project)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
  find_package(GTest QUIET)
endif()

if(NOT TARGET GTest::gtest_main)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
    GIT_SHALLOW    TRUE
  )

  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  # Suppress warnings from googletest sources
  if(NOT MSVC AND NOT WIN32)
    target_compile_options(gtest PRIVATE -Wno-character-conversion -Wno-language-extension-token)
    target_compile_options(gmock PRIVATE -Wno-character-conversion -Wno-language-extension-token)
  endif()
endif()

enable_testing()



add_executable( unittests
  version_tests.cpp
  cfml_tests.cpp
  json_tests.cpp
  php_tests.cpp
  java_tests.cpp
  c_tests.cpp
  cpp_tests.cpp
  csharp_tests.cpp
  javascript_tests.cpp
  css_tests.cpp
  html_tests.cpp
  bash_tests.cpp
  sql_tests.cpp
  pascal_tests.cpp
  typescript_tests.cpp
  fortran_tests.cpp
  perl_tests.cpp
  go_tests.cpp
  asm_tests.cpp
  swift_tests.cpp
  ada_tests.cpp
  matlab_tests.cpp
  md_tests.cpp
  vb_tests.cpp
  r_tests.cpp
  scratch_tests.cpp
  python_tests.cpp
  jai_tests.cpp
  c3_tests.cpp
  zig_tests.cpp
  rust_tests.cpp
  subtraction_tests.cpp
  openfile_tests.cpp
  query_tests.cpp
  incremental_tests.cpp
  structural_tests.cpp
  token_export_tests.cpp
  schema_parity_tests.cpp
  arena_lifecycle_tests.cpp

  tokenparser.hpp
)
target_include_directories( unittests PRIVATE
    ../../include
    ../../definitions
    ../../src
    ../../src/validation
    ../compat
)
target_compile_definitions(unittests PRIVATE
  TEXTPARSER_EXPECTED_VERSION="${GIT_COMMIT_NAME}"
  TEXTPARSER_EXPECTED_VERSION_MAJOR=${TEXTPARSER_VERSION_MAJOR}
  TEXTPARSER_EXPECTED_VERSION_MINOR=${TEXTPARSER_VERSION_MINOR}
  TEXTPARSER_EXPECTED_VERSION_PATCH=${TEXTPARSER_VERSION_PATCH}
  TEXTPARSER_EXPECTED_VERSION_INT=${TEXTPARSER_VERSION_INT}
)
if(NOT MSVC)
  target_compile_options(unittests PRIVATE -Wno-pedantic)
endif()
target_link_libraries( unittests
  libtextparser
  libtextparser-json
  cfml
  php
  html
  css
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unittests)
