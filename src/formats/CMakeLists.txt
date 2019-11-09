cmake_minimum_required(VERSION 3.12)

file(GLOB FORMATS_SOURCES
    "ContainerFile.cpp"
    "ContainerFile.h"
    "File.h"
    "WADFile.cpp"
    "WADFile.h"
    "fs/ContainerDevice.cpp"
    "fs/ContainerDevice.h"
    "fs/Device.h"
    "fs/FileStream.cpp"
    "fs/FileStream.h"
    "fs/FileSystem.cpp"
    "fs/FileSystem.h"
    "fs/NativeDevice.cpp"
    "fs/NativeDevice.h"
    "fs/Path.cpp"
    "fs/Path.h"
    "fs/WADDevice.cpp"
    "fs/WADDevice.h"
)
file(GLOB FORMATS_TEST_SOURCES   
    ${FORMATS_SOURCES}
    "tests/main.cpp"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${FORMATS_SOURCES} ${FORMATS_TEST_SOURCES})

add_library(noire-formats STATIC
    ${FORMATS_SOURCES}
)

add_executable(noire-formats-test
    ${FORMATS_TEST_SOURCES}
)

find_package(doctest CONFIG REQUIRED)
if(NOT doctest_FOUND)
    message(FATAL_ERROR "doctest not found")
endif()

target_compile_definitions(noire-formats PRIVATE DOCTEST_CONFIG_DISABLE)

target_include_directories(noire-formats PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MSGSL_INCLUDE_DIR})
target_include_directories(noire-formats-test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MSGSL_INCLUDE_DIR})

target_link_libraries(noire-formats PRIVATE
    doctest::doctest
)

target_link_libraries(noire-formats-test PRIVATE
    doctest::doctest
)
