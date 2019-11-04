#include "ContainerDevice.h"
#include "FileSystem.h"
#include <array>
#include <charconv>
#include <gsl/gsl>
#include <string_view>

namespace noire::fs
{
	// TODO: reverse name hashes

	CContainerDevice::CContainerDevice(IDevice& parentDevice, std::string_view containerFilePath)
		: mParent{ parentDevice },
		  mContainerFilePath{ (Expects(mParent.FileExists(containerFilePath)), containerFilePath) },
		  mContainerFileStream{ mParent.OpenFile(mContainerFilePath) },
		  mContainerFile{ *mContainerFileStream }
	{
	}

	bool CContainerDevice::PathExists(std::string_view path) const
	{
		std::uint32_t nameHash;
		const auto [_, ec] = std::from_chars(path.data(), path.data() + path.size(), nameHash, 16);
		Expects(ec == std::errc{});

		// check if any entry path is equal to the path or contains it
		return std::any_of(
			mContainerFile.Entries().begin(),
			mContainerFile.Entries().end(),
			[nameHash](const SContainerChunkEntry& e) { return e.NameHash == nameHash; });
	}

	bool CContainerDevice::FileExists(std::string_view filePath) const
	{
		return PathExists(filePath);
	}

	bool CContainerDevice::DirectoryExists(std::string_view dirPath) const
	{
		return dirPath.empty();
	}

	FileStreamSize CContainerDevice::FileSize(std::string_view filePath)
	{
		Expects(FileExists(filePath));

		std::uint32_t nameHash;
		const auto [_, ec] =
			std::from_chars(filePath.data(), filePath.data() + filePath.size(), nameHash, 16);
		Expects(ec == std::errc{});

		auto it = std::find_if(
			mContainerFile.Entries().begin(),
			mContainerFile.Entries().end(),
			[nameHash](const SContainerChunkEntry& e) { return e.NameHash == nameHash; });
		return it->Size();
	}

	std::unique_ptr<IFileStream> CContainerDevice::OpenFile(std::string_view path)
	{
		std::uint32_t nameHash;
		const auto [_, ec] = std::from_chars(path.data(), path.data() + path.size(), nameHash, 16);
		Expects(ec == std::errc{});

		auto it = std::find_if(
			mContainerFile.Entries().begin(),
			mContainerFile.Entries().end(),
			[nameHash](const SContainerChunkEntry& e) { return e.NameHash == nameHash; });

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
		entries.emplace_back(this, "", EDirectoryEntryType::Collection); // root
		for (auto& e : mContainerFile.Entries())
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

	std::vector<SDirectoryEntry> CContainerDevice::GetEntries(std::string_view dirPath)
	{
		Expects(DirectoryExists(dirPath));

		auto e = GetAllEntries();
		e.erase(e.begin()); // remove root entry
		return e;
	}
}