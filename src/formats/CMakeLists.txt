cmake_minimum_required(VERSION 3.12)

set(FORMATS_SOURCES
    "File.h"
    "WADFile.cpp"
    "WADFile.h"
)

add_library(noire-formats STATIC
    ${FORMATS_SOURCES}
)

target_include_directories(noire-formats PUBLIC ${MSGSL_INCLUDE_DIR})
