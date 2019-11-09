#include "WADDevice.h"
#include "FileSystem.h"
#include <gsl/gsl>

namespace noire::fs
{
	CWADDevice::CWADDevice(IDevice& parentDevice, SPathView wadFilePath)
		: mParent{ parentDevice },
		  mWADFilePath{ (Expects(mParent.FileExists(wadFilePath)), wadFilePath) },
		  mWADFileStream{ mParent.OpenFile(mWADFilePath) },
		  mWADFile{ *mWADFileStream }
	{
	}

	bool CWADDevice::PathExists(SPathView path) const
	{
		// check if any entry path is equal to the path or contains it
		const std::string_view p = path.String();
		return std::any_of(mWADFile.Entries().begin(),
						   mWADFile.Entries().end(),
						   [p](const WADRawFileEntry& e) {
							   // TODO: this can probably be moved to SPathView
							   return p.size() <= e.Path.size() &&
									  std::string_view{ e.Path.c_str(), p.size() } == p;
						   });
	}

	bool CWADDevice::FileExists(SPathView filePath) const
	{
		// check if any entry path is equal to the filePath
		return std::any_of(mWADFile.Entries().begin(),
						   mWADFile.Entries().end(),
						   [filePath](const WADRawFileEntry& e) { return e.Path == filePath; });
	}

	bool CWADDevice::DirectoryExists(SPathView dirPath) const
	{
		// check if any entry path contains the dirPath and is not equal (not a file)
		const std::string_view p = dirPath.String();
		return std::any_of(mWADFile.Entries().begin(),
						   mWADFile.Entries().end(),
						   [p](const WADRawFileEntry& e) {
							   // TODO: this can probably be moved to SPathView
							   return p.size() < e.Path.size() &&
									  std::string_view{ e.Path.c_str(), p.size() } == p;
						   });
	}

	FileStreamSize CWADDevice::FileSize(SPathView filePath)
	{
		Expects(FileExists(filePath));

		auto it = std::find_if(mWADFile.Entries().begin(),
							   mWADFile.Entries().end(),
							   [filePath](const WADRawFileEntry& e) { return e.Path == filePath; });
		return it->Size;
	}

	std::unique_ptr<IFileStream> CWADDevice::OpenFile(SPathView path)
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
		entries.emplace_back(device,
							 dir.Path(),
							 dir.IsRoot() ? EDirectoryEntryType::Collection :
											EDirectoryEntryType::Directory);
		for (auto& f : dir.Files())
		{
			const std::string& path = f.Owner().Entries()[f.EntryIndex()].Path;
			entries.emplace_back(device, path, EDirectoryEntryType::File);
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

	static const WADChildDirectory* FindDirectory(const WADChildDirectory& root, SPathView path)
	{
		// TODO: these operations on the path string maybe should be moved to SPathView
		std::size_t separatorPos = path.String().find(fs::CFileSystem::DirectorySeparator);
		if (separatorPos == std::string_view::npos)
		{
			return &root;
		}

		std::string_view directoryName = path.String().substr(0, separatorPos);

		for (auto& d : root.Directories())
		{
			if (d.Name() == directoryName)
			{
				std::string_view remainingPath = path.String().substr(separatorPos + 1);
				return FindDirectory(d, remainingPath);
			}
		}

		return nullptr;
	}

	std::vector<SDirectoryEntry> CWADDevice::GetEntries(SPathView dirPath)
	{
		Expects(DirectoryExists(dirPath));

		const WADChildDirectory* dir = FindDirectory(mWADFile.Root(), dirPath);
		Expects(dir != nullptr);

		std::vector<SDirectoryEntry> entries;
		for (auto& d : dir->Directories())
		{
			entries.emplace_back(this, d.Path(), EDirectoryEntryType::Directory);
		}
		for (auto& f : dir->Files())
		{
			const std::string& path = f.Owner().Entries()[f.EntryIndex()].Path;
			entries.emplace_back(this, path, EDirectoryEntryType::File);
		}

		return entries;
	}
}