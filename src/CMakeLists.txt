cmake_minimum_required(VERSION 3.15)

project(libtextparser LANGUAGES C)

set(CMAKE_C_STANDARD 23)

add_definitions(-DDEBUG_PARSER)

find_package(PkgConfig)

pkg_search_module(json-c REQUIRED json-c>=0.17)
pkg_search_module(libpcre2-8 REQUIRED libpcre2-8>=10.42)
pkg_search_module(libpcre2-16 REQUIRED libpcre2-16>=10.42)
pkg_search_module(libpcre2-32 REQUIRED libpcre2-32>=10.42)

add_library(libtextparser SHARED
    textparser.c
    adv_regex.c

    ../include/textparser.h
    ../include/adv_regex.h
    ../definitions/cfml_definition.h
    ../definitions/json_definition.h
    ../definitions/php_definition.h
)

target_include_directories(libtextparser PUBLIC
    ../include
    ../definitions
)

target_include_directories(libtextparser PRIVATE
    ${json-c_INCLUDE_DIRS}
    ${libpcre2-8_INCLUDE_DIRS}
    ${libpcre2-16_INCLUDE_DIRS}
    ${libpcre2-32_INCLUDE_DIRS}
)

target_link_directories(libtextparser PRIVATE
    ${json-c_LIBRARY_DIRS}
    ${libpcre2-8_LIBRARY_DIRS}
    ${libpcre2-16_LIBRARY_DIRS}
    ${libpcre2-32_LIBRARY_DIRS}
)

target_link_libraries(libtextparser PRIVATE
    ${json-c_LIBRARIES}
    ${libpcre2-8_LIBRARIES}
    ${libpcre2-16_LIBRARIES}
    ${libpcre2-32_LIBRARIES}
)

set_target_properties(libtextparser PROPERTIES
    PUBLIC_HEADER "../include/textparser.h"
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser"
    VERSION "0.5.0"
    SOVERSION "0.5"
)
