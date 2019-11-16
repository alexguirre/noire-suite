#include "ContainerDevice.h"
#include "FileSystem.h"
#include "Hash.h"
#include <array>
#include <charconv>
#include <gsl/gsl>
#include <string_view>

namespace noire::fs
{
	// TODO: reverse name hashes

	CContainerDevice::CContainerDevice(IDevice& parentDevice, SPathView containerFilePath)
		: mParent{ parentDevice },
		  mContainerFilePath{ (Expects(mParent.FileExists(containerFilePath)), containerFilePath) },
		  mContainerFileStream{ mParent.OpenFile(mContainerFilePath) },
		  mContainerFile{ *mContainerFileStream }
	{
	}

	bool CContainerDevice::PathExists(SPathView path) const
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

	bool CContainerDevice::FileExists(SPathView filePath) const { return PathExists(filePath); }

	bool CContainerDevice::DirectoryExists(SPathView dirPath) const { return dirPath.IsEmpty(); }

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

	std::vector<SDirectoryEntry> CContainerDevice::GetAllEntries()
	{
		std::vector<SDirectoryEntry> entries{};
		entries.reserve(mContainerFile.Entries().size() + 1);
		entries.emplace_back(this, SPathView{}, EDirectoryEntryType::Collection); // root
		for (auto& e : mContainerFile.Entries())
		{
			std::string str = CHashDatabase::Instance().GetString(e.NameHash);
			entries.emplace_back(this, std::move(str), EDirectoryEntryType::File);
		}
		return entries;
	}

	std::vector<SDirectoryEntry> CContainerDevice::GetEntries(SPathView dirPath)
	{
		Expects(DirectoryExists(dirPath));

		auto e = GetAllEntries();
		e.erase(e.begin()); // remove root entry
		return e;
	}
}