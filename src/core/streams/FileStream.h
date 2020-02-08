#pragma once
#include "Common.h"
#include "Stream.h"
#include <Windows.h>
#include <filesystem>

namespace noire
{
	inline constexpr struct TempFileTag
	{
	} TempFile;

	class FileStream : public Stream
	{
	public:
		FileStream(const std::filesystem::path& path);
		FileStream(TempFileTag);
		~FileStream() override;

		FileStream(const FileStream&) = delete;
		FileStream(FileStream&&) = default;

		FileStream& operator=(const FileStream&) = delete;
		FileStream& operator=(FileStream&&) = default;

		size Read(void* dstBuffer, size count) override;
		size ReadAt(void* dstBuffer, size count, size offset) override;

		size Write(const void* buffer, size count) override;
		size WriteAt(const void* buffer, size count, size offset) override;

		size Seek(ptrdiff offset, StreamSeekOrigin origin) override;

		size Tell() override;

		size Size() override;

	private:
		HANDLE mHandle;
	};
}
