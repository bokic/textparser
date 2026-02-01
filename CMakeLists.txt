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
                OUTPUT_VARIABLE TEXTPARSER_VERSION_TAG
                OUTPUT_STRIP_TRAILING_WHITESPACE # Removes trailing newline/whitespace
                RESULT_VARIABLE GIT_DESCRIBE_RESULT
                ERROR_QUIET
            )
            execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list ${TEXTPARSER_VERSION_TAG}..HEAD --count
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_REVISION
                OUTPUT_STRIP_TRAILING_WHITESPACE # Removes trailing newline/whitespace
                RESULT_VARIABLE GIT_REV_LIST_RESULT
                ERROR_QUIET
            )
            execute_process(
                COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_COMMIT_NAME
                OUTPUT_STRIP_TRAILING_WHITESPACE # Removes trailing newline/whitespace
                RESULT_VARIABLE GIT_DESCRIBE_RESULT
                ERROR_QUIET
            )
            set(REVISION ${GIT_REVISION})
        endif()
    endif()
endif()

if(NOT TEXTPARSER_VERSION_TAG MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "Git last tag '${TEXTPARSER_VERSION_TAG}' is not in format X.Y.Z")
endif()

string(REGEX REPLACE "^([0-9]+\\.[0-9]+)\\..*$" "\\1" TEXTPARSER_VERSION_TAG_SHORT ${TEXTPARSER_VERSION_TAG})

project(textparser)

include(GNUInstallDirs)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/definitions/json_definition.json.h
  COMMAND python ./json2h.py json_definition.json
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/definitions/json_definition.json
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/definitions
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/definitions/cfml_definition.json.h
  COMMAND python ./json2h.py cfml_definition.json
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/definitions/cfml_definition.json
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/definitions
)

add_subdirectory(src)
# add_subdirectory(src) is already called above
option(BUILD_CLI "Build CLI tool" ON)
if(BUILD_CLI)
    add_subdirectory(cli)
endif()

option(BUILD_CCAT "Build CCAT tool" ON)
if(BUILD_CCAT)
    add_subdirectory(ccat)
endif()

option(BUILD_TESTING "Build tests" ON) # Standard CMake option
if(BUILD_TESTING)
    add_subdirectory(tests)
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
