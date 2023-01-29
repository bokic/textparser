cmake_minimum_required(VERSION 3.5)

project(qguitextparser VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

include_directories(
    .
    ../../include
    ../../definitions
)

set(PROJECT_SOURCES
        main.cpp

        dialog.cpp
        dialog.h
        dialog.ui

        qcodeeditwidget/qcodeeditwidget.cpp
        qcodeeditwidget/qcodeeditwidget.h
)

qt_add_executable(qguitextparser
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_link_libraries(qguitextparser PRIVATE
    Qt6::Widgets
    libtextparser
)

set_target_properties(qguitextparser PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS qguitextparser
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

qt_finalize_executable(qguitextparser)
