#pragma once
#include "Common.h"
#include "File.h"
#include "streams/Stream.h"
#include "trunk/Graphics.h"
#include "trunk/Section.h"
#include "trunk/UniqueTexture.h"
#include <optional>
#include <vector>

namespace noire
{
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

		const std::vector<trunk::Section>& Sections() const { return mSections; }
		u64 GetDataOffset(u32 offset) const;
		bool HasSection(u32 nameHash) const;
		std::optional<trunk::Section> GetSection(u32 nameHash) const;

		bool HasUniqueTexture() const;
		std::optional<trunk::UniqueTexture> GetUniqueTexture();

		bool HasGraphics() const;
		std::optional<trunk::Graphics> GetGraphics();

	private:
		bool mHasChanged;
		u64 mPrimaryDataPos;
		u64 mPrimaryDataSize;
		u64 mSecondaryDataPos;
		u64 mSecondaryDataSize;
		std::vector<trunk::Section> mSections;

	public:
		static constexpr size HeaderSize{ 20 };
		static constexpr u32 HeaderMagic{ 0x234D7274 }; // 'trM#'
		static const TypeDefinition Type;
	};
}
