#include "Stream.h"
#include <algorithm>

namespace noire
{
	void Stream::CopyTo(Stream& stream)
	{
		constexpr size BufferSize{ 81920 };
		std::unique_ptr<u8[]> buffer = std::make_unique<u8[]>(BufferSize);

		u64 read = 0;
		u64 offset = 0;
		while ((read = ReadAt(buffer.get(), BufferSize, offset)) != 0)
		{
			stream.Write(buffer.get(), read);

			offset += read;
		}
	}

	SubStream::SubStream(Stream& baseStream, u64 offset, u64 size)
		: mBaseStream{ baseStream }, mOffset{ offset }, mSize{ size }, mReadingOffset{ 0 }
	{
		Expects(offset + size <= baseStream.Size());
	}

	u64 SubStream::Read(void* dstBuffer, u64 count)
	{
		const u64 read = ReadAt(dstBuffer, count, mReadingOffset);
		mReadingOffset += read;
		return read;
	}

	u64 SubStream::ReadAt(void* dstBuffer, u64 count, u64 offset)
	{
		Expects(dstBuffer);

		if (const u64 maxCount = (mSize - offset); count > maxCount)
		{
			count = maxCount;
		}

		const u64 baseOffset = mOffset + offset;
		return mBaseStream.ReadAt(dstBuffer, count, baseOffset);
	}

	u64 SubStream::Seek(i64 offset, StreamSeekOrigin origin)
	{
		switch (origin)
		{
		case StreamSeekOrigin::Begin: break;
		case StreamSeekOrigin::Current: offset += mReadingOffset; break;
		case StreamSeekOrigin::End: offset += mSize; break;
		default: Expects(false);
		}

		mReadingOffset = gsl::narrow<u64>(std::clamp<i64>(offset, 0, mSize));

		return mReadingOffset;
	}

	u64 SubStream::Tell() { return mReadingOffset; }

	u64 SubStream::Size() { return mSize; }

	ReadOnlyStream::ReadOnlyStream(std::unique_ptr<Stream> baseStream)
		: mBaseStream{ std::move(baseStream) }
	{
		Expects(mBaseStream != nullptr);
	}

	u64 ReadOnlyStream::Read(void* dstBuffer, u64 count)
	{
		return mBaseStream->Read(dstBuffer, count);
	}

	u64 ReadOnlyStream::ReadAt(void* dstBuffer, u64 count, u64 offset)
	{
		return mBaseStream->ReadAt(dstBuffer, count, offset);
	}

	u64 ReadOnlyStream::Write(const void*, u64) { return 0; }

	u64 ReadOnlyStream::WriteAt(const void*, u64, u64) { return 0; }

	u64 ReadOnlyStream::Seek(i64 offset, StreamSeekOrigin origin)
	{
		return mBaseStream->Seek(offset, origin);
	}

	u64 ReadOnlyStream::Tell() { return mBaseStream->Tell(); }

	u64 ReadOnlyStream::Size() { return mBaseStream->Size(); }

	EmptyStream::EmptyStream() {}
	u64 EmptyStream::Read(void*, u64) { return 0; }
	u64 EmptyStream::ReadAt(void*, u64, u64) { return 0; }
	u64 EmptyStream::Write(const void*, u64) { return 0; }
	u64 EmptyStream::WriteAt(const void*, u64, u64) { return 0; }
	u64 EmptyStream::Seek(i64, StreamSeekOrigin) { return 0; }
	u64 EmptyStream::Tell() { return 0; }
	u64 EmptyStream::Size() { return 0; }
}
