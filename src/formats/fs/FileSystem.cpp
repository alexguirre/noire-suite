#include "FileSystem.h"
#include <algorithm>
#include <gsl/gsl>

namespace noire::fs
{
	void CFileSystem::Mount(std::string_view path, std::unique_ptr<IDevice> device)
	{
		Expects(path.size() > 0 && path.back() == DirectorySeparator);
		Expects(device);

		for (auto& m : mMounts)
		{
			if (m.Path == path)
			{
				m.Device = std::move(device);
				return;
			}
		}

		// no mount with same path already exists, add new one
		mMounts.emplace_back(path, std::move(device));

		// sort the mounts, longest paths first
		std::sort(mMounts.begin(), mMounts.end(), [](auto& a, auto& b) {
			return a.Path.size() > b.Path.size();
		});
	}

	void CFileSystem::Unmount(std::string_view path)
	{
		mMounts.erase(std::remove_if(mMounts.begin(),
									 mMounts.end(),
									 [path](auto& m) { return m.Path == path; }),
					  mMounts.end());
	}

	bool CFileSystem::PathExists(std::string_view path)
	{
		std::string_view mountPath{};
		IDevice* dev = FindDevice(path, mountPath);
		// pass path relative to device to PathExists
		return dev != nullptr && dev->PathExists(path.substr(mountPath.size()));
	}

	bool CFileSystem::FileExists(std::string_view filePath)
	{
		std::string_view mountPath{};
		IDevice* dev = FindDevice(filePath, mountPath);
		// pass path relative to device to FileExists
		return dev != nullptr && dev->FileExists(filePath.substr(mountPath.size()));
	}

	bool CFileSystem::DirectoryExists(std::string_view dirPath)
	{
		std::string_view mountPath{};
		IDevice* dev = FindDevice(dirPath, mountPath);
		// pass path relative to device to FileExists
		return dev != nullptr && dev->DirectoryExists(dirPath.substr(mountPath.size()));
	}

	FileStreamSize CFileSystem::FileSize(std::string_view filePath)
	{
		std::string_view mountPath{};
		IDevice* dev = FindDevice(filePath, mountPath);
		Expects(dev != nullptr);
		// pass path relative to device to FileSize
		return dev->FileSize(filePath.substr(mountPath.size()));
	}

	std::unique_ptr<IFileStream> CFileSystem::OpenFile(std::string_view path)
	{
		std::string_view mountPath{};
		IDevice* dev = FindDevice(path, mountPath);
		// pass path relative to device to OpenFile
		return dev != nullptr ? dev->OpenFile(path.substr(mountPath.size())) : nullptr;
	}

	std::vector<SDirectoryEntry> CFileSystem::GetAllEntries()
	{
		// TODO: this is probably really memory inefficient
		std::vector<SDirectoryEntry> dir{};
		for (auto& mount : mMounts)
		{
			auto deviceEntries = mount.Device->GetAllEntries();
			dir.reserve(dir.size() + deviceEntries.size());
			for (auto& entry : deviceEntries)
			{
				dir.emplace_back(entry.Device, mount.Path + entry.Path, entry.Type);
			}
		}
		return dir;
	}

	std::vector<SDirectoryEntry> CFileSystem::GetEntries(std::string_view dirPath)
	{
		std::string_view mountPath{};
		IDevice* dev = FindDevice(dirPath, mountPath);
		Expects(dev != nullptr);

		// pass path relative to device to GetEntries
		auto deviceEntries = dev->GetEntries(dirPath.substr(mountPath.size()));

		// prepend mountPath to entries
		std::vector<SDirectoryEntry> entries;
		entries.reserve(deviceEntries.size());
		for (auto& entry : deviceEntries)
		{
			std::string entryFullPath = std::string{ mountPath } + entry.Path;

			if (entry.Type == EDirectoryEntryType::File)
			{
				if (auto it = std::find_if(mMounts.begin(),
										   mMounts.end(),
										   [&entryFullPath](auto& mount) {
											   return mount.Path ==
													  (entryFullPath + DirectorySeparator);
										   });
					it != mMounts.end())
				{
					// found a mount with the same path as a file entry, convert it to a collection
					// entry
					entryFullPath += DirectorySeparator;
					entry.Type = EDirectoryEntryType::Collection;
				}
			}

			entries.emplace_back(entry.Device, entryFullPath, entry.Type);
		}

		return entries;
	}

	IDevice* CFileSystem::FindDevice(std::string_view path, std::string_view& outMountPath)
	{
		auto it = std::find_if(mMounts.begin(), mMounts.end(), [path](auto& m) {
			return path.size() >= m.Path.size() && m.Path == path.substr(0, m.Path.size());
		});

		return it != mMounts.end() ? (outMountPath = it->Path, it->Device.get()) :
									 (outMountPath = {}, nullptr);
	}
}