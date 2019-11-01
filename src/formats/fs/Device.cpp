#include "Device.h"
#include <gsl/gsl>

namespace noire::fs
{
	namespace stdfs = std::filesystem;

	CNativeDevice::CNativeDevice(const stdfs::path& rootDir) : mRootDir{ stdfs::absolute(rootDir) }
	{
		Expects(stdfs::exists(mRootDir) && stdfs::is_directory(mRootDir));
	}

	bool CNativeDevice::PathExists(std::string_view path) const
	{
		const stdfs::path fullPath = mRootDir / path;
		return stdfs::exists(fullPath);
	}
}