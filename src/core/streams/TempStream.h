#pragma once
#include "Common.h"
#include "FileStream.h"
#include "MemoryStream.h"
#include "Stream.h"
#include <variant>

namespace noire
{
	// Stream for writing that switches from a memory buffer to temp file once its size passes a
	// specific threshold.
	class TempStream final : public Stream
	{
	public:
		static constexpr size DefaultMaxMemoryBufferSize{ 32 * 1024 * 1024 }; // 32MiB

		TempStream(size maxMemoryBufferSize = DefaultMaxMemoryBufferSize);

		TempStream(const TempStream&) = delete;
		TempStream(TempStream&&) = default;

		TempStream& operator=(const TempStream&) = delete;
		TempStream& operator=(TempStream&&) = default;

		u64 Read(void* dstBuffer, u64 count) override;
		u64 ReadAt(void* dstBuffer, u64 count, u64 offset) override;

		u64 Write(const void* buffer, u64 count) override;
		u64 WriteAt(const void* buffer, u64 count, u64 offset) override;

		u64 Seek(i64 offset, StreamSeekOrigin origin) override;

		u64 Tell() override;

		u64 Size() override;

		bool IsUsingTempFile() const;

	private:
		void CheckUsage();

		size mMaxMemoryBufferSize;
		std::variant<MemoryStream, FileStream> mStream;
	};
}
