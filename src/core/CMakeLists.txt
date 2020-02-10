cmake_minimum_required(VERSION 3.12)

file(GLOB CORE_SOURCES
    "Common.h"
    "Path.cpp"
    "Path.h"
    "VFS.cpp"
    "VFS.h"
    "devices/Device.cpp"
    "devices/Device.h"
    "files/File.cpp"
    "files/File.h"
    "files/WAD.cpp"
    "files/WAD.h"
    "streams/FileStream.cpp"
    "streams/FileStream.h"
    "streams/MemoryStream.cpp"
    "streams/MemoryStream.h"
    "streams/Stream.cpp"
    "streams/Stream.h"
    "streams/TempStream.cpp"
    "streams/TempStream.h"
)
file(GLOB CORE_TEST_SOURCES
    ${CORE_SOURCES}
    "tests/main.cpp"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${CORE_SOURCES} ${CORE_TEST_SOURCES})

add_library(noire-core STATIC
    ${CORE_SOURCES}
)

add_executable(noire-core-test
    ${CORE_TEST_SOURCES}
)

find_package(doctest CONFIG REQUIRED)
if(NOT doctest_FOUND)
    message(FATAL_ERROR "doctest not found")
endif()

target_compile_definitions(noire-core PRIVATE DOCTEST_CONFIG_DISABLE)


target_include_directories(noire-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MSGSL_INCLUDE_DIR})
target_include_directories(noire-core-test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MSGSL_INCLUDE_DIR})

target_link_libraries(noire-core PRIVATE
    doctest::doctest
    d3dcompiler
)

target_link_libraries(noire-core-test PRIVATE
    doctest::doctest
    d3dcompiler
)
