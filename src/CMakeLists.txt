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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../os>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(WIN32)
    target_link_directories(libtextparser PRIVATE ../bin)
    target_link_libraries(libtextparser PRIVATE pcre2-8.lib pcre2-16.lib pcre2-32.lib)
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
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

install(TARGETS libtextparser
    EXPORT textparserTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT src
)

add_library(libtextparser-json SHARED
    textparser-json.c
    ../include/textparser-json.h
)

target_include_directories(libtextparser-json PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

install(TARGETS libtextparser-json
    EXPORT textparserTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT src
)

# Export targets
install(EXPORT textparserTargets
    FILE textparserTargets.cmake
    NAMESPACE textparser::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/textparser
    COMPONENT Devel
)

include(CMakePackageConfigHelpers)

set(ConfigPackageLocation ${CMAKE_INSTALL_LIBDIR}/cmake/textparser)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/textparserConfigVersion.cmake"
    VERSION ${TEXTPARSER_VERSION_TAG}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/textparserConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/textparserConfig.cmake"
    INSTALL_DESTINATION ${ConfigPackageLocation}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/textparserConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/textparserConfigVersion.cmake"
    DESTINATION ${ConfigPackageLocation}
    COMPONENT Devel
)
