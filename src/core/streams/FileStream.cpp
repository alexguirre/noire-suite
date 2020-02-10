#include "FileStream.h"
#include <cstring>
#include <doctest/doctest.h>
#include <iostream>
#include <iterator>

namespace noire
{
	FileStream::FileStream(const std::filesystem::path& path)
		: mHandle{ CreateFileW(path.native().c_str(),
							   FILE_GENERIC_READ | FILE_GENERIC_WRITE,
							   FILE_SHARE_READ,
							   nullptr,
							   OPEN_ALWAYS,
							   FILE_ATTRIBUTE_NORMAL,
							   nullptr) },
		  mPath{ path }
	{
		Ensures(mHandle != INVALID_HANDLE_VALUE);
	}

	FileStream::FileStream(TempFileTag) : mHandle{ INVALID_HANDLE_VALUE }, mPath{}
	{
		if (wchar_t tempPath[MAX_PATH]; GetTempPathW(std::size(tempPath), tempPath))
		{
			if (wchar_t fileName[MAX_PATH]; GetTempFileNameW(tempPath, L"noi", 0, fileName))
			{
				mHandle = CreateFileW(fileName,
									  GENERIC_READ | GENERIC_WRITE,
									  0,
									  nullptr,
									  CREATE_ALWAYS,
									  FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
									  nullptr);
				mPath = fileName;
			}
		}

		Ensures(mHandle != INVALID_HANDLE_VALUE);
	}

	FileStream::~FileStream() { CloseHandle(mHandle); }

	u64 FileStream::Read(void* dstBuffer, u64 count)
	{
		// NOTE: limited to 4gb
		if (DWORD bytesRead;
			ReadFile(mHandle, dstBuffer, gsl::narrow<DWORD>(count), &bytesRead, nullptr))
		{
			return bytesRead;
		}
		else
		{
			return static_cast<u64>(0);
		}
	}

	u64 FileStream::ReadAt(void* dstBuffer, u64 count, u64 offset)
	{
		OVERLAPPED ol = { 0 };
		ol.Offset = static_cast<DWORD>(offset);
		ol.OffsetHigh = static_cast<DWORD>(offset >> 32);

		// NOTE: limited to 4gb
		const u64 oldPos = Tell();
		if (DWORD bytesRead;
			ReadFile(mHandle, dstBuffer, gsl::narrow<DWORD>(count), &bytesRead, &ol))
		{
			Seek(gsl::narrow<i64>(oldPos), StreamSeekOrigin::Begin);
			return bytesRead;
		}
		else
		{
			return static_cast<u64>(0);
		}
	}

	u64 FileStream::Write(const void* buffer, u64 count)
	{
		// NOTE: limited to 4gb
		if (DWORD bytesWritten;
			WriteFile(mHandle, buffer, gsl::narrow<DWORD>(count), &bytesWritten, nullptr))
		{
			return bytesWritten;
		}
		else
		{
			return static_cast<u64>(0);
		}
	}

	u64 FileStream::WriteAt(const void* buffer, u64 count, u64 offset)
	{
		OVERLAPPED ol = { 0 };
		ol.Offset = static_cast<DWORD>(offset);
		ol.OffsetHigh = static_cast<DWORD>(offset >> 32);

		// NOTE: limited to 4gb
		const u64 oldPos = Tell();
		if (DWORD bytesWritten;
			WriteFile(mHandle, buffer, gsl::narrow<DWORD>(count), &bytesWritten, &ol))
		{
			Seek(gsl::narrow<i64>(oldPos), StreamSeekOrigin::Begin);
			return bytesWritten;
		}
		else
		{
			return static_cast<u64>(0);
		}
	}

	u64 FileStream::Seek(i64 offset, StreamSeekOrigin origin)
	{
		DWORD moveMethod;
		switch (origin)
		{
		case StreamSeekOrigin::Begin: moveMethod = FILE_BEGIN; break;
		case StreamSeekOrigin::Current: moveMethod = FILE_CURRENT; break;
		case StreamSeekOrigin::End: moveMethod = FILE_END; break;
		default: Expects(false);
		}

		LARGE_INTEGER o;
		o.QuadPart = offset;
		if (LARGE_INTEGER newPos; SetFilePointerEx(mHandle, o, &newPos, moveMethod))
		{
			return static_cast<u64>(newPos.QuadPart);
		}
		else
		{
			return static_cast<u64>(-1);
		}
	}

	u64 FileStream::Tell()
	{
		if (LARGE_INTEGER pos; SetFilePointerEx(mHandle, { 0 }, &pos, FILE_CURRENT))
		{
			return static_cast<u64>(pos.QuadPart);
		}
		else
		{
			return static_cast<u64>(-1);
		}
	}

	u64 FileStream::Size()
	{
		if (LARGE_INTEGER fsize; GetFileSizeEx(mHandle, &fsize))
		{
			return static_cast<u64>(fsize.QuadPart);
		}
		else
		{
			return static_cast<u64>(-1);
		}
	}
}

TEST_SUITE("FileStream")
{
	using namespace noire;

	void TestStream(FileStream & f)
	{
		std::cout << f.Path() << '\n';
		{
			u8 data[]{ 0, 1, 2, 3 };

			CHECK_EQ(f.Tell(), 0);
			CHECK_EQ(f.Write(data, std::size(data)), std::size(data));
			CHECK_EQ(f.Size(), 4);

			CHECK_EQ(f.Write(data, std::size(data)), std::size(data));
			CHECK_EQ(f.Size(), 8);

			CHECK_EQ(f.Write(data, std::size(data)), std::size(data));
			CHECK_EQ(f.Size(), 12);

			CHECK_EQ(f.Tell(), 12);
			CHECK_EQ(f.Seek(0, StreamSeekOrigin::Begin), 0);
			CHECK_EQ(f.Tell(), 0);
		}

		{
			u8 readData[12]{ 0 };
			u8 expectedReadData[12]{ 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3 };

			CHECK_EQ(f.Read(readData, std::size(readData)), std::size(readData));
			CHECK_EQ(std::memcmp(readData, expectedReadData, std::size(readData)), 0);
			CHECK_EQ(f.Tell(), 12);
		}

		{
			u8 data[8]{ 7, 6, 5, 4, 3, 2, 1, 0 };
			CHECK_EQ(f.WriteAt(data, std::size(data), 2), std::size(data));

			u8 readData[8]{ 0 };
			CHECK_EQ(f.ReadAt(readData, std::size(readData), 2), std::size(readData));

			CHECK_EQ(std::memcmp(readData, data, std::size(readData)), 0);
		}
	}

	TEST_CASE("Regular files")
	{
		const std::filesystem::path p{ "test" };
		{
			FileStream f{ p };
			TestStream(f);
		}
		DeleteFileW(p.native().c_str());
	}

	TEST_CASE("Temp files")
	{
		FileStream f{ TempFile };
		TestStream(f);
	}
}
