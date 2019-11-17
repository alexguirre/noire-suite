#pragma once
#include "ContainerFile.h"
#include "Device.h"
#include "FileStream.h"
#include <memory>
#include <vector>

namespace noire::fs
{
	class CContainerDevice : public IDevice
	{
	public:
		CContainerDevice(IDevice& parentDevice, SPathView containerFilePath);

		CContainerDevice(const CContainerDevice&) = delete;
		CContainerDevice& operator=(const CContainerDevice&) = delete;
		CContainerDevice(CContainerDevice&&) = delete;
		CContainerDevice& operator=(CContainerDevice&&) = delete;

		bool PathExists(SPathView path) const override;
		bool FileExists(SPathView filePath) const override;
		bool DirectoryExists(SPathView dirPath) const override;
		FileStreamSize FileSize(SPathView filePath) override;
		std::unique_ptr<IFileStream> OpenFile(SPathView path) override;
		std::vector<SDirectoryEntry> GetAllEntries() override;
		std::vector<SDirectoryEntry> GetEntries(SPathView dirPath) override;

	private:
		void CreateEntries();

		IDevice& mParent;
		SPath mContainerFilePath;
		std::unique_ptr<IFileStream> mContainerFileStream;
		CContainerFile mContainerFile;
		std::vector<SDirectoryEntry> mEntries;
	};
}
