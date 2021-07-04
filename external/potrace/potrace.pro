CONFIG -= c++ qt

INCLUDEPATH += $$PWD/src

TEMPLATE = lib

TARGET = potrace

SOURCES = \
    src/potracelib.c \
    src/decompose.c \
    src/trace.c \
    src/curve.c

DEFINES += VERSION=\\\"1.16\\\"
