#include "crc.cuh"
#include <cstddef>
#include <cstdint>
#include <cuda_runtime_api.h>
#include <iostream>
#include <math.h>
#include <random>

template<std::size_t N>
struct fixedString
{
	char buffer[N];

	__host__ __device__ std::size_t capacity() const { return N; }
	__host__ __device__ std::size_t size() const
	{
		std::size_t i = 0;
		while (i < N && buffer[i])
		{
			i++;
		}

		return i;
	}
};

template<std::size_t N, std::size_t StrSize>
struct fixedStringArray
{
	fixedString<StrSize> strings[N];

	__host__ __device__ std::size_t capacity() const { return N; }
	__host__ __device__ std::size_t size() const { return N; }
};

using stringArray = fixedStringArray<8192*128, 64>;

// function to add the elements of two
__global__ void calcHashes(stringArray* strings, std::uint32_t* hashes)
{
	const int index = blockIdx.x * blockDim.x + threadIdx.x;
	const int stride = blockDim.x * gridDim.x;

	for (int i = index; i < strings->size(); i += stride)
	{
		hashes[i] = crc32(strings->strings[i].buffer, strings->strings[i].size());
	}
}

int main(void)
{
	stringArray* strings;
	cudaMallocManaged(&strings, sizeof(*strings));
	cudaMemset(strings, 0, sizeof(*strings));

	std::uint32_t* hashes;
	cudaMallocManaged(&hashes, strings->size() * sizeof(*hashes));
	cudaMemset(hashes, 0, strings->size() * sizeof(*hashes));

	std::random_device rndDev{};
	std::mt19937 rndEng{ rndDev() };
	std::uniform_int_distribution<std::mt19937::result_type> rndDist(48, 122);

	for (std::size_t i = 0; i < strings->size(); i++)
	{
		auto& s = strings->strings[i];
		for (std::size_t j = 0; j < s.capacity() - 1; j++)
		{
			s.buffer[j] = rndDist(rndEng);
		}
	}

	// Run kernel on 1M elements on the CPU
	int BlockSize = 256;
	int NumBlocks = (strings->size() + BlockSize - 1) / BlockSize;
	calcHashes<<<NumBlocks, BlockSize>>>(strings, hashes);

	cudaDeviceSynchronize();

	/*for (std::size_t i = 0; i < strings->size(); i++)
	{
		auto& s = strings->strings[i];
		printf("#%d: %s = 0x%08X\n", i, s.buffer, hashes[i]);
	}*/

	// Free memory
	cudaFree(strings);
	cudaFree(hashes);

	return 0;
}