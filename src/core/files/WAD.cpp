#include "WAD.h"
#include "streams/FileStream.h"
#include "streams/Stream.h"
#include <doctest/doctest.h>

namespace noire
{
	WAD::WAD(std::shared_ptr<Stream> input) : File(input) {}

	// Device implementation (TODO)
	bool WAD::Exists(PathView path) const
	{
		(void)path;
		return false;
	}
	std::shared_ptr<Stream> WAD::Open(PathView path)
	{
		(void)path;
		return nullptr;
	}
	std::shared_ptr<Stream> WAD::Create(PathView path)
	{
		(void)path;
		return nullptr;
	}
	bool WAD::Delete(PathView path)
	{
		(void)path;
		return false;
	}

	// File implementation
	void WAD::Load()
	{
		Stream& s = *Input();

		s.Seek(0, StreamSeekOrigin::Begin);

		const u32 magic = s.Read<u32>();
		Expects(magic == HeaderMagic);

		const u32 entryCount = s.Read<u32>();

		mEntries.reserve(entryCount);

		// read entries
		for (size i = 0; i < entryCount; ++i)
		{
			const u32 hash = s.Read<u32>();
			const u32 offset = s.Read<u32>();
			const u32 size = s.Read<u32>();

			mEntries.emplace_back("", hash, offset, size);
		}

		// read paths
		const WADEntry& lastEntry = mEntries.back();
		s.Seek(gsl::narrow<i64>(lastEntry.Offset) + lastEntry.Size, StreamSeekOrigin::Begin);
		for (size i = 0; i < entryCount; ++i)
		{
			const u16 strLength = s.Read<u16>();
			std::string& str = mEntries[i].Path;
			str.resize(strLength);

			s.Read(str.data(), strLength);
		}
	}

	void WAD::Save(Stream& s)
	{
		s.Write<u32>(HeaderMagic);

		const u32 entryCount = gsl::narrow<u32>(mEntries.size());
		s.Write(entryCount);

		// write entries
		for (size i = 0; i < entryCount; ++i)
		{
			const WADEntry& e = mEntries[i];
			s.Write<u32>(e.PathHash);
			s.Write<u32>(e.Offset);
			s.Write<u32>(e.Size);
		}

		// write data
		for (size i = 0; i < entryCount; ++i)
		{
			const WADEntry& e = mEntries[i];
			SubStream entryData{ Input(), e.Offset, e.Size };
			entryData.CopyTo(s);
		}

		// write path
		for (size i = 0; i < entryCount; ++i)
		{
			const WADEntry& e = mEntries[i];
			s.Write<u16>(gsl::narrow<u16>(e.Path.size()));
			s.Write(e.Path.data(), e.Path.size());
		}
	}
}

TEST_SUITE("WAD")
{
	using namespace noire;

	TEST_CASE("Load/Save" * doctest::skip(true))
	{
		std::shared_ptr<Stream> input = std::make_shared<FileStream>(
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out.wad.pc");

		WAD w{ input };
		w.Load();

		std::shared_ptr<Stream> output = std::make_shared<FileStream>(
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out_copy.wad.pc");

		w.Save(*output);
	}
}
