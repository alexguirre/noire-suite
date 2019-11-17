#pragma once
#include "FileStream.h"
#include "Path.h"
#include <memory>
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
		SPath Path;
		EDirectoryEntryType Type;

		SDirectoryEntry(IDevice* device, SPathView path, EDirectoryEntryType type) noexcept
			: Device{ device }, Path{ path }, Type{ type }
		{
		}
	};

	class IDevice
	{
	public:
		virtual ~IDevice() = default;

		virtual bool PathExists(SPathView path) const = 0;
		virtual bool FileExists(SPathView filePath) const = 0;
		virtual bool DirectoryExists(SPathView dirPath) const = 0;
		virtual FileStreamSize FileSize(SPathView filePath) = 0;
		virtual std::unique_ptr<IFileStream> OpenFile(SPathView path) = 0;
		virtual std::vector<SDirectoryEntry> GetAllEntries() = 0;
		virtual std::vector<SDirectoryEntry> GetEntries(SPathView dirPath) = 0;
	};
}
