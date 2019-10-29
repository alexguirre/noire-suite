cmake_minimum_required(VERSION 3.12)

file(GLOB HOOK_SOURCES
    "main.cpp"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HOOK_SOURCES})

add_library(noire-hook SHARED
    ${HOOK_SOURCES}
)

target_include_directories(noire-hook PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
