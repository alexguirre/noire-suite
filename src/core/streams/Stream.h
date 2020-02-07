#pragma once
#include "Common.h"

namespace noire
{
	enum class StreamSeekOrigin
	{
		Begin = 0,
		Current,
		End
	};

	class Stream
	{
	public:
		// Returns number of bytes read
		virtual size Read(void* dstBuffer, size count) = 0;
		virtual size ReadAt(void* dstBuffer, size count, size offset) = 0;

		virtual void Write(const void* buffer, size count) = 0;
		virtual void WriteAt(const void* buffer, size count, size offset) = 0;

		// Returns the new position within the stream
		virtual size Seek(ptrdiff offset, StreamSeekOrigin origin) = 0;

		virtual size Tell() = 0;

		virtual size Size() = 0;
	};
}