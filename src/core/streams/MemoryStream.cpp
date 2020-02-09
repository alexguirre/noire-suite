#include "MemoryStream.h"
#include <algorithm>
#include <cstring>
#include <doctest/doctest.h>

namespace noire
{
	MemoryStream::MemoryStream(size initialBufferSize)
		: mBuffer{ std::make_unique<byte[]>(initialBufferSize) },
		  mBufferSize{ initialBufferSize },
		  mSize{ 0 },
		  mPosition{ 0 }
	{
	}

	u64 MemoryStream::Read(void* dstBuffer, u64 count)
	{
		const u64 read = ReadAt(dstBuffer, count, mPosition);
		mPosition += gsl::narrow<size>(read);
		return read;
	}

	u64 MemoryStream::ReadAt(void* dstBuffer, u64 count, u64 offset)
	{
		if (offset < mSize)
		{
			const size o = gsl::narrow<size>(offset);
			const size bytesToRead = std::min(gsl::narrow<size>(count), mSize - o);
			std::memcpy(dstBuffer, &mBuffer[o], bytesToRead);
			return bytesToRead;
		}
		else
		{
			return 0;
		}
	}

	u64 MemoryStream::Write(const void* buffer, u64 count)
	{
		const u64 written = WriteAt(buffer, count, mPosition);
		mPosition += gsl::narrow<size>(written);
		return written;
	}

	u64 MemoryStream::WriteAt(const void* buffer, u64 count, u64 offset)
	{
		const size c = gsl::narrow<size>(count);
		const size o = gsl::narrow<size>(offset);
		const size endOffset = o + c;

		Grow(std::max(mSize, o) + c);

		std::memcpy(&mBuffer[o], buffer, c);
		if (endOffset > mSize)
		{
			mSize = endOffset;
		}
		return c;
	}

	u64 MemoryStream::Seek(i64 offset, StreamSeekOrigin origin)
	{
		switch (origin)
		{
		case StreamSeekOrigin::Begin: break;
		case StreamSeekOrigin::Current: offset += mPosition; break;
		case StreamSeekOrigin::End: offset += mSize; break;
		default: Expects(false);
		}

		mPosition = gsl::narrow<size>(offset);
		if (mPosition > mSize)
		{
			mSize = mPosition;
			Grow(mSize);
		}

		return mPosition;
	}

	u64 MemoryStream::Tell() { return mPosition; }

	u64 MemoryStream::Size() { return mSize; }

	void MemoryStream::Grow(size minSize)
	{
		if (mBufferSize >= minSize)
		{
			return;
		}

		const size newSize = std::max(minSize, mBufferSize + MinBytesToGrow);
		std::unique_ptr<byte[]> newBuffer = std::make_unique<byte[]>(newSize);

		std::memcpy(newBuffer.get(), mBuffer.get(), mSize);

		mBuffer.swap(newBuffer);
		mBufferSize = newSize;
	}
}

TEST_SUITE("MemoryStream")
{
	using namespace noire;

	void TestStream(MemoryStream & s)
	{
		{
			u8 data[]{ 0, 1, 2, 3 };

			CHECK_EQ(s.Tell(), 0);
			CHECK_EQ(s.Write(data, std::size(data)), std::size(data));
			CHECK_EQ(s.Size(), 4);

			CHECK_EQ(s.Write(data, std::size(data)), std::size(data));
			CHECK_EQ(s.Size(), 8);

			CHECK_EQ(s.Write(data, std::size(data)), std::size(data));
			CHECK_EQ(s.Size(), 12);

			CHECK_EQ(s.Tell(), 12);
			CHECK_EQ(s.Seek(0, StreamSeekOrigin::Begin), 0);
			CHECK_EQ(s.Tell(), 0);
		}

		{
			u8 readData[12]{ 0 };
			u8 expectedReadData[12]{ 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3 };

			CHECK_EQ(s.Read(readData, std::size(readData)), std::size(readData));
			CHECK_EQ(std::memcmp(readData, expectedReadData, std::size(readData)), 0);
			CHECK_EQ(s.Tell(), 12);
		}

		{
			u8 data[8]{ 7, 6, 5, 4, 3, 2, 1, 0 };
			CHECK_EQ(s.WriteAt(data, std::size(data), 2), std::size(data));

			u8 readData[8]{ 0 };
			CHECK_EQ(s.ReadAt(readData, std::size(readData), 2), std::size(readData));

			CHECK_EQ(std::memcmp(readData, data, std::size(readData)), 0);
		}
	}

	TEST_CASE("Basic")
	{
		MemoryStream s{};
		CHECK_EQ(s.BufferSize(), MemoryStream::DefaultBufferSize);
		TestStream(s);
	}

	TEST_CASE("WriteAt greater than current Size")
	{
		MemoryStream s{ 4 };
		CHECK_EQ(s.BufferSize(), 4);

		constexpr u64 offset{ 32 };
		u8 data[8]{ 7, 6, 5, 4, 3, 2, 1, 0 };
		CHECK_EQ(s.WriteAt(data, std::size(data), offset), std::size(data));
		CHECK_EQ(s.Size(), offset + std::size(data));
		CHECK_EQ(s.BufferSize(), 4 + MemoryStream::MinBytesToGrow);

		u8 readData[8]{ 0 };
		CHECK_EQ(s.ReadAt(readData, std::size(readData), 32), std::size(readData));

		CHECK_EQ(std::memcmp(readData, data, std::size(readData)), 0);
	}

	TEST_CASE("Seek greater than current Size")
	{
		MemoryStream s{ 4 };
		CHECK_EQ(s.BufferSize(), 4);

		CHECK_EQ(s.Seek(0x100, StreamSeekOrigin::Begin), 0x100);
		CHECK_EQ(s.Tell(), 0x100);
		CHECK_EQ(s.BufferSize(), 4 + MemoryStream::MinBytesToGrow);

		u8 data[]{ 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 };
		CHECK_EQ(s.Write(data, std::size(data)), std::size(data));
		CHECK_EQ(s.Tell(), 0x100 + std::size(data));

		CHECK_EQ(s.Seek(-gsl::narrow<i64>(std::size(data)), StreamSeekOrigin::Current), 0x100);
		CHECK_EQ(s.Tell(), 0x100);

		u8 readData[std::size(data)]{ 0 };
		CHECK_EQ(s.Read(readData, std::size(readData)), std::size(readData));
		CHECK_EQ(s.Tell(), 0x100 + std::size(readData));

		CHECK_EQ(std::memcmp(readData, data, std::size(readData)), 0);
	}

	TEST_CASE("Read greater than current Size")
	{
		MemoryStream s{};

		u8 data[16]{};
		CHECK_EQ(s.Tell(), 0);
		CHECK_EQ(s.Read(data, std::size(data)), 0);
		CHECK_EQ(s.Tell(), 0);
		CHECK_EQ(s.ReadAt(data, std::size(data), 0), 0);
		CHECK_EQ(s.ReadAt(data, std::size(data), 0x100), 0);
	}
}
