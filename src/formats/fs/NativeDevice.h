#pragma once
#include "Device.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>

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

		bool PathExists(std::string_view path) const override;
		bool FileExists(std::string_view filePath) const override;
		bool DirectoryExists(std::string_view dirPath) const override;
		FileStreamSize FileSize(std::string_view filePath) override;
		std::unique_ptr<IFileStream> OpenFile(std::string_view path) override;
		std::vector<SDirectoryEntry> GetAllEntries() override;
		std::vector<SDirectoryEntry> GetEntries(std::string_view dirPath) override;

		const std::filesystem::path& RootDirectory() const { return mRootDir; }

	private:
		std::filesystem::path mRootDir;
	};

	class CNativeFileStream : public IFileStream
	{
	public:
		CNativeFileStream(const std::filesystem::path& path);

		void Read(void* destBuffer, FileStreamSize count) override;
		void Seek(FileStreamSize offset) override;
		FileStreamSize Tell() override;
		FileStreamSize Size() override;

	private:
		std::ifstream mStream;
	};
}