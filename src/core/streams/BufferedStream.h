#pragma once
#include "Common.h"
#include "Stream.h"
#include <memory>

namespace noire
{
	class BufferedStream final : public Stream
	{
	public:
		static constexpr size DefaultBufferSize{ 0x1000 }; // 4KiB

		BufferedStream(std::shared_ptr<Stream> baseStream, size bufferSize = DefaultBufferSize);

		u64 Read(void* dstBuffer, u64 count) override;
		u64 ReadAt(void* dstBuffer, u64 count, u64 offset) override;

		u64 Write(const void* buffer, u64 count) override;
		u64 WriteAt(const void* buffer, u64 count, u64 offset) override;

		u64 Seek(i64 offset, StreamSeekOrigin origin) override;

		u64 Tell() override;

		u64 Size() override;

	private:
		std::shared_ptr<Stream> mBaseStream;
		std::unique_ptr<byte[]> mBuffer;
		size mBufferSize;
	};
}
