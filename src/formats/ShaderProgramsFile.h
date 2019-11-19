#pragma once
#include "File.h"
#include "fs/FileStream.h"
#include <vector>

namespace noire
{
	struct SShaderProgramEntry
	{
		std::uint32_t NameHash;
		fs::FileStreamSize VertexShaderOffset;
		fs::FileStreamSize PixelShaderOffset;
		fs::FileStreamSize VertexShaderSize;
		fs::FileStreamSize PixelShaderSize;

		SShaderProgramEntry() : SShaderProgramEntry(0, 0, 0, 0, 0) {}
		SShaderProgramEntry(std::uint32_t nameHash,
							fs::FileStreamSize vertexShaderOffset,
							fs::FileStreamSize pixelShaderOffset,
							fs::FileStreamSize vertexShaderSize,
							fs::FileStreamSize pixelShaderSize)
			: NameHash{ nameHash },
			  VertexShaderOffset{ vertexShaderOffset },
			  PixelShaderOffset{ pixelShaderOffset },
			  VertexShaderSize{ vertexShaderSize },
			  PixelShaderSize{ pixelShaderSize }
		{
		}
	};

	class CShaderProgramsFile
	{
	public:
		CShaderProgramsFile(fs::IFileStream& stream);

		const std::vector<SShaderProgramEntry>& Entries() const { return mEntries; }

	private:
		void LoadEntries();

		fs::IFileStream& mStream;
		std::vector<SShaderProgramEntry> mEntries;
		fs::FileStreamSize mRawDataOffset;
		fs::FileStreamSize mRawDataSize;

	public:
		static bool IsValid(fs::IFileStream& stream);
	};

	template<>
	struct TFileTraits<CShaderProgramsFile>
	{
		static constexpr bool IsCollection{ true };
		static bool IsValid(fs::IFileStream& stream)
		{
			return CShaderProgramsFile::IsValid(stream);
		}
	};
}