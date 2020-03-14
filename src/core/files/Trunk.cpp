#include "Trunk.h"
#include "Hash.h"
#include "streams/Stream.h"
#include <doctest/doctest.h>

namespace noire
{
	TrunkUniqueTexture::TrunkUniqueTexture(Trunk& owner,
										   TrunkSectionHeader main,
										   TrunkSectionHeader vram)
		: Owner{ owner }, Main{ main }, VRAM{ vram }, Textures{}
	{
		std::optional<SubStream> mainStream = Owner.GetSectionDataStream(Main.NameHash);
		Stream& m = *mainStream;

		// these 4 bytes are used by the game at runtime indicate if it already loaded the texture,
		// the file should always have 0 here
		Expects(m.Read<u32>() == 0);

		const u32 textureCount = m.Read<u32>();
		Textures.reserve(textureCount);
		for (size i = 0; i < textureCount; i++)
		{
			Entry& e = Textures.emplace_back();
			e.Offset = m.Read<u32>();
			Expects(m.Read<u32>() == 0); // unknown, should be always zero
			e.NameHash = m.Read<u32>();
		}
	}

	std::vector<byte> TrunkUniqueTexture::GetTextureData(size textureIndex) const
	{
		Expects(textureIndex < Textures.size());

		std::optional<SubStream> vramStream = Owner.GetSectionDataStream(VRAM.NameHash);
		Stream& v = *vramStream;

		const Entry& e = Textures[textureIndex];
		// this expects the entries to be sorted by offset, not sure if that is always the case
		const size dataSize = ((textureIndex < (Textures.size() - 1)) ?
								   (Textures[textureIndex + 1].Offset - e.Offset) :
								   (gsl::narrow<size>(v.Size()) - e.Offset));

		std::vector<byte> buffer{};
		buffer.resize(dataSize);
		vramStream->Seek(e.Offset, StreamSeekOrigin::Begin);
		vramStream->Read(buffer.data(), dataSize);

		return buffer;
	}

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
			TrunkSectionHeader& section = mSections.emplace_back();
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

	std::optional<TrunkSectionHeader> Trunk::GetSection(u32 nameHash) const
	{
		auto it = std::find_if(mSections.begin(), mSections.end(), [nameHash](auto& s) {
			return s.NameHash == nameHash;
		});

		return it != mSections.end() ? std::make_optional(*it) : std::nullopt;
	}

	std::optional<SubStream> Trunk::GetSectionDataStream(u32 nameHash)
	{
		auto section = GetSection(nameHash);
		if (!section)
		{
			return std::nullopt;
		}

		return SubStream{ Raw(), GetDataOffset(section->Offset), section->Size };
	}

	constexpr u32 hash_uniquetexturemain{ crc32("uniquetexturemain") };
	constexpr u32 hash_uniquetexturevram{ crc32("uniquetexturevram") };

	bool Trunk::HasUniqueTexture() const
	{
		return HasSection(hash_uniquetexturemain) && HasSection(hash_uniquetexturevram);
	}

	std::optional<TrunkUniqueTexture> Trunk::GetUniqueTexture()
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

		return TrunkUniqueTexture{ *this, *main, *vram };
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
