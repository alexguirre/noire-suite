#pragma once
#include <cstddef>
#include <cstdint>
#include <cuda_runtime_api.h>

__device__ std::uint32_t
crc32Partial(const char* data, std::size_t dataLength, std::uint32_t initialHash = 0xFFFFFFFF);
__device__ std::uint32_t
crc32(const char* data, std::size_t dataLength, std::uint32_t initialHash = 0xFFFFFFFF);