#include "ContainerFile.h"
#include <gsl/gsl>

namespace noire
{
	CContainerFile::CContainerFile(fs::IFileStream& stream) : mStream{ stream }, mEntries{}
	{
		LoadEntries();
	}

	void CContainerFile::LoadEntries()
	{
		mStream.Seek(0);

		const fs::FileStreamSize pos = mStream.Size() - 4;
		Expects(pos < mStream.Size());

		mStream.Seek(pos);
		const std::uint32_t entriesOffset = mStream.Read<std::uint32_t>();

		const fs::FileStreamSize entriesPos = mStream.Size() - entriesOffset;
		Expects(entriesPos < mStream.Size());

		mStream.Seek(entriesPos);
		mStream.Read<std::uint32_t>(); // magicValue

		const std::uint32_t entryCount = mStream.Read<std::uint32_t>();
		mEntries.reserve(entryCount);
		for (std::size_t i = 0; i < entryCount; i++)
		{
			const std::uint32_t f0 = mStream.Read<std::uint32_t>();
			const std::uint32_t f1 = mStream.Read<std::uint32_t>();
			const std::uint32_t f2 = mStream.Read<std::uint32_t>();
			const std::uint32_t f3 = mStream.Read<std::uint32_t>();
			const std::uint32_t f4 = mStream.Read<std::uint32_t>();

			mEntries.emplace_back(f0, f1, f2, f3, f4);
		}
	}

	bool CContainerFile::IsValid(fs::IFileStream& stream)
	{
		const auto resetStreamPos = gsl::finally([&stream]() { stream.Seek(0); });

		const fs::FileStreamSize size = stream.Size();
		if (size <= 12) // min required bytes
		{
			return false;
		}

		const fs::FileStreamSize pos = size - 4;
		if (pos >= size) // check incase of underflow
		{
			return false;
		}

		stream.Seek(pos);
		const std::uint32_t entriesOffset = stream.Read<std::uint32_t>();

		const fs::FileStreamSize entriesPos = size - entriesOffset;
		if (entriesPos + 4 >= size) // +4 as it should be able to read at least 4 bytes
		{
			return false;
		}

		stream.Seek(entriesPos);
		const std::uint32_t magicValue = stream.Read<std::uint32_t>();

		constexpr std::uint32_t ExpectedMagicValue{ 3 };
		return magicValue == ExpectedMagicValue;
	}
}