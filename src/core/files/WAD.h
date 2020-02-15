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

		inline WADEntry() : Path{ "" }, PathHash{ 0 }, Offset{ 0 }, Size{ 0 }, File{ nullptr } {}

		inline WADEntry(std::string path, u32 pathHash, u32 offset, u32 size)
			: Path{ std::move(path) },
			  PathHash{ pathHash },
			  Offset{ offset },
			  Size{ size },
			  File{ nullptr }
		{
		}
	};

	class WAD final : public File, public Device
	{
	public:
		explicit WAD();
		WAD(std::shared_ptr<Stream> input);

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
		size GetEntryIndex(size pathHash) const;
		const WADEntry& GetEntry(size pathHash) const;
		const std::vector<WADEntry>& GetEntries() const { return mEntries; }

	private:
		void FixUpOffsets();
		bool IsSorted() const;
		void Sort();

		std::vector<WADEntry> mEntries;
		VirtualFileSystem<size> mVFS; // VFS entry info refers to PathHash of the WADEntry

	public:
		static constexpr u32 HeaderMagic{ 0x01444157 }; // WAD\01
		static const Type Type;
	};
}
