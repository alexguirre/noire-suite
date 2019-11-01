cmake_minimum_required(VERSION 3.12)

file(GLOB FORMATS_SOURCES
    "File.h"
    "WADFile.cpp"
    "WADFile.h"
    "fs/Device.h"
    "fs/FileStream.cpp"
    "fs/FileStream.h"
    "fs/FileSystem.cpp"
    "fs/FileSystem.h"
    "fs/NativeDevice.cpp"
    "fs/NativeDevice.h"
    "fs/WADDevice.cpp"
    "fs/WADDevice.h"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${FORMATS_SOURCES})

add_library(noire-formats STATIC
    ${FORMATS_SOURCES}
)

target_include_directories(noire-formats PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MSGSL_INCLUDE_DIR})
