#pragma once
#include "Device.h"
#include "Path.h"
#include <memory>
#include <vector>

namespace noire
{
	class MultiDevice : public Device
	{
	public:
		struct MountPoint
		{
			Path Path;
			std::unique_ptr<Device> Device;

			inline MountPoint(PathView path, std::unique_ptr<noire::Device> device)
				: Path{ path }, Device{ std::move(device) }
			{
			}
		};

		bool Exists(PathView path) const override;
		std::shared_ptr<File> Open(PathView path) override;
		std::shared_ptr<File> Create(PathView path, size fileTypeId) override;
		bool Delete(PathView path) override;
		void Visit(DeviceVisitCallback visitDirectory,
				   DeviceVisitCallback visitFile,
				   PathView path,
				   bool recursive) override;
		std::shared_ptr<ReadOnlyStream> OpenStream(PathView path) override;

	public:
		void Mount(PathView path, std::unique_ptr<Device> device);
		/// Gets the device that contains the specified path or null if none exists.
		/// 'outRelPath': returns the 'path' relative to the device mount path.
		Device* GetDevice(PathView path, PathView* outRelPath = nullptr) const;

	private:
		std::vector<MountPoint> mMounts;
	};
}