cmake_minimum_required(VERSION 3.16)
project(gtk4_sourceview_textparser VERSION 0.1.0 LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(GTKSOURCEVIEW REQUIRED IMPORTED_TARGET gtksourceview-5)
pkg_check_modules(TEXTPARSER REQUIRED IMPORTED_TARGET textparser)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(TEXTPARSER_JSON2H NAMES textparser_json2h.py)
if(NOT TEXTPARSER_JSON2H)
    message(FATAL_ERROR "textparser_json2h.py is required to generate the language header")
endif()

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
add_custom_command(
    OUTPUT "${GENERATED_DIR}/json.json.h"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GENERATED_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/definitions/json.json" "${GENERATED_DIR}/json.json"
    COMMAND "${Python3_EXECUTABLE}" "${TEXTPARSER_JSON2H}"
        --no-native-regex "${GENERATED_DIR}/json.json"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/c11_header.py"
        "${GENERATED_DIR}/json.json.h"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/definitions/json.json" "${TEXTPARSER_JSON2H}"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/c11_header.py"
    BYPRODUCTS "${GENERATED_DIR}/json.json"
    COMMENT "Generating embedded JSON language definition"
    VERBATIM
)

add_executable(gtk4-sourceview main.c "${GENERATED_DIR}/json.json.h")
target_include_directories(gtk4-sourceview PRIVATE "${GENERATED_DIR}")
set_target_properties(gtk4-sourceview PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)
target_link_libraries(gtk4-sourceview PRIVATE
    PkgConfig::GTK4
    PkgConfig::GTKSOURCEVIEW
    PkgConfig::TEXTPARSER
)

option(BUILD_EDITOR_TESTS "Build GTK editor integration checks (requires a display)" OFF)
if(BUILD_EDITOR_TESTS)
    enable_testing()
    add_executable(editor-test tests/editor-test.c "${GENERATED_DIR}/json.json.h")
    add_dependencies(editor-test gtk4-sourceview)
    target_include_directories(editor-test PRIVATE "${GENERATED_DIR}")
    set_target_properties(editor-test PROPERTIES
        C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
    target_link_libraries(editor-test PRIVATE
        PkgConfig::GTK4 PkgConfig::GTKSOURCEVIEW PkgConfig::TEXTPARSER)
    add_test(NAME editor-integration COMMAND editor-test)
    set_tests_properties(editor-integration PROPERTIES ENVIRONMENT "GSK_RENDERER=cairo")
endif()
