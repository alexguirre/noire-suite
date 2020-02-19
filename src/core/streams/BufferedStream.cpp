#include "BufferedStream.h"

namespace noire
{
	BufferedStream::BufferedStream(std::shared_ptr<Stream> baseStream, size bufferSize)
		: mBaseStream{ baseStream }, mBuffer{ nullptr }, mBufferSize{ bufferSize }
	{
		Expects(baseStream != nullptr);
	}

	u64 BufferedStream::Read(void* dstBuffer, u64 count) {}

	u64 BufferedStream::ReadAt(void* dstBuffer, u64 count, u64 offset) {}

	
	u64 BufferedStream::Write(const void* buffer, u64 count){};
	u64 BufferedStream::WriteAt(const void* buffer, u64 count, u64 offset) {}

	u64 BufferedStream::Seek(i64 offset, StreamSeekOrigin origin) {}

	u64 BufferedStream::Tell() {}

	u64 BufferedStream::Size() {}
}
