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

	bool CNativeDevice::PathExists(SPathView path) const
	{
		const stdfs::path fullPath = mRootDir / path.String();
		return stdfs::exists(fullPath);
	}

	bool CNativeDevice::FileExists(SPathView filePath) const
	{
		const stdfs::path fullPath = mRootDir / filePath.String();
		return stdfs::is_regular_file(fullPath);
	}

	bool CNativeDevice::DirectoryExists(SPathView dirPath) const
	{
		const stdfs::path fullPath = mRootDir / dirPath.String();
		return stdfs::is_directory(fullPath);
	}

	FileStreamSize CNativeDevice::FileSize(SPathView filePath)
	{
		Expects(FileExists(filePath));

		const stdfs::path fullPath = mRootDir / filePath.String();
		return gsl::narrow<FileStreamSize>(stdfs::file_size(fullPath));
	}

	std::unique_ptr<IFileStream> CNativeDevice::OpenFile(SPathView path)
	{
		const stdfs::path fullPath = mRootDir / path.String();
		return std::make_unique<CNativeFileStream>(fullPath);
	}

	std::vector<SDirectoryEntry> CNativeDevice::GetAllEntries()
	{
		std::vector<SDirectoryEntry> entries{};

		for (auto& p : stdfs::recursive_directory_iterator(mRootDir))
		{
			const bool isFile = p.is_regular_file();
			SPath path{ p.path().lexically_relative(mRootDir).string() };
			path.Normalize();
			if (!isFile)
			{
				path += SPath::DirectorySeparator;
			}

			entries.emplace_back(this,
								 path,
								 isFile ? EDirectoryEntryType::File :
										  EDirectoryEntryType::Directory);
		}

		return entries;
	}

	std::vector<SDirectoryEntry> CNativeDevice::GetEntries(SPathView dirPath)
	{
		const stdfs::path fullPath = mRootDir / dirPath.String();
		Expects(stdfs::is_directory(fullPath));

		std::vector<SDirectoryEntry> entries{};

		for (auto& e : stdfs::directory_iterator(fullPath))
		{
			const bool isFile = e.is_regular_file();
			SPath path{ e.path().lexically_relative(mRootDir).string() };
			path.Normalize();
			if (!isFile)
			{
				path += SPath::DirectorySeparator;
			}

			entries.emplace_back(this,
								 path,
								 isFile ? EDirectoryEntryType::File :
										  EDirectoryEntryType::Directory);
		}

		return entries;
	}

	// NOTE: temp variable for debuggin
	static std::size_t Num{ 0 };

	CNativeFileStream::CNativeFileStream(const stdfs::path& path)
		: mStream{ path, std::ios::binary | std::ios::in }
	{
		Expects(stdfs::is_regular_file(path));

		Ensures(mStream.is_open());

		Num++;
	}

	CNativeFileStream::~CNativeFileStream() { Num--; }

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