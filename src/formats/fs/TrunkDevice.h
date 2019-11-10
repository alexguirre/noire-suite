#pragma once
#include "Device.h"
#include "FileStream.h"
#include "TrunkFile.h"
#include <memory>

namespace noire::fs
{
	class CTrunkDevice : public IDevice
	{
	public:
		CTrunkDevice(IDevice& parentDevice, SPathView trunkFilePath);

		CTrunkDevice(const CTrunkDevice&) = delete;
		CTrunkDevice& operator=(const CTrunkDevice&) = delete;
		CTrunkDevice(CTrunkDevice&&) = default;
		CTrunkDevice& operator=(CTrunkDevice&&) = default;

		bool PathExists(SPathView path) const override;
		bool FileExists(SPathView filePath) const override;
		bool DirectoryExists(SPathView dirPath) const override;
		FileStreamSize FileSize(SPathView filePath) override;
		std::unique_ptr<IFileStream> OpenFile(SPathView path) override;
		std::vector<SDirectoryEntry> GetAllEntries() override;
		std::vector<SDirectoryEntry> GetEntries(SPathView dirPath) override;

	private:
		IDevice& mParent;
		SPath mTrunkFilePath;
		std::unique_ptr<IFileStream> mTrunkFileStream;
		CTrunkFile mTrunkFile;
	};
}
