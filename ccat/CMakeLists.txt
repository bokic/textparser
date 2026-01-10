cmake_minimum_required(VERSION 3.15)

project(ccat VERSION 0.6.0 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../definitions/json_definition.json.h
  COMMAND ./json2h.py json_definition.json
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../definitions/json_definition.json
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../definitions
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../definitions/cfml_definition.json.h
  COMMAND ./json2h.py cfml_definition.json
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../definitions/cfml_definition.json
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../definitions
)

add_executable(ccat
    main.c
    ../definitions/json_definition.json.h
    ../definitions/cfml_definition.json.h
)

target_include_directories(ccat PRIVATE
    ../definitions
)

target_link_libraries(ccat PRIVATE
    libtextparser
)

install(TARGETS ccat
    COMPONENT ccat
)
