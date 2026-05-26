cmake_minimum_required(VERSION 3.19)
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

find_library(QSCINTILLA_LIBRARY NAMES qscintilla2_qt6 qscintilla2-qt6)
if(NOT QSCINTILLA_LIBRARY)
    message(WARNING "QScintilla QT6 library not found! Compiler link might fail.")
    set(QSCINTILLA_LIBRARY qscintilla2_qt6) # Fallback to original
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
