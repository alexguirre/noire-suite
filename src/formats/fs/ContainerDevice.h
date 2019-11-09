#pragma once
#include "ContainerFile.h"
#include "Device.h"
#include "FileStream.h"
#include <memory>

namespace noire::fs
{
	class CContainerDevice : public IDevice
	{
	public:
		CContainerDevice(IDevice& parentDevice, SPathView containerFilePath);

		CContainerDevice(const CContainerDevice&) = delete;
		CContainerDevice& operator=(const CContainerDevice&) = delete;
		CContainerDevice(CContainerDevice&&) = default;
		CContainerDevice& operator=(CContainerDevice&&) = default;

		bool PathExists(SPathView path) const override;
		bool FileExists(SPathView filePath) const override;
		bool DirectoryExists(SPathView dirPath) const override;
		FileStreamSize FileSize(SPathView filePath) override;
		std::unique_ptr<IFileStream> OpenFile(SPathView path) override;
		std::vector<SDirectoryEntry> GetAllEntries() override;
		std::vector<SDirectoryEntry> GetEntries(SPathView dirPath) override;

	private:
		IDevice& mParent;
		SPath mContainerFilePath;
		std::unique_ptr<IFileStream> mContainerFileStream;
		CContainerFile mContainerFile;
	};
}
