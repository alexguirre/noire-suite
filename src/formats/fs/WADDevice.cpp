#include "WADDevice.h"
#include "FileSystem.h"
#include <gsl/gsl>

namespace noire::fs
{
	namespace stdfs = std::filesystem;

	CWADDevice::CWADDevice(IDevice& parentDevice, std::string_view wadFilePath)
		: mParent{ parentDevice },
		  mWADFilePath{ (Expects(mParent.FileExists(wadFilePath)), wadFilePath) },
		  mWADFileStream{ mParent.OpenFile(mWADFilePath) },
		  mWADFile{ *mWADFileStream }
	{
	}

	bool CWADDevice::PathExists(std::string_view path) const
	{
		// check if any entry path is equal to the path or contains it
		return std::any_of(mWADFile.Entries().begin(),
						   mWADFile.Entries().end(),
						   [path](const WADRawFileEntry& e) {
							   return path.size() <= e.Path.size() &&
									  std::string_view{ e.Path.c_str(), path.size() } == path;
						   });
	}

	bool CWADDevice::FileExists(std::string_view filePath) const
	{
		// check if any entry path is equal to the filePath
		return std::any_of(mWADFile.Entries().begin(),
						   mWADFile.Entries().end(),
						   [filePath](const WADRawFileEntry& e) { return e.Path == filePath; });
	}

	bool CWADDevice::DirectoryExists(std::string_view dirPath) const
	{
		// check if any entry path contains the dirPath and is not equal (not a file)
		return std::any_of(mWADFile.Entries().begin(),
						   mWADFile.Entries().end(),
						   [dirPath](const WADRawFileEntry& e) {
							   return dirPath.size() < e.Path.size() &&
									  std::string_view{ e.Path.c_str(), dirPath.size() } == dirPath;
						   });
	}

	FileStreamSize CWADDevice::FileSize(std::string_view filePath)
	{
		Expects(FileExists(filePath));

		auto it = std::find_if(mWADFile.Entries().begin(),
							   mWADFile.Entries().end(),
							   [filePath](const WADRawFileEntry& e) { return e.Path == filePath; });
		return it->Size;
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

	static void InternalGetEntries(CWADDevice* device,
								   const WADChildDirectory& dir,
								   std::vector<SDirectoryEntry>& entries)
	{
		entries.emplace_back(device, dir.Path(), false);
		for (auto& f : dir.Files())
		{
			const std::string& path = f.Owner().Entries()[f.EntryIndex()].Path;
			entries.emplace_back(device, path, true);
		}

		for (auto& d : dir.Directories())
		{
			InternalGetEntries(device, d, entries);
		}
	}

	std::vector<SDirectoryEntry> CWADDevice::GetAllEntries()
	{
		std::vector<SDirectoryEntry> entries{};
		InternalGetEntries(this, mWADFile.Root(), entries);
		return entries;
	}

	static const WADChildDirectory* FindDirectory(const WADChildDirectory& root,
												  std::string_view path)
	{
		std::size_t separatorPos = path.find(fs::CFileSystem::DirectorySeparator);
		if (separatorPos == std::string_view::npos)
		{
			return &root;
		}

		std::string_view directoryName = path.substr(0, separatorPos);

		for (auto& d : root.Directories())
		{
			if (d.Name() == directoryName)
			{
				std::string_view remainingPath = path.substr(separatorPos + 1);
				return FindDirectory(d, remainingPath);
			}
		}

		return nullptr;
	}

	std::vector<SDirectoryEntry> CWADDevice::GetEntries(std::string_view dirPath)
	{
		Expects(DirectoryExists(dirPath));

		const WADChildDirectory* dir = FindDirectory(mWADFile.Root(), dirPath);
		Expects(dir != nullptr);

		std::vector<SDirectoryEntry> entries;
		for (auto& d : dir->Directories())
		{
			entries.emplace_back(this, d.Path(), false);
		}
		for (auto& f : dir->Files())
		{
			const std::string& path = f.Owner().Entries()[f.EntryIndex()].Path;
			entries.emplace_back(this, path, true);
		}

		return entries;
	}
}