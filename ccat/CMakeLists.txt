cmake_minimum_required(VERSION 3.15)

project(ccat VERSION 0.6.0 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(ccat
    main.c
)

target_link_libraries(ccat PRIVATE
    libtextparser
)
