#pragma once
#include "ContainerFile.h"
#include "Device.h"
#include "FileStream.h"
#include <memory>
#include <string>
#include <string_view>

namespace noire::fs
{
	class CContainerDevice : public IDevice
	{
	public:
		CContainerDevice(IDevice& parentDevice, std::string_view containerFilePath);

		CContainerDevice(const CContainerDevice&) = delete;
		CContainerDevice& operator=(const CContainerDevice&) = delete;
		CContainerDevice(CContainerDevice&&) = default;
		CContainerDevice& operator=(CContainerDevice&&) = default;

		bool PathExists(std::string_view path) const override;
		bool FileExists(std::string_view filePath) const override;
		bool DirectoryExists(std::string_view dirPath) const override;
		FileStreamSize FileSize(std::string_view filePath) override;
		std::unique_ptr<IFileStream> OpenFile(std::string_view path) override;
		std::vector<SDirectoryEntry> GetAllEntries() override;
		std::vector<SDirectoryEntry> GetEntries(std::string_view dirPath) override;

	private:
		IDevice& mParent;
		std::string mContainerFilePath;
		std::unique_ptr<IFileStream> mContainerFileStream;
		CContainerFile mContainerFile;
	};
}
