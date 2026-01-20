cmake_minimum_required(VERSION 3.15)

project(libtextparser LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(WIN32)
    set(OS_FILES ../os/os_win.c ../os/os.h)
else()
    set(OS_FILES ../os/os_posix.c ../os/os.h)
endif()

if(NOT WIN32)
    include(GNUInstallDirs)

    find_package(json-c CONFIG REQUIRED)
    find_package(PkgConfig)

    pkg_search_module(libpcre2-8 REQUIRED libpcre2-8>=10.42)
    pkg_search_module(libpcre2-16 REQUIRED libpcre2-16>=10.42)
    pkg_search_module(libpcre2-32 REQUIRED libpcre2-32>=10.42)
endif()

add_library(libtextparser SHARED
    textparser.c
    adv_regex.c

    ../include/textparser.h
    ../include/adv_regex.h

    ${OS_FILES}
)

target_include_directories(libtextparser PUBLIC
    ../include
    ../os
)

if(WIN32)
    target_link_directories(libtextparser PRIVATE ../bin)
    target_link_libraries(libtextparser PRIVATEpcre2-8.lib  pcre2-16.lib pcre2-32.lib)
else()
    target_include_directories(libtextparser PRIVATE
        ${libpcre2-8_INCLUDE_DIRS}
        ${libpcre2-16_INCLUDE_DIRS}
        ${libpcre2-32_INCLUDE_DIRS}
    )

    target_link_directories(libtextparser PRIVATE
        ${libpcre2-8_LIBRARY_DIRS}
        ${libpcre2-16_LIBRARY_DIRS}
        ${libpcre2-32_LIBRARY_DIRS}
    )

    target_link_libraries(libtextparser PRIVATE
        ${libpcre2-8_LIBRARIES}
        ${libpcre2-16_LIBRARIES}
        ${libpcre2-32_LIBRARIES}
    )
endif()

set_target_properties(libtextparser PROPERTIES
    PUBLIC_HEADER "../include/textparser.h"
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser"
    VERSION "0.7.0"
    SOVERSION "0.7"
)

install(TARGETS libtextparser
    RUNTIME DESTINATION bin
    PUBLIC_HEADER
    COMPONENT src
)

add_library(libtextparser-json SHARED
    textparser-json.c
    ../include/textparser-json.h
)

target_include_directories(libtextparser-json PUBLIC
    ../include
)

if(WIN32)
    target_link_directories(libtextparser-json PRIVATE ../bin)
    target_link_libraries(libtextparser-json PRIVATE json-c.lib)
else()
    target_include_directories(libtextparser-json PRIVATE
        json-c::json-c
    )

    target_link_directories(libtextparser-json PRIVATE
        json-c::json-c
    )

    target_link_libraries(libtextparser-json PRIVATE
        json-c::json-c
    )
endif()

set_target_properties(libtextparser-json PROPERTIES
    PUBLIC_HEADER "../include/textparser-json.h"
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser-json"
    VERSION "0.7.0"
    SOVERSION "0.7"
)

install(TARGETS libtextparser-json
    RUNTIME DESTINATION bin
    PUBLIC_HEADER
    COMPONENT src
)
