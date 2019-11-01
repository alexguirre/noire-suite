#include "WADDevice.h"
#include <gsl/gsl>

namespace noire::fs
{
	namespace stdfs = std::filesystem;

	CWADDevice::CWADDevice(IDevice& parentDevice, std::string_view wadFilePath)
		: mParent{ parentDevice },
		  mWADFilePath{ (Expects(mParent.PathExists(wadFilePath)), wadFilePath) },
		  mWADFileStream{ mParent.OpenFile(mWADFilePath) },
		  mWADFile{ *mWADFileStream }
	{
	}

	bool CWADDevice::PathExists(std::string_view path) const
	{
		return std::any_of(mWADFile.Entries().begin(),
						   mWADFile.Entries().end(),
						   [path](const WADRawFileEntry& e) {
							   return path.size() <= e.Path.size() &&
									  std::string_view{ e.Path.c_str(), path.size() } == path;
						   });
	}

	std::unique_ptr<IFileStream> CWADDevice::OpenFile(std::string_view path)
	{
		auto it = std::find_if(mWADFile.Entries().begin(),
							   mWADFile.Entries().end(),
							   [path](const WADRawFileEntry& e) { return e.Path == path; });

		if (it != mWADFile.Entries().end())
		{
			const WADRawFileEntry& entry = *it;
			return std::make_unique<CSubFileStream>(mWADFileStream.get(), entry.Offset, entry.Size);
		}
		else
		{
			return nullptr;
		}
	}
}