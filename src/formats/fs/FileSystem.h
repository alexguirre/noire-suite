#pragma once
#include "Device.h"
#include "FileStream.h"
#include "Path.h"
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace noire::fs
{
	struct SMountPoint
	{
		SPath Path;
		std::unique_ptr<IDevice> Device;

		SMountPoint(SPathView path, std::unique_ptr<IDevice> device)
			: Path{ path }, Device{ std::move(device) }
		{
		}
	};

	class CFileSystem
	{
	public:
		static constexpr char DirectorySeparator{ SPath::DirectorySeparator };

		CFileSystem() {}

		void Mount(SPathView path, std::unique_ptr<IDevice> device);
		void Unmount(SPathView path);

		bool PathExists(SPathView path);
		bool FileExists(SPathView filePath);
		bool DirectoryExists(SPathView filePath);
		FileStreamSize FileSize(SPathView filePath);
		std::unique_ptr<IFileStream> OpenFile(SPathView path);
		std::vector<SDirectoryEntry> GetAllEntries();
		std::vector<SDirectoryEntry> GetEntries(SPathView dirPath);

		IDevice* FindDevice(SPathView path, SPathView& outMountPath);
		IDevice* FindDevice(SPathView path)
		{
			SPathView tmp{};
			return FindDevice(path, tmp);
		}
		SPathView GetDeviceMountPath(const IDevice* device) const;

	private:
		std::vector<SMountPoint> mMounts;
	};
}