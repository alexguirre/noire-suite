#include "NativeDevice.h"
#include "FileSystem.h"
#include <algorithm>
#include <gsl/gsl>

namespace noire::fs
{
	namespace stdfs = std::filesystem;

	CNativeDevice::CNativeDevice(const stdfs::path& rootDir) : mRootDir{ stdfs::absolute(rootDir) }
	{
		Expects(stdfs::exists(mRootDir) && stdfs::is_directory(mRootDir));
	}

	bool CNativeDevice::PathExists(std::string_view path) const
	{
		const stdfs::path fullPath = mRootDir / path;
		return stdfs::exists(fullPath);
	}

	bool CNativeDevice::FileExists(std::string_view filePath) const
	{
		const stdfs::path fullPath = mRootDir / filePath;
		return stdfs::is_regular_file(fullPath);
	}

	bool CNativeDevice::DirectoryExists(std::string_view dirPath) const
	{
		const stdfs::path fullPath = mRootDir / dirPath;
		return stdfs::is_directory(fullPath);
	}

	FileStreamSize CNativeDevice::FileSize(std::string_view filePath)
	{
		Expects(FileExists(filePath));

		const stdfs::path fullPath = mRootDir / filePath;
		return gsl::narrow<FileStreamSize>(stdfs::file_size(fullPath));
	}

	std::unique_ptr<IFileStream> CNativeDevice::OpenFile(std::string_view path)
	{
		const stdfs::path fullPath = mRootDir / path;
		return std::make_unique<CNativeFileStream>(fullPath);
	}

	std::vector<SDirectoryEntry> CNativeDevice::GetAllEntries()
	{
		std::vector<SDirectoryEntry> entries{};

		for (auto& p : stdfs::recursive_directory_iterator(mRootDir))
		{
			const bool isFile = p.is_regular_file();
			// normalize path
			std::string path = p.path().lexically_relative(mRootDir).string();
			std::replace(path.begin(), path.end(), '\\', CFileSystem::DirectorySeparator);
			if (!isFile)
			{
				path += CFileSystem::DirectorySeparator;
			}

			entries.emplace_back(this, path, isFile);
		}

		return entries;
	}

	std::vector<SDirectoryEntry> CNativeDevice::GetEntries(std::string_view dirPath)
	{
		const stdfs::path fullPath = mRootDir / dirPath;
		Expects(stdfs::is_directory(fullPath));

		std::vector<SDirectoryEntry> entries{};

		for (auto& e : stdfs::directory_iterator(fullPath))
		{
			const bool isFile = e.is_regular_file();
			// normalize path
			std::string path = e.path().lexically_relative(mRootDir).string();
			std::replace(path.begin(), path.end(), '\\', CFileSystem::DirectorySeparator);
			if (!isFile)
			{
				path += CFileSystem::DirectorySeparator;
			}

			entries.emplace_back(this, path, isFile);
		}

		return entries;
	}

	CNativeFileStream::CNativeFileStream(const stdfs::path& path)
		: mStream{ path, std::ios::binary | std::ios::in }
	{
	}

	void CNativeFileStream::Read(void* destBuffer, FileStreamSize count)
	{
		mStream.read(reinterpret_cast<char*>(destBuffer), count);
	}

	void CNativeFileStream::Seek(FileStreamSize offset) { mStream.seekg(offset); }

	FileStreamSize CNativeFileStream::Tell()
	{
		return gsl::narrow<FileStreamSize>(mStream.tellg());
	}

	FileStreamSize CNativeFileStream::Size()
	{
		const std::streampos pos = mStream.tellg();
		mStream.seekg(0, std::ios::end);
		const std::streampos size = mStream.tellg();
		mStream.seekg(pos);
		return gsl::narrow<FileStreamSize>(size);
	};
}