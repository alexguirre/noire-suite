#pragma once
#include "File.h"
#include "fs/FileStream.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <gsl/span>
#include <string>
#include <vector>

namespace noire
{
	struct WADRawFileEntry
	{
		std::string Path;
		std::uint32_t PathHash;
		std::size_t Offset;
		std::size_t Size;

		WADRawFileEntry(std::string path,
						std::uint32_t pathHash,
						std::size_t offset,
						std::size_t size)
			: Path{ path }, PathHash{ pathHash }, Offset{ offset }, Size{ size }
		{
		}
	};

	class WADFile;

	class WADChildFile
	{
		friend class WADFile;

	public:
		const WADFile& Owner() const { return *mOwner; }
		size_t EntryIndex() const { return mEntryIndex; }
		std::string_view Name() const { return mName; }

		WADChildFile(const WADChildFile&) = delete;
		WADChildFile& operator=(const WADChildFile&) = delete;

		WADChildFile(WADChildFile&&) = default;
		WADChildFile& operator=(WADChildFile&&) = default;

	private:
		WADChildFile(const WADFile* owner, std::size_t entryIndex);

		const WADFile* mOwner;
		std::size_t mEntryIndex;
		std::string_view mName; // view of `WADRawFileEntry::Path`
	};

	class WADChildDirectory
	{
		friend class WADFile;

	public:
		const WADFile& Owner() const { return *mOwner; }
		const std::string& Name() const { return mName; }
		const std::vector<WADChildDirectory>& Directories() const { return mDirectories; }
		const std::vector<WADChildFile>& Files() const { return mFiles; }
		const WADChildDirectory* Parent() const;
		std::string Path() const;

		bool IsRoot() const { return mName == ""; }

		WADChildDirectory(const WADChildDirectory&) = delete;
		WADChildDirectory& operator=(const WADChildDirectory&) = delete;

		WADChildDirectory(WADChildDirectory&&) = default;
		WADChildDirectory& operator=(WADChildDirectory&&) = default;

	private:
		WADChildDirectory(const WADFile* owner);

		const WADFile* mOwner;
		std::string mName;
		std::vector<WADChildDirectory> mDirectories;
		std::vector<WADChildFile> mFiles;
	};

	class WADFile
	{
	public:
		WADFile(fs::IFileStream& stream);

		const std::vector<WADRawFileEntry>& Entries() const { return mEntries; }
		const WADChildDirectory& Root() const { return mRoot; }

		void Read(std::size_t offset, gsl::span<std::byte> dest) const;

	private:
		void LoadRawEntries();
		void InitRoot();
		WADChildDirectory& FindOrCreateDirectory(WADChildDirectory& root, std::string_view path);
		void SortDirectories(WADChildDirectory& root);

		fs::IFileStream& mStream;
		std::vector<WADRawFileEntry> mEntries;
		WADChildDirectory mRoot;

	public:
		static bool IsValid(fs::IFileStream& stream);
		static constexpr std::uint32_t HeaderMagic{ 0x01444157 }; // WAD\01
	};

	template<>
	struct TFileTraits<WADFile>
	{
		static constexpr bool IsCollection{ true };
		static bool IsValid(fs::IFileStream& stream) { return WADFile::IsValid(stream); }
	};
}
