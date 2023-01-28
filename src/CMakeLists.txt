cmake_minimum_required(VERSION 3.15)

project(libtextparser LANGUAGES C)

set(CMAKE_C_STANDARD 23)

include_directories(BEFORE PUBLIC
    ../include
)

add_library(libtextparser SHARED
    textparser.c
    adv_regex.c

    ../include/textparser.h
    ../include/adv_regex.h
    ../definitions/cfml_definition.h
    ../definitions/json_definition.h
    ../definitions/php_definition.h
)

target_link_libraries(libtextparser
    utf8proc
    pcre2-8
)

set_target_properties(libtextparser PROPERTIES
    PUBLIC_HEADER "../include/textparser.h"
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser"
    VERSION "0.5.0"
    SOVERSION "0.5"
)