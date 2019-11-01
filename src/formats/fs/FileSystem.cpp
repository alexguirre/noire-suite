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

	std::unique_ptr<IFileStream> CFileSystem::OpenFile(std::string_view path)
	{
		std::string_view mountPath{};
		IDevice* dev = FindDevice(path, mountPath);
		// pass path relative to device to OpenFile
		return dev != nullptr ? dev->OpenFile(path.substr(mountPath.size())) : nullptr;
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