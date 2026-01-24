cmake_minimum_required(VERSION 3.15)

if (NOT APPLE)
    add_subdirectory(unittests)
endif()

add_subdirectory(cfml_validator)
