#include "Trunk.h"
#include "Hash.h"
#include "streams/Stream.h"
#include <doctest/doctest.h>

namespace noire
{
	Trunk::Trunk(Device& parent, PathView path, bool created)
		: File(parent, path, created),
		  mHasChanged{ created },
		  mPrimaryDataPos{ 0 },
		  mPrimaryDataSize{ 0 },
		  mSecondaryDataPos{ 0 },
		  mSecondaryDataSize{ 0 }
	{
	}

	// File implementation
	void Trunk::LoadImpl()
	{
		Stream& s = Raw();
		if (s.Size() == 0)
		{
			return;
		}

		s.Seek(0, StreamSeekOrigin::Begin);

		const u32 magic = s.Read<u32>();
		Expects(magic == HeaderMagic);

		s.Seek(4, StreamSeekOrigin::Current); // meaning of these 4 bytes is unknown

		const u32 primaryDataPlusHeaderSize = s.Read<u32>();
		const u32 secondaryDataSize = s.Read<u32>();
		s.Seek(4, StreamSeekOrigin::Current); // these 4 bytes can be ignored, should always be 0
											  // and the game replaces them with a pointer to
											  // dynamically allocated memory where secondary data
											  // is copied to

		mPrimaryDataPos = s.Tell();
		mPrimaryDataSize = primaryDataPlusHeaderSize - HeaderSize;
		mSecondaryDataPos = mPrimaryDataPos + mPrimaryDataSize;
		mSecondaryDataSize = secondaryDataSize;

		const u32 sectionCount = s.Read<u32>();
		mSections.reserve(sectionCount);
		for (size i = 0; i < sectionCount; i++)
		{
			trunk::Section& section = mSections.emplace_back();
			section.NameHash = s.Read<u32>();
			section.Size = s.Read<u32>();
			section.Offset = s.Read<u32>();
		}
	}

	// TODO: Trunk::Save
	void Trunk::Save()
	{
		if (!HasChanged())
		{
			return;
		}

		File::Save();

		mHasChanged = false;
	}

	// TODO: Trunk::Size
	u64 Trunk::Size() { return File::Size(); }

	bool Trunk::HasChanged() const { return mHasChanged; }

	u64 Trunk::GetDataOffset(u32 offset) const
	{
		return (offset & 1) ? (mSecondaryDataPos + (offset & 0xFFFFFFFE)) : offset;
	}

	bool Trunk::HasSection(u32 nameHash) const
	{
		return std::any_of(mSections.begin(), mSections.end(), [nameHash](auto& s) {
			return s.NameHash == nameHash;
		});
	}

	std::optional<trunk::Section> Trunk::GetSection(u32 nameHash) const
	{
		auto it = std::find_if(mSections.begin(), mSections.end(), [nameHash](auto& s) {
			return s.NameHash == nameHash;
		});

		return it != mSections.end() ? std::make_optional(*it) : std::nullopt;
	}

	constexpr u32 hash_uniquetexturemain{ crc32("uniquetexturemain") };
	constexpr u32 hash_uniquetexturevram{ crc32("uniquetexturevram") };

	bool Trunk::HasUniqueTexture() const
	{
		return HasSection(hash_uniquetexturemain) && HasSection(hash_uniquetexturevram);
	}

	std::optional<trunk::UniqueTexture> Trunk::GetUniqueTexture()
	{
		auto main = GetSection(hash_uniquetexturemain);
		if (!main)
		{
			return std::nullopt;
		}

		auto vram = GetSection(hash_uniquetexturevram);
		if (!vram)
		{
			return std::nullopt;
		}

		return trunk::UniqueTexture{ *this, *main, *vram };
	}

	constexpr u32 hash_graphicsmain{ crc32("graphicsmain") };
	constexpr u32 hash_graphicsvram{ crc32("graphicsvram") };

	bool Trunk::HasGraphics() const
	{
		return HasSection(hash_graphicsmain) && HasSection(hash_graphicsvram);
	}

	std::optional<trunk::Graphics> Trunk::GetGraphics()
	{
		auto main = GetSection(hash_graphicsmain);
		if (!main)
		{
			return std::nullopt;
		}

		auto vram = GetSection(hash_graphicsvram);
		if (!vram)
		{
			return std::nullopt;
		}

		return trunk::Graphics{ *this, *main, *vram };
	}

	static bool Validator(Stream& input)
	{
		if (input.Size() < Trunk::HeaderSize) // min required bytes (size of header)
		{
			return false;
		}

		const u32 magic = input.ReadAt<u32>(0);
		return magic == Trunk::HeaderMagic;
	}

	static std::shared_ptr<File> Creator(Device& parent, PathView path, bool created)
	{
		return std::make_shared<Trunk>(parent, path, created);
	}

	const File::TypeDefinition Trunk::Type{ std::hash<std::string_view>{}("Trunk"),
											1,
											&Validator,
											&Creator };
}

#ifndef DOCTEST_CONFIG_DISABLE
TEST_SUITE("Trunk")
{
	using namespace noire;

	TEST_CASE("" * doctest::skip(true)) {}
}
#endif
