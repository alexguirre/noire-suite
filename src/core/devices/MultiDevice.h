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
			std::shared_ptr<Device> Device;

			inline MountPoint(PathView path, std::shared_ptr<noire::Device> device)
				: Path{ path }, Device{ device }
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
		ReadOnlyStream OpenStream(PathView path) override;
		void Commit() override;

		void Mount(PathView path, std::shared_ptr<Device> device);
		/// Gets the device that contains the specified path or null if none exists.
		/// 'outRelPath': returns the 'path' relative to the device mount path.
		Device* GetDevice(PathView path,
						  PathView* outRelPath = nullptr,
						  PathView* outDeviceMountPath = nullptr) const;

	private:
		std::vector<MountPoint> mMounts;
	};
}