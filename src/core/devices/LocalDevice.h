#pragma once
#include "Device.h"
#include "Path.h"
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

namespace noire
{
	class LocalDevice : public Device
	{
	public:
		LocalDevice(const std::filesystem::path& rootPath);

		bool Exists(PathView path) const override;
		std::shared_ptr<File> Open(PathView path) override;
		std::shared_ptr<File> Create(PathView path, size fileTypeId) override;
		bool Delete(PathView path) override;
		void Visit(DeviceVisitCallback visitDirectory,
				   DeviceVisitCallback visitFile,
				   PathView path,
				   bool recursive) override;
		ReadOnlyStream OpenStream(PathView path) override;
		void Commit() override;

		inline const std::filesystem::path& RootPath() const { return mRootPath; }

	private:
		std::filesystem::path FullPath(PathView path) const;

		std::filesystem::path mRootPath;
		std::unordered_map<size, std::shared_ptr<File>> mCachedFiles;
	};
}