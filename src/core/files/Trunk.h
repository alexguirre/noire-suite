#pragma once
#include "Common.h"
#include "File.h"
#include "streams/Stream.h"
#include <optional>
#include <vector>

namespace noire
{
	class Trunk;

	struct TrunkSectionHeader
	{
		u32 NameHash;
		u32 Size;
		u32 Offset;
	};

	struct TrunkUniqueTexture
	{
		struct Entry
		{
			u32 Offset;
			u32 NameHash;
		};

		Trunk& Owner;
		TrunkSectionHeader Main;
		TrunkSectionHeader VRAM;
		std::vector<Entry> Textures;

		TrunkUniqueTexture(Trunk& owner, TrunkSectionHeader main, TrunkSectionHeader vram);

		std::vector<byte> GetTextureData(size textureIndex);
	};

	class Trunk final : public File
	{
	public:
		Trunk(Device& parent, PathView path, bool created);

	protected:
		void LoadImpl() override;

	public:
		void Save() override;
		u64 Size() override;
		bool HasChanged() const override;

		const std::vector<TrunkSectionHeader>& Sections() const { return mSections; }
		u64 GetDataOffset(u32 offset) const;
		bool HasSection(u32 nameHash) const;
		std::optional<TrunkSectionHeader> GetSection(u32 nameHash) const;
		std::optional<SubStream> GetSectionDataStream(u32 nameHash);

		bool HasUniqueTexture() const;
		std::optional<TrunkUniqueTexture> GetUniqueTexture();

	private:
		bool mHasChanged;
		u64 mPrimaryDataPos;
		u64 mPrimaryDataSize;
		u64 mSecondaryDataPos;
		u64 mSecondaryDataSize;
		std::vector<TrunkSectionHeader> mSections;

	public:
		static constexpr size HeaderSize{ 20 };
		static constexpr u32 HeaderMagic{ 0x234D7274 }; // 'trM#'
		static const TypeDefinition Type;
	};
}
