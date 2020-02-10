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

		inline WADEntry(std::string path, u32 pathHash, u32 offset, u32 size)
			: Path{ std::move(path) },
			  PathHash{ pathHash },
			  Offset{ offset },
			  Size{ size },
			  File{ nullptr }
		{
		}
	};

	class WAD final : public File, Device
	{
	public:
		WAD(std::shared_ptr<Stream> input);

		bool Exists(PathView path) const override;
		std::shared_ptr<Stream> Open(PathView path) override;
		std::shared_ptr<Stream> Create(PathView path) override;
		bool Delete(PathView path) override;

		void Load() override;
		void Save(Stream& output) override;
		u64 Size() override;

		size GetEntryIndex(PathView path) const;

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
