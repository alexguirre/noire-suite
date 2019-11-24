#pragma once
#include "Device.h"
#include "Path.h"
#include <filesystem>
#include <fstream>
#include <memory>

namespace noire::fs
{
	class CNativeDevice : public IDevice
	{
	public:
		CNativeDevice(const std::filesystem::path& rootDir);

		CNativeDevice(const CNativeDevice&) = delete;
		CNativeDevice& operator=(const CNativeDevice&) = delete;
		CNativeDevice(CNativeDevice&&) = default;
		CNativeDevice& operator=(CNativeDevice&&) = default;

		bool PathExists(SPathView path) const override;
		bool FileExists(SPathView filePath) const override;
		bool DirectoryExists(SPathView dirPath) const override;
		FileStreamSize FileSize(SPathView filePath) override;
		std::unique_ptr<IFileStream> OpenFile(SPathView path) override;
		std::vector<SDirectoryEntry> GetAllEntries() override;
		std::vector<SDirectoryEntry> GetEntries(SPathView dirPath) override;

		const std::filesystem::path& RootDirectory() const { return mRootDir; }

	private:
		std::filesystem::path mRootDir;
	};

	class CNativeFileStream : public IFileStream
	{
	public:
		CNativeFileStream(const std::filesystem::path& path);
		~CNativeFileStream();

		void Read(void* destBuffer, FileStreamSize count) override;
		void Seek(FileStreamSize offset) override;
		FileStreamSize Tell() override;
		FileStreamSize Size() override;

	private:
		std::ifstream mStream;
	};
}