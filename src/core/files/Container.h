#pragma once
#include "Common.h"
#include "File.h"
#include "VFS.h"
#include "devices/Device.h"
#include <memory>
#include <utility>
#include <vector>

namespace noire
{
	struct ContainerEntry final
	{
		u32 NameHash;
		u32 Unk1; // Offset >> 4
		u32 Unk2;
		u32 Unk3;
		u32 Unk4; // 'sges' chunk size
		std::shared_ptr<File> File;

		inline ContainerEntry()
			: NameHash{ 0 }, Unk1{ 0 }, Unk2{ 0 }, Unk3{ 0 }, Unk4{ 0 }, File{ nullptr }
		{
		}

		inline ContainerEntry(u32 nameHash, u32 unk1, u32 unk2, u32 unk3, u32 unk4)
			: NameHash{ nameHash },
			  Unk1{ unk1 },
			  Unk2{ unk2 },
			  Unk3{ unk3 },
			  Unk4{ unk4 },
			  File{ nullptr }
		{
		}

		u64 Offset() const { return static_cast<u64>(Unk1) << 4; }
		u64 Size() const { return Size1() != 0 ? Size1() : Size2(); }

	private:
		// Used with 'sges' chunks (compressed?), 'trM#' chunks have Unk4 set to 0.
		u64 Size1() const { return Unk4; }
		// Used with 'trM#' chunks (uncompressed size?).
		u64 Size2() const { return static_cast<u64>(Unk2 & 0x7FFFFFFF) + (Unk3 & 0x7FFFFFFF); }
	};

	class Container final : public File, public Device
	{
	public:
		explicit Container();
		Container(std::shared_ptr<Stream> input);

		bool Exists(PathView path) const override;
		std::shared_ptr<File> Open(PathView path) override;
		std::shared_ptr<File> Create(PathView path, size fileTypeId) override;
		bool Delete(PathView path) override;

	protected:
		void LoadImpl() override;

	public:
		void Save(Stream& output) override;
		u64 Size() override;

		size GetEntryIndex(PathView path) const;
		size GetEntryIndex(size nameHash) const;
		const ContainerEntry& GetEntry(size nameHash) const;

	private:
		std::vector<ContainerEntry> mEntries;

	public:
		VirtualFileSystem mVFS; // VFS entry info refers to NameHash of the WADEntry

	public:
		static constexpr u32 EntriesHeaderMagic{ 3 }; // WAD\01
		static const Type Type;
	};
}
