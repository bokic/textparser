TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c textparser.c
HEADERS +=        textparser.h

# Language definitions
HEADERS += cfml_definition.h

LIBS    += -lutf8proc
