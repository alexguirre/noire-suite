#include "MultiDevice.h"
#include <algorithm>
#include <string_view>

namespace noire
{
	bool MultiDevice::Exists(PathView path) const
	{
		PathView relPath;
		Device* d = GetDevice(path, &relPath);
		return d ? d->Exists(relPath) : false;
	}

	std::shared_ptr<File> MultiDevice::Open(PathView path)
	{
		PathView relPath;
		Device* d = GetDevice(path, &relPath);
		return d ? d->Open(relPath) : nullptr;
	}

	std::shared_ptr<File> MultiDevice::Create(PathView path, size fileTypeId)
	{
		PathView relPath;
		Device* d = GetDevice(path, &relPath);
		return d ? d->Create(relPath, fileTypeId) : nullptr;
	}

	bool MultiDevice::Delete(PathView path)
	{
		PathView relPath;
		Device* d = GetDevice(path, &relPath);
		return d ? d->Delete(relPath) : false;
	}

	void MultiDevice::Mount(PathView path, std::unique_ptr<Device> device)
	{
		Expects(std::find_if(mMounts.begin(), mMounts.end(), [path](const MountPoint& m) {
					return m.Path == path;
				}) == mMounts.end());

		mMounts.emplace_back(path, std::move(device));

		// sort the mounts, longest paths first
		std::sort(mMounts.begin(), mMounts.end(), [](auto& a, auto& b) {
			return a.Path.String().size() > b.Path.String().size();
		});
	}

	static bool StartsWith(std::string_view str, std::string_view prefix)
	{
		return str.compare(0, prefix.size(), prefix) == 0;
	}

	Device* MultiDevice::GetDevice(PathView path, PathView* outRelPath) const
	{
		for (const MountPoint& m : mMounts)
		{
			if (StartsWith(path.String(), m.Path.String()))
			{
				if (outRelPath)
				{
					*outRelPath = path.String().substr(m.Path.String().size() - 1);
				}
				return m.Device.get();
			}
		}

		if (outRelPath)
		{
			*outRelPath = PathView{};
		}
		return nullptr;
	}
}
