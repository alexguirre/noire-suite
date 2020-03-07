#pragma once
#include "Common.h"
#include "File.h"
#include <vector>

namespace noire
{
	struct TrunkSection
	{
		u32 NameHash;
		u32 Size;
		u32 Offset;
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

		u64 GetDataOffset(u32 offset) const;

	private:
		bool mHasChanged;
		u64 mPrimaryDataPos;
		u64 mPrimaryDataSize;
		u64 mSecondaryDataPos;
		u64 mSecondaryDataSize;
		std::vector<TrunkSection> mSections;

	public:
		static constexpr size HeaderSize{ 20 };
		static constexpr u32 HeaderMagic{ 0x234D7274 }; // 'trM#'
		static const TypeDefinition Type;
	};
}
