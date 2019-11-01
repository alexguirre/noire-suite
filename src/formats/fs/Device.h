#pragma once
#include <filesystem>
#include <string_view>
#include <vector>

namespace noire::fs
{
	class IDevice
	{
	public:
		virtual bool PathExists(std::string_view path) const = 0;
	};

	class CNativeDevice : public IDevice
	{
	public:
		CNativeDevice(const std::filesystem::path& rootDir);

		CNativeDevice(const CNativeDevice&) = delete;
		CNativeDevice& operator=(const CNativeDevice&) = delete;
		CNativeDevice(CNativeDevice&&) = default;
		CNativeDevice& operator=(CNativeDevice&&) = default;

		bool PathExists(std::string_view path) const override;

		const std::filesystem::path& RootDirectory() const { return mRootDir; }

	private:
		std::filesystem::path mRootDir;
	};
}
