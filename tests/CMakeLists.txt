cmake_minimum_required(VERSION 3.15)

if (NOT APPLE)
    add_subdirectory(unittests)
    add_subdirectory(benchmark)
endif()

#add_subdirectory(qscintilla)
