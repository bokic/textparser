cmake_minimum_required(VERSION 3.15)

add_subdirectory(unittests)
if(BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif()
