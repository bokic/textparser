cmake_minimum_required(VERSION 3.15)

project(cfml_validator VERSION "${TEXTPARSER_VERSION_TAG}" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCES
    main.cpp
)

add_executable(cfml_validator
    ${PROJECT_SOURCES}
)

target_include_directories(cfml_validator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../definitions
)

target_link_libraries(cfml_validator PRIVATE
    libtextparser
)

install(TARGETS cfml_validator
    RUNTIME DESTINATION bin
    COMPONENT tests
)
