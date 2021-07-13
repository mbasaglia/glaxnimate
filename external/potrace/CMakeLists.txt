project(potrace C)

include_directories(src/)

set(SOURCES
    src/potracelib.c
    src/decompose.c
    src/trace.c
    src/curve.c
)

add_compile_definitions(VERSION="1.16")

add_library(potrace ${SOURCES})
