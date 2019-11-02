#pragma once
#include "FileStream.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace noire::fs
{
	class IDevice;

	struct SDirectoryEntry
	{
		IDevice* Device;
		std::string Path;
		bool IsFile;

		SDirectoryEntry(IDevice* device, std::string_view path, bool isFile) noexcept
			: Device{ device }, Path{ path }, IsFile{ isFile }
		{
		}
	};

	class IDevice
	{
	public:
		virtual bool PathExists(std::string_view path) const = 0;
		virtual bool FileExists(std::string_view filePath) const = 0;
		virtual bool DirectoryExists(std::string_view dirPath) const = 0;
		virtual std::size_t FileSize(std::string_view filePath) = 0;
		virtual std::unique_ptr<IFileStream> OpenFile(std::string_view path) = 0;
		virtual std::vector<SDirectoryEntry> GetAllEntries() = 0;
		virtual std::vector<SDirectoryEntry> GetEntries(std::string_view dirPath) = 0;
	};
}
