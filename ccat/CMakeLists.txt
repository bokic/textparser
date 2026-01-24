cmake_minimum_required(VERSION 3.15)

project(ccat VERSION "${TEXTPARSER_VERSION_TAG}" LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if(WIN32)
    set(OS_FILES ../os/os_win.c ../os/os.h)
else()
    set(OS_FILES ../os/os_posix.c ../os/os.h)
endif()

add_executable(ccat
    main.c
    ../definitions/json_definition.json.h
    ../definitions/cfml_definition.json.h

    ${OS_FILES}
)

target_include_directories(ccat PRIVATE
    ../definitions
)

target_link_libraries(ccat PRIVATE
    libtextparser
)

install(TARGETS ccat
    RUNTIME DESTINATION bin
    COMPONENT cli
)
