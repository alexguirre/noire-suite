#include "WADFile.h"
#include <algorithm>
#include <fstream>
#include <gsl/gsl>
#include <string_view>

namespace fs = std::filesystem;

namespace noire
{
	static constexpr char PathSeparator{ '/' };

	WADChildFile::WADChildFile(const WADFile* owner, std::size_t entryIndex)
		: mOwner{ owner }, mEntryIndex{ entryIndex }
	{
		Expects(owner != nullptr);
		Expects(entryIndex < mOwner->Entries().size());

		mName = mOwner->Entries()[mEntryIndex].Path;
		if (std::size_t fileNameStart = mName.rfind(PathSeparator) + 1;
			fileNameStart != std::string_view::npos)
		{
			mName = mName.substr(fileNameStart);
		}
	}

	WADChildDirectory::WADChildDirectory(const WADFile* owner) : mOwner{ owner }
	{
		Expects(owner != nullptr);
	}

	static const WADChildDirectory* FindParent(const WADChildDirectory* current,
											   const WADChildDirectory* child)
	{
		// TODO: this will probably be too slow in some cases
		if (std::any_of(current->Directories().cbegin(),
						current->Directories().cend(),
						[child](auto& d) { return &d == child; }))
		{
			return current;
		}
		else
		{
			const WADChildDirectory* foundDir = nullptr;
			for (auto& d : current->Directories())
			{
				if (foundDir = FindParent(&d, child); foundDir)
				{
					break;
				}
			}
			return foundDir;
		}
	}

	const WADChildDirectory* WADChildDirectory::Parent() const
	{
		return FindParent(&mOwner->Root(), this);
	}

	std::string WADChildDirectory::Path() const
	{
		const WADChildDirectory* parent = Parent();
		return (parent ? parent->Path() : "") + mName + PathSeparator;
	}

	WADFile::WADFile(fs::IFileStream& stream) : mStream{ stream }, mEntries{}, mRoot{ this }
	{
		LoadRawEntries();
		InitRoot();
	}

	void WADFile::Read(std::size_t offset, gsl::span<std::byte> dest) const
	{
		mStream.Seek(offset);
		mStream.Read(dest.data(), dest.size_bytes());
	}

	void WADFile::LoadRawEntries()
	{
		mStream.Seek(0);

		std::uint32_t magic;
		mStream.Read(&magic, sizeof(magic));
		Expects(magic == TFileTraits<WADFile>::HeaderMagic);

		std::uint32_t entryCount;
		mStream.Read(&entryCount, sizeof(entryCount));

		mEntries.reserve(entryCount);

		// read entries
		for (std::size_t i = 0; i < entryCount; i++)
		{
			std::uint32_t hash, offset, size;
			mStream.Read(&hash, sizeof(hash));
			mStream.Read(&offset, sizeof(offset));
			mStream.Read(&size, sizeof(size));

			mEntries.emplace_back("", hash, offset, size);
		}

		// read path
		mStream.Seek(mEntries.back().Offset + mEntries.back().Size);
		for (std::size_t i = 0; i < entryCount; i++)
		{
			std::uint16_t strLength{ 0 };
			mStream.Read(&strLength, sizeof(strLength));

			std::string& str = mEntries[i].Path;
			str.resize(strLength);
			mStream.Read(str.data(), strLength);
		}
	}

	WADChildDirectory& WADFile::FindOrCreateDirectory(WADChildDirectory& root,
													  std::string_view path)
	{
		std::size_t separatorPos = path.find(PathSeparator);
		if (separatorPos == std::string_view::npos)
		{
			return root;
		}

		std::string_view directoryName = path.substr(0, separatorPos);
		std::string_view remainingPath = path.substr(separatorPos + 1);

		for (auto& d : root.mDirectories)
		{
			if (d.Name() == directoryName)
			{
				return FindOrCreateDirectory(d, remainingPath);
			}
		}

		WADChildDirectory newDir{ this };
		newDir.mName = directoryName;
		return FindOrCreateDirectory(root.mDirectories.emplace_back(std::move(newDir)),
									 remainingPath);
	}

	void WADFile::InitRoot()
	{
		mRoot.mName = "";

		for (std::size_t i = 0; i < mEntries.size(); i++)
		{
			const WADRawFileEntry& e = mEntries[i];
			WADChildDirectory& d = FindOrCreateDirectory(mRoot, e.Path);
			WADChildFile newFile{ this, i };
			d.mFiles.emplace_back(std::move(newFile));
		}

		SortDirectories(mRoot);
	}

	void WADFile::SortDirectories(WADChildDirectory& root)
	{
		std::sort(root.mDirectories.begin(),
				  root.mDirectories.end(),
				  [](const auto& a, const auto& b) { return a.Name() < b.Name(); });

		std::sort(root.mFiles.begin(), root.mFiles.end(), [](const auto& a, const auto& b) {
			return a.Name() < b.Name();
		});

		for (auto& d : root.mDirectories)
		{
			SortDirectories(d);
		}
	}
}
