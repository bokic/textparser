cmake_minimum_required(VERSION 3.19)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(qscintilla LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(qscintilla
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

set_target_properties(qscintilla PROPERTIES
    OUTPUT_NAME demo-qscintilla
)

target_compile_options(qscintilla PRIVATE
    -Wno-pedantic
)

find_library(QSCINTILLA_LIBRARY NAMES qscintilla2_qt6 qscintilla2-qt6)
if(NOT QSCINTILLA_LIBRARY)
    message(WARNING "QScintilla QT6 library not found!")
    return()
endif()

target_link_libraries(qscintilla
    PRIVATE
        Qt::Core
        Qt::Widgets
        ${QSCINTILLA_LIBRARY}
        libtextparser
)

include(GNUInstallDirs)

install(TARGETS qscintilla
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET qscintilla
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
