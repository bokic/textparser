cmake_minimum_required(VERSION 3.15)

SET(TEXTPARSER_VERSION_TAG CACHE STRING "")
SET(REVISION CACHE STRING "0")

if(NOT TEXTPARSER_VERSION_TAG)
    find_package(Git QUIET)

    set(GIT_DIR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    if(EXISTS ${GIT_DIR_PATH})
        if(Git_FOUND)
            execute_process(
                COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_TAG_OUTPUT
                OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE GIT_DESCRIBE_RESULT
                ERROR_QUIET
            )
            if(GIT_DESCRIBE_RESULT EQUAL 0 AND GIT_TAG_OUTPUT)
                set(TEXTPARSER_VERSION_TAG "${GIT_TAG_OUTPUT}")
                execute_process(
                    COMMAND ${GIT_EXECUTABLE} rev-list ${TEXTPARSER_VERSION_TAG}..HEAD --count
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    OUTPUT_VARIABLE GIT_REVISION
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    RESULT_VARIABLE GIT_REV_LIST_RESULT
                    ERROR_QUIET
                )
                if(GIT_REV_LIST_RESULT EQUAL 0 AND GIT_REVISION)
                    set(REVISION ${GIT_REVISION})
                endif()
            endif()
        endif()
    endif()
endif()

if(NOT TEXTPARSER_VERSION_TAG)
    if(DEFINED ENV{TEXTPARSER_VERSION})
        set(TEXTPARSER_VERSION_TAG "$ENV{TEXTPARSER_VERSION}")
        set(REVISION "0")
    else()
        message(FATAL_ERROR "Not building under Git repository and the TEXTPARSER_VERSION environment variable is not set! Please set TEXTPARSER_VERSION (e.g., export TEXTPARSER_VERSION=X.Y.Z) to configure the build.")
    endif()
endif()

if(NOT TEXTPARSER_VERSION_TAG MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "Git last tag '${TEXTPARSER_VERSION_TAG}' is not in format X.Y.Z")
endif()

string(REGEX REPLACE "^([0-9]+\\.[0-9]+)\\..*$" "\\1" TEXTPARSER_VERSION_TAG_SHORT ${TEXTPARSER_VERSION_TAG})

project(textparser)

include(GNUInstallDirs)
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
if(IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO is not supported: ${IPO_ERROR}")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

option(BUILD_TESTS "Build tests" ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /GS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wstack-protector -fstack-protector-strong)
endif()

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
endif()

file(GLOB DEF_FILES "${CMAKE_CURRENT_SOURCE_DIR}/definitions/*_definition.json")
set(GENERATED_DEFINITIONS "")
foreach(DEF_FILE ${DEF_FILES})
  get_filename_component(DEF_NAME "${DEF_FILE}" NAME)
  set(HEADER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/definitions/${DEF_NAME}.h")
  list(APPEND GENERATED_DEFINITIONS "${HEADER_FILE}")
  add_custom_command(
    OUTPUT "${HEADER_FILE}"
    COMMAND ${Python3_EXECUTABLE} ./json2h.py "${DEF_NAME}"
    DEPENDS "${DEF_FILE}" "${CMAKE_CURRENT_SOURCE_DIR}/definitions/json2h.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/definitions"
  )
endforeach()

add_custom_target(generated_definitions DEPENDS ${GENERATED_DEFINITIONS})

add_subdirectory(src)
add_subdirectory(cli)
add_subdirectory(ccat)
if(BUILD_TESTS)
  add_subdirectory(tests)
endif()

add_dependencies(textparser generated_definitions)
add_dependencies(ccat generated_definitions)
if(BUILD_TESTS)
  if(NOT APPLE)
    add_dependencies(unittests generated_definitions)
  endif()
endif()

# PkgConfig config
configure_file(textparser.pc.in textparser.pc @ONLY)
configure_file(textparser-json.pc.in textparser-json.pc @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/textparser.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        COMPONENT src)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/textparser-json.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        COMPONENT src)
