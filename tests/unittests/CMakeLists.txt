cmake_minimum_required(VERSION 3.15)
project(my_project)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP YES
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable( unittests
  cfml_tests.cpp
  json_tests.cpp

  tokenparser.hpp
)
target_include_directories( unittests PRIVATE
    ../../definitions
)
target_link_libraries( unittests
  libtextparser
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unittests)
