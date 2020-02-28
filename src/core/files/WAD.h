#pragma once
#include "Common.h"
#include "File.h"
#include "VFS.h"
#include "devices/Device.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace noire
{
	struct WADEntry final
	{
		std::string Path;
		u32 PathHash;
		u32 Offset;
		u32 Size;
		std::shared_ptr<File> File;
		size FileType;
		u32 NewOffset;
		u32 NewSize;

		inline WADEntry()
			: Path{ "" },
			  PathHash{ 0 },
			  Offset{ 0 },
			  Size{ 0 },
			  File{ nullptr },
			  FileType{ File::InvalidTypeId },
			  NewOffset{ 0 },
			  NewSize{ 0 }
		{
		}

		inline WADEntry(std::string path, u32 pathHash, u32 offset, u32 size)
			: Path{ std::move(path) },
			  PathHash{ pathHash },
			  Offset{ offset },
			  Size{ size },
			  File{ nullptr },
			  FileType{ File::InvalidTypeId },
			  NewOffset{ 0 },
			  NewSize{ 0 }
		{
		}
	};

	class WAD final : public File, public Device
	{
	public:
		WAD(Device& parent, PathView path, bool created);

		bool Exists(PathView path) const override;
		std::shared_ptr<File> Open(PathView path) override;
		std::shared_ptr<File> Create(PathView path, size fileTypeId) override;
		bool Delete(PathView path) override;
		void Visit(DeviceVisitCallback visitDirectory,
				   DeviceVisitCallback visitFile,
				   PathView path,
				   bool recursive) override;
		ReadOnlyStream OpenStream(PathView path) override;

	protected:
		void LoadImpl() override;

	public:
		void Save() override;
		u64 Size() override;
		bool HasChanged() const override;

		size GetEntryIndex(PathView path) const;
		size GetEntryIndex(size pathHash) const;
		const WADEntry& GetEntry(PathView path) const;
		const WADEntry& GetEntry(size pathHash) const;
		const std::vector<WADEntry>& GetEntries() const { return mEntries; }

	private:
		void FixUpOffsets();
		bool IsSorted() const;

		std::vector<WADEntry> mEntries;
		VirtualFileSystem mVFS; // VFS entry info refers to PathHash of the WADEntry
		bool mHasChanged;

	public:
		static constexpr u32 HeaderMagic{ 0x01444157 }; // WAD\01
		static const TypeDefinition Type;
	};
}
