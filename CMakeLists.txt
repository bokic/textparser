cmake_minimum_required(VERSION 3.15)

project(textparser)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_subdirectory(src)
add_subdirectory(cli)
add_subdirectory(tests)
