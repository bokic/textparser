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

    ${OS_FILES}
)

target_include_directories(ccat PRIVATE
    ../include
    ../definitions
)

target_compile_definitions(ccat PRIVATE
    TEXTPARSER_BINARY_VERSION="${TEXTPARSER_VERSION_TAG}"
)

target_link_libraries(ccat PRIVATE
    libtextparser
)

if(WIN32)
    target_sources(ccat PRIVATE ${RC_CCAT_OUT_FILE})
endif()

install(TARGETS ccat
    RUNTIME DESTINATION bin
    COMPONENT cli
)

if(BUILD_TESTS)
    add_test(NAME ccat_version COMMAND ccat --version)
    set_tests_properties(ccat_version PROPERTIES
        PASS_REGULAR_EXPRESSION "^ccat version "
    )
endif()
