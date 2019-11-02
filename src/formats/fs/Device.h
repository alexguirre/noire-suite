#pragma once
#include "FileStream.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace noire::fs
{
	class IDevice;

	enum class EDirectoryEntryType
	{
		File = 0,
		Directory,
		Collection, // a file that acts like a directory
	};

	struct SDirectoryEntry
	{
		IDevice* Device;
		std::string Path;
		EDirectoryEntryType Type;

		SDirectoryEntry(IDevice* device, std::string_view path, EDirectoryEntryType type) noexcept
			: Device{ device }, Path{ path }, Type{ type }
		{
		}
	};

	class IDevice
	{
	public:
		virtual bool PathExists(std::string_view path) const = 0;
		virtual bool FileExists(std::string_view filePath) const = 0;
		virtual bool DirectoryExists(std::string_view dirPath) const = 0;
		virtual FileStreamSize FileSize(std::string_view filePath) = 0;
		virtual std::unique_ptr<IFileStream> OpenFile(std::string_view path) = 0;
		virtual std::vector<SDirectoryEntry> GetAllEntries() = 0;
		virtual std::vector<SDirectoryEntry> GetEntries(std::string_view dirPath) = 0;
	};
}
