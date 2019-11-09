#pragma once
#include "Device.h"
#include "FileStream.h"
#include "Path.h"
#include "WADFile.h"
#include <memory>

namespace noire::fs
{
	class CWADDevice : public IDevice
	{
	public:
		CWADDevice(IDevice& parentDevice, SPathView wadFilePath);

		CWADDevice(const CWADDevice&) = delete;
		CWADDevice& operator=(const CWADDevice&) = delete;
		CWADDevice(CWADDevice&&) = default;
		CWADDevice& operator=(CWADDevice&&) = default;

		bool PathExists(SPathView path) const override;
		bool FileExists(SPathView filePath) const override;
		bool DirectoryExists(SPathView dirPath) const override;
		FileStreamSize FileSize(SPathView filePath) override;
		std::unique_ptr<IFileStream> OpenFile(SPathView path) override;
		std::vector<SDirectoryEntry> GetAllEntries() override;
		std::vector<SDirectoryEntry> GetEntries(SPathView dirPath) override;

	private:
		IDevice& mParent;
		SPath mWADFilePath;
		std::unique_ptr<IFileStream> mWADFileStream;
		WADFile mWADFile;
	};
}
