#pragma once
#include "File.h"
#include "fs/FileStream.h"
#include <vector>

namespace noire
{
	struct STrunkHeader
	{
		fs::FileStreamSize PrimaryDataPos;
		fs::FileStreamSize PrimaryDataSize;
		fs::FileStreamSize SecondaryDataPos;
		fs::FileStreamSize SecondaryDataSize;

		STrunkHeader()
			: PrimaryDataPos{ 0 },
			  PrimaryDataSize{ 0 },
			  SecondaryDataPos{ 0 },
			  SecondaryDataSize{ 0 }
		{
		}
	};

	struct STrunkEntry
	{
		std::uint32_t NameHash;
		std::uint32_t Size;
		std::uint32_t Offset;

		STrunkEntry(std::uint32_t nameHash, std::uint32_t size, std::uint32_t offset)
			: NameHash{ nameHash }, Size{ size }, Offset{ offset }
		{
		}
	};

	class CTrunkFile
	{
	public:
		CTrunkFile(fs::IFileStream& stream);

		const STrunkHeader& Header() const { return mHeader; }
		const std::vector<STrunkEntry>& Entries() const { return mEntries; }
		fs::FileStreamSize GetDataOffset(std::uint32_t offset);

	private:
		void LoadEntries();

		fs::IFileStream& mStream;
		STrunkHeader mHeader;
		std::vector<STrunkEntry> mEntries;

	public:
		static bool IsValid(fs::IFileStream& stream);
		static constexpr fs::FileStreamSize HeaderSize{ 20 };
		static constexpr std::uint32_t HeaderMagic{ 0x234D7274 }; // "trM#"
	};

	template<>
	struct TFileTraits<CTrunkFile>
	{
		static constexpr bool IsCollection{ true };
		static bool IsValid(fs::IFileStream& stream) { return CTrunkFile::IsValid(stream); }
	};
}