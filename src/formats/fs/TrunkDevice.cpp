#include "TrunkDevice.h"
#include "FileSystem.h"
#include <array>
#include <charconv>
#include <gsl/gsl>
#include <string_view>

namespace noire::fs
{
	CTrunkDevice::CTrunkDevice(IDevice& parentDevice, SPathView trunkFilePath)
		: mParent{ parentDevice },
		  mTrunkFilePath{ (Expects(mParent.FileExists(trunkFilePath)), trunkFilePath) },
		  mTrunkFileStream{ mParent.OpenFile(mTrunkFilePath) },
		  mTrunkFile{ *mTrunkFileStream }
	{
	}

	bool CTrunkDevice::PathExists(SPathView path) const
	{
		std::uint32_t nameHash;
		const auto [_, ec] = std::from_chars(path.String().data(),
											 path.String().data() + path.String().size(),
											 nameHash,
											 16);
		Expects(ec == std::errc{});

		// check if any entry path is equal to the path or contains it
		return std::any_of(mTrunkFile.Entries().begin(),
						   mTrunkFile.Entries().end(),
						   [nameHash](const STrunkEntry& e) { return e.NameHash == nameHash; });
	}

	bool CTrunkDevice::FileExists(SPathView filePath) const { return PathExists(filePath); }

	bool CTrunkDevice::DirectoryExists(SPathView dirPath) const { return dirPath.IsEmpty(); }

	FileStreamSize CTrunkDevice::FileSize(SPathView filePath)
	{
		Expects(FileExists(filePath));

		std::uint32_t nameHash;
		const auto [_, ec] = std::from_chars(filePath.String().data(),
											 filePath.String().data() + filePath.String().size(),
											 nameHash,
											 16);
		Expects(ec == std::errc{});

		auto it = std::find_if(mTrunkFile.Entries().begin(),
							   mTrunkFile.Entries().end(),
							   [nameHash](const STrunkEntry& e) { return e.NameHash == nameHash; });
		return it->Size;
	}

	std::unique_ptr<IFileStream> CTrunkDevice::OpenFile(SPathView path)
	{
		std::uint32_t nameHash;
		const auto [_, ec] = std::from_chars(path.String().data(),
											 path.String().data() + path.String().size(),
											 nameHash,
											 16);
		Expects(ec == std::errc{});

		auto it = std::find_if(mTrunkFile.Entries().begin(),
							   mTrunkFile.Entries().end(),
							   [nameHash](const STrunkEntry& e) { return e.NameHash == nameHash; });

		if (it != mTrunkFile.Entries().end())
		{
			const STrunkEntry& entry = *it;
			return std::make_unique<CSubFileStream>(mTrunkFileStream.get(),
													mTrunkFile.GetDataOffset(entry.Offset),
													entry.Size);
		}
		else
		{
			return nullptr;
		}
	}

	std::vector<SDirectoryEntry> CTrunkDevice::GetAllEntries()
	{
		std::vector<SDirectoryEntry> entries{};
		entries.reserve(mTrunkFile.Entries().size() + 1);
		entries.emplace_back(this, SPathView{}, EDirectoryEntryType::Collection); // root
		for (auto& e : mTrunkFile.Entries())
		{
			std::array<char, 16> buffer{};
			const auto [p, ec] =
				std::to_chars(buffer.data(), buffer.data() + buffer.size(), e.NameHash, 16);
			Expects(ec == std::errc{});

			const std::size_t strSize = p - buffer.data();
			entries.emplace_back(this,
								 std::string_view{ buffer.data(), strSize },
								 EDirectoryEntryType::File);
		}
		return entries;
	}

	std::vector<SDirectoryEntry> CTrunkDevice::GetEntries(SPathView dirPath)
	{
		Expects(DirectoryExists(dirPath));

		auto e = GetAllEntries();
		e.erase(e.begin()); // remove root entry
		return e;
	}
}