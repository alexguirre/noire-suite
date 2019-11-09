#include "FileSystem.h"
#include <algorithm>
#include <gsl/gsl>

namespace noire::fs
{
	void CFileSystem::Mount(SPathView path, std::unique_ptr<IDevice> device)
	{
		Expects(path.IsDirectory());
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
			return a.Path.String().size() > b.Path.String().size();
		});
	}

	void CFileSystem::Unmount(SPathView path)
	{
		mMounts.erase(std::remove_if(mMounts.begin(),
									 mMounts.end(),
									 [path](auto& m) { return m.Path == path; }),
					  mMounts.end());
	}

	bool CFileSystem::PathExists(SPathView path)
	{
		SPathView mountPath{};
		IDevice* dev = FindDevice(path, mountPath);
		return dev != nullptr && dev->PathExists(path.RelativeTo(mountPath));
	}

	bool CFileSystem::FileExists(SPathView filePath)
	{
		SPathView mountPath{};
		IDevice* dev = FindDevice(filePath, mountPath);
		return dev != nullptr && dev->FileExists(filePath.RelativeTo(mountPath));
	}

	bool CFileSystem::DirectoryExists(SPathView dirPath)
	{
		SPathView mountPath{};
		IDevice* dev = FindDevice(dirPath, mountPath);
		return dev != nullptr && dev->DirectoryExists(dirPath.RelativeTo(mountPath));
	}

	FileStreamSize CFileSystem::FileSize(SPathView filePath)
	{
		SPathView mountPath{};
		IDevice* dev = FindDevice(filePath, mountPath);
		Expects(dev != nullptr);
		return dev->FileSize(filePath.RelativeTo(mountPath));
	}

	std::unique_ptr<IFileStream> CFileSystem::OpenFile(SPathView path)
	{
		SPathView mountPath{};
		IDevice* dev = FindDevice(path, mountPath);
		return dev != nullptr ? dev->OpenFile(path.RelativeTo(mountPath)) : nullptr;
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
				dir.emplace_back(entry.Device, mount.Path / entry.Path, entry.Type);
			}
		}
		return dir;
	}

	std::vector<SDirectoryEntry> CFileSystem::GetEntries(SPathView dirPath)
	{
		SPathView mountPath{};
		IDevice* dev = FindDevice(dirPath, mountPath);
		Expects(dev != nullptr);

		auto deviceEntries = dev->GetEntries(dirPath.RelativeTo(mountPath));

		// prepend mountPath to entries
		std::vector<SDirectoryEntry> entries;
		entries.reserve(deviceEntries.size());
		for (auto& entry : deviceEntries)
		{
			SPath entryFullPath = mountPath / entry.Path;

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

	IDevice* CFileSystem::FindDevice(SPathView path, SPathView& outMountPath)
	{
		const std::string_view p = path.String();
		auto it = std::find_if(mMounts.begin(), mMounts.end(), [p](const SMountPoint& m) {
			// TODO: this check probably can be moved to SPathView
			return p.size() >= m.Path.String().size() &&
				   m.Path.String() == p.substr(0, m.Path.String().size());
		});

		return it != mMounts.end() ? (outMountPath = it->Path, it->Device.get()) :
									 (outMountPath = {}, nullptr);
	}

	SPathView CFileSystem::GetDeviceMountPath(const IDevice* device) const
	{
		Expects(device != nullptr);

		auto it = std::find_if(mMounts.begin(), mMounts.end(), [device](const SMountPoint& m) {
			return m.Device.get() == device;
		});

		Expects(it != mMounts.end());

		return it->Path;
	}
}