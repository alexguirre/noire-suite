#pragma once
#include "File.h"
#include "fs/FileStream.h"
#include <vector>

namespace noire
{
	// TODO: more research on the fields of SContainerChunkEntry
	struct SContainerChunkEntry
	{
		std::uint32_t NameHash;
		std::uint32_t Field1;
		std::uint32_t Field2;
		std::uint32_t Field3;
		std::uint32_t Field4;

		SContainerChunkEntry(std::uint32_t nameHash,
							 std::uint32_t field1,
							 std::uint32_t field2,
							 std::uint32_t field3,
							 std::uint32_t field4)
			: NameHash{ nameHash },
			  Field1{ field1 },
			  Field2{ field2 },
			  Field3{ field3 },
			  Field4{ field4 }
		{
		}

		fs::FileStreamSize Offset() const { return static_cast<fs::FileStreamSize>(Field1) << 4; }
		fs::FileStreamSize Size() const { return Size1() != 0 ? Size1() : Size2(); }

	private:
		// Used with 'sges' chunks, 'trM#' chunks have field4 set to 0.
		fs::FileStreamSize Size1() const { return Field4; }
		// Used with 'trM#' chunks.
		fs::FileStreamSize Size2() const
		{
			return static_cast<fs::FileStreamSize>(Field2 & 0x7FFFFFFF) + (Field3 & 0x7FFFFFFF);
		}
	};

	class CContainerFile
	{
	public:
		CContainerFile(fs::IFileStream& stream);

		const std::vector<SContainerChunkEntry>& Entries() const { return mEntries; }

	private:
		void LoadEntries();

		fs::IFileStream& mStream;
		std::vector<SContainerChunkEntry> mEntries;

	public:
		static bool IsValid(fs::IFileStream& stream);
	};

	template<>
	struct TFileTraits<CContainerFile>
	{
		static constexpr bool IsCollection{ true };
		static bool IsValid(fs::IFileStream& stream) { return CContainerFile::IsValid(stream); }
	};
}