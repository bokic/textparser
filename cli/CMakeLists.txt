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

#set_property(TARGET textparser PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

#set_target_properties(textparser PROPERTIES
#    INTERPROCEDURAL_OPTIMIZATION TRUE
#    C_VISIBILITY_PRESET hidden
#)

target_link_libraries(textparser PRIVATE
    libtextparser
)
