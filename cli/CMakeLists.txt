cmake_minimum_required(VERSION 3.15)

project(textparser LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT WIN32)
    include(GNUInstallDirs)

    find_package(json-c CONFIG REQUIRED)
    find_package(PkgConfig)

    pkg_search_module(libpcre2-8 REQUIRED libpcre2-8>=10.42)
    pkg_search_module(libpcre2-16 REQUIRED libpcre2-16>=10.42)
    pkg_search_module(libpcre2-32 REQUIRED libpcre2-32>=10.42)
endif()

add_executable(textparser
    main.c
)

target_include_directories(textparser PRIVATE
    ../definitions
)

target_link_libraries(textparser PRIVATE
    libtextparser
)

if(WIN32)
    target_link_directories(textparser PRIVATE ../bin)
    target_link_libraries(textparser PRIVATE json-c.lib)
else()
    target_include_directories(textparser PRIVATE
        json-c::json-c
    )

    target_link_directories(textparser PRIVATE
        json-c::json-c
    )

    target_link_libraries(textparser PRIVATE
        json-c::json-c
    )
endif()

install(TARGETS textparser
    RUNTIME DESTINATION bin
    COMPONENT cli
)
