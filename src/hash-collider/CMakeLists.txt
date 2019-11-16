cmake_minimum_required(VERSION 3.12)

file(GLOB HASH_COLLIDER_SOURCES
    "crc.cu"
    "crc.cuh"
    "main.cu"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HASH_COLLIDER_SOURCES})

enable_language(CUDA)
add_executable(noire-hash-collider
    ${HASH_COLLIDER_SOURCES}
)

set_property(TARGET noire-hash-collider PROPERTY CUDA_SEPARABLE_COMPILATION ON)
set_property(TARGET noire-hash-collider PROPERTY CUDA_STANDARD 14)
set_property(TARGET noire-hash-collider PROPERTY CUDA_STANDARD_REQUIRED ON)
set_property(TARGET noire-hash-collider PROPERTY CUDA_RESOLVE_DEVICE_SYMBOLS ON)

target_include_directories(noire-hash-collider PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
