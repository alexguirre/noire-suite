#pragma once
#include "Device.h"
#include "FileStream.h"
#include "Path.h"
#include <functional>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace noire::fs
{
	class CFileSystem;

	struct SMountPoint
	{
		SPath Path;
		std::unique_ptr<IDevice> Device;

		SMountPoint(SPathView path, std::unique_ptr<IDevice> device)
			: Path{ path }, Device{ std::move(device) }
		{
		}
	};

	struct SDeviceType
	{
		using ValidatorFunction = std::function<bool(fs::IFileStream&)>;
		using CreatorFunction =
			std::function<std::unique_ptr<IDevice>(CFileSystem&,
												   IDevice& parentDevice,
												   SPathView pathRelativeToDevice)>;

		ValidatorFunction Validator;
		CreatorFunction Creator;

		SDeviceType(ValidatorFunction validator, CreatorFunction creator)
			: Validator{ std::move(validator) }, Creator{ std::move(creator) }
		{
		}
	};

	class CFileSystem
	{
	public:
		static constexpr char DirectorySeparator{ SPath::DirectorySeparator };

		CFileSystem();

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

		bool IsDeviceScanningEnabled() const { return mDeviceScanningEnabled; }
		void EnableDeviceScanning(bool enable) { mDeviceScanningEnabled = enable; }
		void RegisterDeviceType(SDeviceType::ValidatorFunction validator,
								SDeviceType::CreatorFunction creator);

	private:
		void ScanForDevices(SPathView path);

		bool mDeviceScanningEnabled;
		std::vector<SDeviceType> mDeviceTypes;
		std::vector<SMountPoint> mMounts;
	};
}