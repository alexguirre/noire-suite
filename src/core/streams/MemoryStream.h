#pragma once
#include "Common.h"
#include "Stream.h"
#include <memory>

namespace noire
{
	class MemoryStream final : public Stream
	{
		// NOTE: memory not written to contains garbage
	public:
		static constexpr size DefaultBufferSize{ 0x2000 };
		static constexpr size MinBytesToGrow{ 0x1000 };

		MemoryStream(size initialBufferSize = DefaultBufferSize);

		MemoryStream(const MemoryStream&) = delete;
		MemoryStream(MemoryStream&&) = default;

		MemoryStream& operator=(const MemoryStream&) = delete;
		MemoryStream& operator=(MemoryStream&&) = default;

		u64 Read(void* dstBuffer, u64 count) override;
		u64 ReadAt(void* dstBuffer, u64 count, u64 offset) override;

		u64 Write(const void* buffer, u64 count) override;
		u64 WriteAt(const void* buffer, u64 count, u64 offset) override;

		u64 Seek(i64 offset, StreamSeekOrigin origin) override;

		u64 Tell() override;

		u64 Size() override;

		void Grow(size minSize);
		inline size BufferSize() const { return mBufferSize; };

	private:
		std::unique_ptr<byte[]> mBuffer;
		size mBufferSize;
		size mSize;
		size mPosition;
	};
}
