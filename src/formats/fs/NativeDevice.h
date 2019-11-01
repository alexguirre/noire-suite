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
		std::unique_ptr<IFileStream> OpenFile(std::string_view path) override;

		const std::filesystem::path& RootDirectory() const { return mRootDir; }

	private:
		std::filesystem::path mRootDir;
	};

	class CNativeFileStream : public IFileStream
	{
	public:
		CNativeFileStream(const std::filesystem::path& path);

		void Read(void* destBuffer, std::size_t count) override;
		void Seek(std::size_t offset) override;
		std::size_t Tell() override;
		std::size_t Size() override;

	private:
		std::ifstream mStream;
	};
}