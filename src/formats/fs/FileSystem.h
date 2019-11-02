#pragma once
#include "Device.h"
#include "FileStream.h"
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace noire::fs
{
	struct SMountPoint
	{
		std::string Path;
		std::unique_ptr<IDevice> Device;

		SMountPoint(std::string_view path, std::unique_ptr<IDevice> device)
			: Path{ path }, Device{ std::move(device) }
		{
		}
	};

	class CFileSystem
	{
	public:
		static constexpr char DirectorySeparator{ '/' };

		CFileSystem() {}

		void Mount(std::string_view path, std::unique_ptr<IDevice> device);
		void Unmount(std::string_view path);

		bool PathExists(std::string_view path);
		bool FileExists(std::string_view filePath);
		std::unique_ptr<IFileStream> OpenFile(std::string_view path);

		IDevice* FindDevice(std::string_view path, std::string_view& outMountPath);
		IDevice* FindDevice(std::string_view path)
		{
			std::string_view tmp{};
			return FindDevice(path, tmp);
		}

	private:
		std::vector<SMountPoint> mMounts;
	};
}