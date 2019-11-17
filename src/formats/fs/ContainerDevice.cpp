#include "ContainerDevice.h"
#include "FileSystem.h"
#include "Hash.h"
#include <array>
#include <charconv>
#include <gsl/gsl>
#include <string_view>
#include <unordered_set>

namespace noire::fs
{
	CContainerDevice::CContainerDevice(IDevice& parentDevice, SPathView containerFilePath)
		: mParent{ parentDevice },
		  mContainerFilePath{ (Expects(mParent.FileExists(containerFilePath)), containerFilePath) },
		  mContainerFileStream{ mParent.OpenFile(mContainerFilePath) },
		  mContainerFile{ *mContainerFileStream },
		  mEntries{}
	{
		CreateEntries();
	}

	bool CContainerDevice::PathExists(SPathView path) const
	{
		if (path.IsFile())
		{
			std::uint32_t nameHash;
			const auto [_, ec] = std::from_chars(path.String().data(),
												 path.String().data() + path.String().size(),
												 nameHash,
												 16);
			Expects(ec == std::errc{} || ec == std::errc::invalid_argument);
			const std::uint32_t nameHash2 = crc32(path.String());

			// check if any entry path is equal to the path or contains it
			return std::any_of(mContainerFile.Entries().begin(),
							   mContainerFile.Entries().end(),
							   [nameHash, nameHash2](const SContainerChunkEntry& e) {
								   return e.NameHash == nameHash || e.NameHash == nameHash2;
							   });
		}
		else
		{
			for (auto& e : mEntries)
			{
				if (e.Type != EDirectoryEntryType::File && e.Path == path)
				{
					return true;
				}
			}

			return false;
		}
	}

	bool CContainerDevice::FileExists(SPathView filePath) const
	{
		Expects(filePath.IsFile());

		return PathExists(filePath);
	}

	bool CContainerDevice::DirectoryExists(SPathView dirPath) const
	{
		if (dirPath.IsEmpty()) // empty path represent root of this device
		{
			return true;
		}

		Expects(dirPath.IsDirectory());

		return PathExists(dirPath);
	}

	FileStreamSize CContainerDevice::FileSize(SPathView filePath)
	{
		Expects(FileExists(filePath));

		std::uint32_t nameHash;
		const auto [_, ec] = std::from_chars(filePath.String().data(),
											 filePath.String().data() + filePath.String().size(),
											 nameHash,
											 16);
		Expects(ec == std::errc{} || ec == std::errc::invalid_argument);
		const std::uint32_t nameHash2 = crc32(filePath.String());

		auto it = std::find_if(mContainerFile.Entries().begin(),
							   mContainerFile.Entries().end(),
							   [nameHash, nameHash2](const SContainerChunkEntry& e) {
								   return e.NameHash == nameHash || e.NameHash == nameHash2;
							   });
		return it->Size();
	}

	std::unique_ptr<IFileStream> CContainerDevice::OpenFile(SPathView path)
	{
		std::uint32_t nameHash;
		const auto [_, ec] = std::from_chars(path.String().data(),
											 path.String().data() + path.String().size(),
											 nameHash,
											 16);
		Expects(ec == std::errc{} || ec == std::errc::invalid_argument);
		const std::uint32_t nameHash2 = crc32(path.String());

		auto it = std::find_if(mContainerFile.Entries().begin(),
							   mContainerFile.Entries().end(),
							   [nameHash, nameHash2](const SContainerChunkEntry& e) {
								   return e.NameHash == nameHash || e.NameHash == nameHash2;
							   });

		if (it != mContainerFile.Entries().end())
		{
			const SContainerChunkEntry& entry = *it;
			return std::make_unique<CSubFileStream>(mContainerFileStream.get(),
													entry.Offset(),
													entry.Size());
		}
		else
		{
			return nullptr;
		}
	}

	std::vector<SDirectoryEntry> CContainerDevice::GetAllEntries() { return mEntries; }

	std::vector<SDirectoryEntry> CContainerDevice::GetEntries(SPathView dirPath)
	{
		Expects(DirectoryExists(dirPath));

		std::vector<SDirectoryEntry> entries{};
		for (auto& e : mEntries)
		{
			if (!e.Path.IsEmpty() && e.Path.Parent() == dirPath)
			{
				entries.emplace_back(this, e.Path, e.Type);
			}
		}

		return entries;
	}

	void CContainerDevice::CreateEntries()
	{
		mEntries.emplace_back(this, SPathView{}, EDirectoryEntryType::Collection); // root

		std::unordered_set<std::string_view> dirs{};
		const auto getDirectoriesFromPath = [](SPathView path,
											   std::unordered_set<std::string_view>& directories) {
			SPathView p = path.IsFile() ? path.Parent() : path;
			while (!p.IsEmpty() && !p.IsRoot())
			{
				directories.emplace(p.String());
				p = p.Parent();
			}
		};

		for (auto& e : mContainerFile.Entries())
		{
			std::string str = CHashDatabase::Instance().GetString(e.NameHash);
			mEntries.emplace_back(this, std::move(str), EDirectoryEntryType::File);
			getDirectoriesFromPath(mEntries.back().Path, dirs);
		}

		for (std::string_view d : dirs)
		{
			mEntries.emplace_back(this, d, EDirectoryEntryType::Directory);
		}
	}
}