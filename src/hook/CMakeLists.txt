cmake_minimum_required(VERSION 3.12)

set(HOOK_SOURCES
    "main.cpp"
)

add_library(noire-hook SHARED
    ${HOOK_SOURCES}
)
