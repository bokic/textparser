cmake_minimum_required(VERSION 3.15)
project(my_project)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

enable_testing()

add_executable( unittests
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

  tokenparser.hpp
)
target_include_directories( unittests PRIVATE
    ../../include
    ../../definitions
)
target_link_libraries( unittests
  libtextparser
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unittests)
