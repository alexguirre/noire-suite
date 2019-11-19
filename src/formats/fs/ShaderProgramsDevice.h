#pragma once
#include "ShaderProgramsFile.h"
#include "Device.h"
#include "FileStream.h"
#include <memory>
#include <vector>

namespace noire::fs
{
	class CShaderProgramsDevice : public IDevice
	{
	public:
		CShaderProgramsDevice(IDevice& parentDevice, SPathView programsFilePath);

		CShaderProgramsDevice(const CShaderProgramsDevice&) = delete;
		CShaderProgramsDevice& operator=(const CShaderProgramsDevice&) = delete;
		CShaderProgramsDevice(CShaderProgramsDevice&&) = delete;
		CShaderProgramsDevice& operator=(CShaderProgramsDevice&&) = delete;

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
		SPath mProgramsFilePath;
		std::unique_ptr<IFileStream> mProgramsFileStream;
		CShaderProgramsFile mProgramsFile;
		std::vector<SDirectoryEntry> mEntries;
	};
}
