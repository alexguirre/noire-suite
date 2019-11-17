#include "FileSystem.h"
#include <algorithm>
#include <gsl/gsl>

namespace noire::fs
{
	CFileSystem::CFileSystem() : mDeviceScanningEnabled{ false }, mDeviceTypes{}, mMounts{} {}

	void CFileSystem::Mount(SPathView path, std::unique_ptr<IDevice> device)
	{
		Expects(path.IsDirectory());
		Expects(device);

		bool exists = false;
		for (auto& m : mMounts)
		{
			if (m.Path == path)
			{
				m.Device = std::move(device);
				exists = true;
				break;
			}
		}

		if (!exists)
		{
			// no mount with same path already exists, add new one
			mMounts.emplace_back(path, std::move(device));

			// sort the mounts, longest paths first
			std::sort(mMounts.begin(), mMounts.end(), [](auto& a, auto& b) {
				return a.Path.String().size() > b.Path.String().size();
			});
		}

		if (mDeviceScanningEnabled)
		{
			ScanForDevices(path);
		}
	}

	void CFileSystem::Unmount(SPathView path)
	{
		// TODO: should CFileSystem::Unmount unmount any child mount points (mounts that have `path`
		// in their path)?
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

	void CFileSystem::RegisterDeviceType(SDeviceType::ValidatorFunction validator,
										 SDeviceType::CreatorFunction creator)
	{
		mDeviceTypes.emplace_back(std::move(validator), std::move(creator));
	}

	void CFileSystem::ScanForDevices(SPathView path)
	{
		// TODO: this scan is probably way too slow, we should scan lazily, for example only when
		// the users selects the folder
		Expects(path.IsDirectory() && DirectoryExists(path));

		IDevice* device = FindDevice(path);
		std::vector<SDirectoryEntry> entries = device->GetAllEntries();
		for (SDirectoryEntry& e : entries)
		{
			if (e.Type != noire::fs::EDirectoryEntryType::File)
			{
				continue;
			}

			auto f = device->OpenFile(e.Path);

			auto it = std::find_if(mDeviceTypes.begin(), mDeviceTypes.end(), [&f](SDeviceType& d) {
				return d.Validator(*f);
			});

			if (it != mDeviceTypes.end())
			{
				const noire::fs::SPath mountPath =
					(path / e.Path) + noire::fs::CFileSystem::DirectorySeparator;

				Mount(mountPath, it->Creator(*this, *device, e.Path));
			}
		}
	}
}