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

	class FileStream final : public Stream
	{
	public:
		FileStream(const std::filesystem::path& path);
		FileStream(TempFileTag);
		~FileStream() override;

		FileStream(const FileStream&) = delete;
		FileStream(FileStream&&) noexcept;

		FileStream& operator=(const FileStream&) = delete;
		FileStream& operator=(FileStream&&) noexcept;

		u64 Read(void* dstBuffer, u64 count) override;
		u64 ReadAt(void* dstBuffer, u64 count, u64 offset) override;

		u64 Write(const void* buffer, u64 count) override;
		u64 WriteAt(const void* buffer, u64 count, u64 offset) override;

		u64 Seek(i64 offset, StreamSeekOrigin origin) override;

		u64 Tell() override;

		u64 Size() override;

		const std::filesystem::path& Path() const { return mPath; }

	private:
		std::filesystem::path mPath;
		HANDLE mHandle;
	};
}
