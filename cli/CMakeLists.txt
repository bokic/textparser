cmake_minimum_required(VERSION 3.15)

project(textparser LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(textparser
    main.c
)

target_include_directories(textparser PRIVATE
    ../definitions
)

target_link_libraries(textparser PRIVATE
    libtextparser
)

install(TARGETS textparser
    RUNTIME DESTINATION bin
    COMPONENT cli
)
