cmake_minimum_required(VERSION 3.15)
project(textparser_benchmark LANGUAGES CXX)

if(NOT WIN32)
    find_package(PkgConfig REQUIRED)
    foreach(width 8 16 32)
        pkg_search_module(libpcre2-${width} REQUIRED IMPORTED_TARGET libpcre2-${width}>=10.42)
    endforeach()
endif()

add_executable(benchmark
    benchmark.cpp
)

target_include_directories(benchmark PRIVATE
    ../../include
    ../../definitions
)

if(NOT MSVC)
    target_compile_options(benchmark PRIVATE -Wno-pedantic)
endif()

target_link_libraries(benchmark PRIVATE
    libtextparser
    cfml
    PkgConfig::libpcre2-8
    PkgConfig::libpcre2-16
    PkgConfig::libpcre2-32
)
