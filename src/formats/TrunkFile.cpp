#include "TrunkFile.h"
#include <gsl/gsl>

namespace noire
{
	CTrunkFile::CTrunkFile(fs::IFileStream& stream) : mStream{ stream }, mEntries{}, mHeader{}
	{
		LoadEntries();
	}

	fs::FileStreamSize CTrunkFile::GetDataOffset(std::uint32_t offset)
	{
		return (offset & 1) ? (mHeader.SecondaryDataPos + (offset & 0xFFFFFFFE)) : offset;
	}

	void CTrunkFile::LoadEntries()
	{
		mStream.Seek(0);

		// read header (20 bytes)
		Expects(mStream.Read<std::uint32_t>() == HeaderMagic);
		mStream.Read<std::uint32_t>(); // meaning of these 4 bytes is unknown
		const std::uint32_t primaryDataPlusHeaderSize = mStream.Read<std::uint32_t>();
		const std::uint32_t secondaryDataSize = mStream.Read<std::uint32_t>();
		mStream.Read<std::uint32_t>(); // these 4 bytes can be ignored, should always be 0 and the
									   // game replaces them with a pointer to dynamically allocated
									   // memory where secondary data is copied to

		mHeader.PrimaryDataPos = mStream.Tell();
		mHeader.PrimaryDataSize = primaryDataPlusHeaderSize - HeaderSize;
		mHeader.SecondaryDataPos = mHeader.PrimaryDataPos + mHeader.PrimaryDataSize;
		mHeader.SecondaryDataSize = secondaryDataSize;

		const std::uint32_t entryCount = mStream.Read<std::uint32_t>();
		mEntries.reserve(entryCount);
		for (std::size_t i = 0; i < entryCount; i++)
		{
			const std::uint32_t nameHash = mStream.Read<std::uint32_t>();
			const std::uint32_t size = mStream.Read<std::uint32_t>();
			const std::uint32_t offset = mStream.Read<std::uint32_t>();

			mEntries.emplace_back(nameHash, size, offset);
		}
	}

	bool CTrunkFile::IsValid(fs::IFileStream& stream)
	{
		const auto resetStreamPos = gsl::finally([&stream]() { stream.Seek(0); });

		if (stream.Size() <= HeaderSize) // min required bytes
		{
			return false;
		}

		stream.Seek(0);
		const std::uint32_t magic = stream.Read<std::uint32_t>();
		return magic == HeaderMagic;
	}
}