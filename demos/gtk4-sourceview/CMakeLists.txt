cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(gtk4-sourceview LANGUAGES C)

find_package(PkgConfig)
if(NOT PkgConfig_FOUND)
    message(WARNING "PkgConfig not found!")
    return()
endif()
pkg_check_modules(GTK4 IMPORTED_TARGET gtk4)
if(NOT GTK4_FOUND)
    message(WARNING "GTK4 not found!")
    return()
endif()
pkg_check_modules(GTKSOURCEVIEW IMPORTED_TARGET gtksourceview-5)
if(NOT GTKSOURCEVIEW_FOUND)
    message(WARNING "GTKSOURCEVIEW not found!")
    return()
endif()
pkg_check_modules(TEXTPARSER IMPORTED_TARGET textparser)
if(NOT TEXTPARSER_FOUND)
    message(WARNING "TEXTPARSER not found!")
    return()
endif()

add_executable(gtk4-sourceview
    main.c
)
set_target_properties(gtk4-sourceview PROPERTIES
    OUTPUT_NAME demo-gtk4-sourceview
)
target_include_directories(gtk4-sourceview PRIVATE
    ../../definitions
    ../../include
)
target_link_libraries(gtk4-sourceview PRIVATE
    PkgConfig::GTK4
    PkgConfig::GTKSOURCEVIEW
    PkgConfig::TEXTPARSER
)
