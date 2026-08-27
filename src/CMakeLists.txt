cmake_minimum_required(VERSION 3.15)

project(libtextparser LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(validation)

if(WIN32)
    set(OS_FILES ../os/os_win.c ../os/os.h)
else()
    set(OS_FILES ../os/os_posix.c ../os/os.h)
endif()

if(NOT WIN32)
    include(GNUInstallDirs)

    find_package(json-c CONFIG REQUIRED)
endif()

add_library(libtextparser SHARED
    ${CMAKE_BINARY_DIR}/version.c
    textparser.c
    adv_regex.c
    adv_regex.h
    search_function_gen.c

    ../include/search_function_gen.h
    ../include/textparser.h
    ../include/textparser.hpp

    ${OS_FILES}
)

target_include_directories(libtextparser PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../os>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(WIN32)
    target_include_directories(libtextparser PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/json>
    )
endif()

if(NOT WIN32)
    target_link_libraries(libtextparser PRIVATE ${CMAKE_DL_LIBS})
endif()

set_target_properties(libtextparser PROPERTIES
    PUBLIC_HEADER "../include/textparser.h;../include/textparser.hpp"
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

if(WIN32)
    target_sources(libtextparser PRIVATE ${RC_DLL_OUT_FILE})
endif()

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
    target_include_directories(libtextparser-json PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/json-c>
    )
endif()

target_link_libraries(libtextparser-json PUBLIC libtextparser)

if(WIN32)
    target_link_directories(libtextparser-json PRIVATE ../bin)
    target_link_libraries(libtextparser-json PRIVATE json-c.lib)
else()
    target_link_libraries(libtextparser-json PRIVATE
        json-c::json-c
    )
endif()

set_target_properties(libtextparser-json PROPERTIES
    PUBLIC_HEADER "../include/textparser-json.h"
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser-json"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

if(WIN32)
    target_sources(libtextparser-json PRIVATE ${RC_DLL_OUT_FILE})
endif()

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
