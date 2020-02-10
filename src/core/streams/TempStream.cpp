#include "TempStream.h"
#include <algorithm>
#include <cstring>
#include <doctest/doctest.h>

namespace noire
{
	TempStream::TempStream(size maxMemoryBufferSize)
		: mMaxMemoryBufferSize{ maxMemoryBufferSize }, mStream{ MemoryStream{} }
	{
	}

	u64 TempStream::Read(void* dstBuffer, u64 count)
	{
		return std::visit([dstBuffer, count](Stream& s) { return s.Read(dstBuffer, count); },
						  mStream);
	}

	u64 TempStream::ReadAt(void* dstBuffer, u64 count, u64 offset)
	{
		return std::visit(
			[dstBuffer, count, offset](Stream& s) { return s.ReadAt(dstBuffer, count, offset); },
			mStream);
	}

	u64 TempStream::Write(const void* buffer, u64 count)
	{
		CheckUsage();
		return std::visit([buffer, count](Stream& s) { return s.Write(buffer, count); }, mStream);
	}

	u64 TempStream::WriteAt(const void* buffer, u64 count, u64 offset)
	{
		CheckUsage();
		return std::visit(
			[buffer, count, offset](Stream& s) { return s.WriteAt(buffer, count, offset); },
			mStream);
	}

	u64 TempStream::Seek(i64 offset, StreamSeekOrigin origin)
	{
		return std::visit([offset, origin](Stream& s) { return s.Seek(offset, origin); }, mStream);
	}

	u64 TempStream::Tell()
	{
		return std::visit([](Stream& s) { return s.Tell(); }, mStream);
	}

	u64 TempStream::Size()
	{
		return std::visit([](Stream& s) { return s.Size(); }, mStream);
	}

	bool TempStream::IsUsingTempFile() const
	{
		constexpr size FileStreamIndex{ 1 };
		return mStream.index() == FileStreamIndex;
	}

	void TempStream::CheckUsage()
	{
		constexpr size MemoryStreamIndex{ 0 };

		if (mStream.index() == MemoryStreamIndex && Size() >= mMaxMemoryBufferSize)
		{
			// switch to temp file
			FileStream f{ TempFile };
			MemoryStream& m = std::get<MemoryStream>(mStream);
			m.CopyTo(f);
			f.Seek(gsl::narrow<i64>(m.Tell()), StreamSeekOrigin::Begin);

			Ensures(m.Size() == f.Size());

			mStream = std::move(f);
		}
	}
}

TEST_SUITE("TempStream")
{
	using namespace noire;

	void TestStream(TempStream & s)
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
		TempStream s{};
		CHECK_FALSE(s.IsUsingTempFile());
		TestStream(s);
		CHECK_FALSE(s.IsUsingTempFile());
	}

	TEST_CASE("Basic, switching to temp file")
	{
		TempStream s{ 4 };
		CHECK_FALSE(s.IsUsingTempFile());
		TestStream(s);
		CHECK(s.IsUsingTempFile());
	}

	TEST_CASE("WriteAt greater than current Size")
	{
		TempStream s{};

		constexpr u64 offset{ 32 };
		u8 data[8]{ 7, 6, 5, 4, 3, 2, 1, 0 };
		CHECK_EQ(s.WriteAt(data, std::size(data), offset), std::size(data));
		CHECK_EQ(s.Size(), offset + std::size(data));
		CHECK_FALSE(s.IsUsingTempFile());

		u8 readData[8]{ 0 };
		CHECK_EQ(s.ReadAt(readData, std::size(readData), 32), std::size(readData));
		CHECK_FALSE(s.IsUsingTempFile());

		CHECK_EQ(std::memcmp(readData, data, std::size(readData)), 0);
	}

	TEST_CASE("Seek greater than current Size")
	{
		TempStream s{};

		CHECK_EQ(s.Seek(0x100, StreamSeekOrigin::Begin), 0x100);
		CHECK_EQ(s.Tell(), 0x100);
		CHECK_FALSE(s.IsUsingTempFile());

		u8 data[]{ 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 };
		CHECK_EQ(s.Write(data, std::size(data)), std::size(data));
		CHECK_EQ(s.Tell(), 0x100 + std::size(data));
		CHECK_FALSE(s.IsUsingTempFile());

		CHECK_EQ(s.Seek(-gsl::narrow<i64>(std::size(data)), StreamSeekOrigin::Current), 0x100);
		CHECK_EQ(s.Tell(), 0x100);
		CHECK_FALSE(s.IsUsingTempFile());

		u8 readData[std::size(data)]{ 0 };
		CHECK_EQ(s.Read(readData, std::size(readData)), std::size(readData));
		CHECK_EQ(s.Tell(), 0x100 + std::size(readData));
		CHECK_FALSE(s.IsUsingTempFile());

		CHECK_EQ(std::memcmp(readData, data, std::size(readData)), 0);
	}

	TEST_CASE("Read greater than current Size")
	{
		TempStream s{};

		u8 data[16]{};
		CHECK_EQ(s.Tell(), 0);
		CHECK_EQ(s.Read(data, std::size(data)), 0);
		CHECK_EQ(s.Tell(), 0);
		CHECK_EQ(s.ReadAt(data, std::size(data), 0), 0);
		CHECK_EQ(s.ReadAt(data, std::size(data), 0x100), 0);
		CHECK_FALSE(s.IsUsingTempFile());
	}
}
