#include "ShaderProgramsFile.h"
#include <gsl/gsl>

namespace noire
{
	CShaderProgramsFile::CShaderProgramsFile(fs::IFileStream& stream)
		: mStream{ stream }, mEntries{}, mRawDataOffset{ 0 }, mRawDataSize{ 0 }
	{
		LoadEntries();
	}

	void CShaderProgramsFile::LoadEntries()
	{
		mStream.Seek(0);

		const std::uint32_t entryCount = mStream.Read<std::uint32_t>();
		mRawDataSize = mStream.Read<std::uint32_t>();

		mEntries.resize(entryCount);

		for (std::size_t i = 0; i < entryCount; i++)
		{
			mEntries[i].NameHash = mStream.Read<std::uint32_t>();
		}

		for (std::size_t i = 0; i < entryCount; i++)
		{
			auto& e = mEntries[i];
			e.VertexShaderOffset = mStream.Read<std::uint32_t>();
			mStream.Read<std::uint32_t>(); // always 0 in file, replaced by the game at runtime
			e.PixelShaderOffset = mStream.Read<std::uint32_t>();
			mStream.Read<std::uint32_t>(); // always 0 in file, replaced by the game at runtime
		}

		mRawDataOffset = mStream.Tell();

		for (std::size_t i = 0; i < entryCount; i++)
		{
			auto& e = mEntries[i];
			e.VertexShaderOffset += mRawDataOffset;
			e.PixelShaderOffset += mRawDataOffset;
			mStream.Seek(e.VertexShaderOffset);
			e.VertexShaderSize = mStream.Read<std::uint32_t>();
			mStream.Seek(e.PixelShaderOffset);
			e.PixelShaderSize = mStream.Read<std::uint32_t>();
		}
	}

	bool CShaderProgramsFile::IsValid(fs::IFileStream& stream)
	{
		const auto resetStreamPos = gsl::finally([&stream]() { stream.Seek(0); });

		const fs::FileStreamSize streamSize = stream.Size();
		if (streamSize <= 8) // min required bytes
		{
			return false;
		}

		// this file type doesn't have any kind of magic number and the game doesn't do any checks
		// to verify it is the correct format, so just check if the stream size is equal to the
		// expected size based on the file header
		const std::uint32_t entryCount = stream.Read<std::uint32_t>();
		const std::uint32_t rawDataSize = stream.Read<std::uint32_t>();

		// don't consider `programs.vfp.dx9` a CShaderProgramsFile for now, seems to have a
		// different layout than `programs.vfp.dx11`
		// TODO: research `programs.vfp.dx9`
		if (entryCount == 15582 && rawDataSize == 0xBB8780)
		{
			return false;
		}

		const fs::FileStreamSize rawDataOffset = // headerSize + entryHashesSize + entriesSize
			8 + (static_cast<fs::FileStreamSize>(entryCount) * 4) +
			(static_cast<fs::FileStreamSize>(entryCount) * 0x10);

		const fs::FileStreamSize totalSize = rawDataOffset + rawDataSize;
		return totalSize == streamSize;
	}
}