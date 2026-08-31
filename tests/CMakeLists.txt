cmake_minimum_required(VERSION 3.15)

if(BUILD_TESTS)
  add_subdirectory(unittests)
endif()
if(BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif()
