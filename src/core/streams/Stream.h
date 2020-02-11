#pragma once
#include "Common.h"
#include <memory>

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
		virtual ~Stream() = default;

		// Returns number of bytes read
		virtual u64 Read(void* dstBuffer, u64 count) = 0;
		// Reads at specified offset without modifying stream position
		virtual u64 ReadAt(void* dstBuffer, u64 count, u64 offset) = 0;

		// Returns number of bytes written
		virtual u64 Write(const void* buffer, u64 count) = 0;
		// Writes at specified offset without modifying stream position
		virtual u64 WriteAt(const void* buffer, u64 count, u64 offset) = 0;

		// Returns the new position within the stream
		virtual u64 Seek(i64 offset, StreamSeekOrigin origin) = 0;

		virtual u64 Tell() = 0;

		virtual u64 Size() = 0;

		void CopyTo(Stream& stream);

		template<class T>
		T Read()
		{
			static_assert(std::is_pod_v<T>, "Expected a POD type for T");

			T tmp;
			Read(&tmp, sizeof(tmp));
			return tmp;
		}

		template<class T>
		T ReadAt(u64 offset)
		{
			static_assert(std::is_pod_v<T>, "Expected a POD type for T");

			T tmp;
			ReadAt(&tmp, sizeof(tmp), offset);
			return tmp;
		}

		template<class T>
		void Write(T v)
		{
			static_assert(std::is_pod_v<T>, "Expected a POD type for T");

			Write(&v, sizeof(v));
		}

		template<class T>
		void WriteAt(T v, u64 offset)
		{
			static_assert(std::is_pod_v<T>, "Expected a POD type for T");

			WriteAt(&v, sizeof(v), offset);
		}
	};

	// Wraps a Stream such that only a section can be read/written.
	class SubStream final : public Stream
	{
	public:
		SubStream(std::shared_ptr<Stream> baseStream, u64 offset, u64 size);

		u64 Read(void* dstBuffer, u64 count) override;
		u64 ReadAt(void* dstBuffer, u64 count, u64 offset) override;

		// not implemented
		u64 Write(const void*, u64) override
		{
			Expects(false);
			// TODO
			return 0;
		}
		u64 WriteAt(const void*, u64, u64) override
		{
			Expects(false);
			// TODO
			return 0;
		}

		u64 Seek(i64 offset, StreamSeekOrigin origin) override;

		u64 Tell() override;

		u64 Size() override;

	private:
		std::shared_ptr<Stream> mBaseStream;
		u64 mOffset;
		u64 mSize;
		u64 mReadingOffset;
	};

	// Wraps a Stream such that it can only be read. The function calls are passed to the base
	// stream, except Write/WriteAt which do nothing and always return 0 bytes written.
	class ReadOnlyStream final : public Stream
	{
	public:
		ReadOnlyStream(std::shared_ptr<Stream> baseStream);

		u64 Read(void* dstBuffer, u64 count) override;
		u64 ReadAt(void* dstBuffer, u64 count, u64 offset) override;

		u64 Write(const void* buffer, u64 count) override;
		u64 WriteAt(const void* buffer, u64 count, u64 offset) override;

		u64 Seek(i64 offset, StreamSeekOrigin origin) override;

		u64 Tell() override;

		u64 Size() override;

	private:
		std::shared_ptr<Stream> mBaseStream;
	};
}
