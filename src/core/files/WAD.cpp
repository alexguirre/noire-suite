#include "WAD.h"
#include "streams/FileStream.h"
#include "streams/Stream.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <iostream>

namespace noire
{
	WAD::WAD(std::shared_ptr<Stream> input) : File(input) {}

	// Device implementation (TODO)
	bool WAD::Exists(PathView path) const
	{
		Expects(path.IsAbsolute());

		return mVFS.Exists(path);
	}

	std::shared_ptr<Stream> WAD::Open(PathView path)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		return nullptr;
	}

	std::shared_ptr<Stream> WAD::Create(PathView path)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		return nullptr;
	}

	bool WAD::Delete(PathView path)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		if (mVFS.Exists(path))
		{
			const size index = GetEntryIndex(path);
			mEntries.erase(mEntries.begin() + index);
			mVFS.Delete(path);
			FixUpOffsets();
			return true;
		}

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

			mEntries.emplace_back("", hash, offset, size).File =
				File::FromStream(std::make_shared<SubStream>(Input(), offset, size));
		}

		Expects(IsSorted());

		// read paths
		const WADEntry& lastEntry = mEntries.back();
		s.Seek(gsl::narrow<i64>(lastEntry.Offset) + lastEntry.Size, StreamSeekOrigin::Begin);
		for (size i = 0; i < entryCount; ++i)
		{
			const u64 t = s.Tell();
			const u16 strLength = s.Read<u16>();
			std::string& str = mEntries[i].Path;
			str.resize(strLength);

			s.Read(str.data(), strLength);

			mVFS.RegisterExistingFile(Path::Root / str, mEntries[i].PathHash);
		}
	}

	void WAD::Save(Stream& s)
	{
		Ensures(IsSorted());

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
			e.File->Save(s);
		}

		// write path
		for (size i = 0; i < entryCount; ++i)
		{
			const WADEntry& e = mEntries[i];
			s.Write<u16>(gsl::narrow<u16>(e.Path.size()));
			s.Write(e.Path.data(), e.Path.size());
		}
	}

	u64 WAD::Size()
	{
		// TODO: maybe size should be cached?
		size total = 0;
		total += sizeof(u32);                       // magic
		total += sizeof(u32);                       // entryCount
		total += sizeof(u32) * 3 * mEntries.size(); // entries
		for (const WADEntry& e : mEntries)
		{
			total += e.Size;                      // entry data
			total += sizeof(u16) + e.Path.size(); // entry path
		}
		return total;
	}

	size WAD::GetEntryIndex(PathView path) const
	{
		// TODO: don't use linear search here
		const size hash = mVFS.GetFileInfo(path);
		for (size i = 0; i < mEntries.size(); ++i)
		{
			if (mEntries[i].PathHash == hash)
			{
				return i;
			}
		}

		return static_cast<size>(-1);
	}

	void WAD::FixUpOffsets()
	{
		const size startingOffset = sizeof(u32) /*magic*/ + sizeof(u32) /*entryCount*/ +
									(sizeof(u32) * 3) * mEntries.size(); /*entries*/
		size currOffset = startingOffset;
		for (WADEntry& e : mEntries)
		{
			e.Offset = currOffset;
			e.Size = gsl::narrow<u32>(e.File->Size());
			currOffset += e.Size;
		}

		Sort();
	}

	static bool WADEntryComparer(const WADEntry& a, const WADEntry& b)
	{
		return a.Offset < b.Offset;
	}

	bool WAD::IsSorted() const
	{
		return std::is_sorted(mEntries.begin(), mEntries.end(), &WADEntryComparer);
	}

	void WAD::Sort() { std::sort(mEntries.begin(), mEntries.end(), &WADEntryComparer); }

	static bool Validator(std::shared_ptr<Stream> input)
	{
		if (input->Size() <= 8) // min required bytes
		{
			return false;
		}

		const u32 magic = input->ReadAt<u32>(0);
		return magic == WAD::HeaderMagic;
	}

	static std::shared_ptr<File> Creator(std::shared_ptr<Stream> input)
	{
		return std::make_shared<WAD>(input);
	}

	const File::Type WAD::Type{ 0, &Validator, &Creator };
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
		CHECK_EQ(w.Size(), output->Size());
	}

	TEST_CASE("Load/Delete/Save" * doctest::skip(false))
	{
		std::shared_ptr<Stream> input = std::make_shared<FileStream>(
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out.wad.pc");

		WAD w{ input };
		w.Load();

		CHECK(w.Exists("/out/graphicsdata/"));
		CHECK(w.Exists("/out/graphicsdata/programs.vfp.dx11"));
		CHECK(w.Exists("/out/trunk/models/la/environment/weather/"));
		CHECK(w.Exists("/out/trunk/models/la/environment/weather/sky.trunk.pc"));

		w.Delete("/out/graphicsdata/programs.vfp.dx11");

		CHECK_FALSE(w.Exists("/out/graphicsdata/programs.vfp.dx11"));
		CHECK(w.Exists("/out/trunk/models/la/environment/weather/sky.trunk.pc"));

		w.Delete("/out/trunk/models/la/environment/weather/sky.trunk.pc");

		CHECK_FALSE(w.Exists("/out/graphicsdata/programs.vfp.dx11"));
		CHECK_FALSE(w.Exists("/out/trunk/models/la/environment/weather/sky.trunk.pc"));

		std::shared_ptr<Stream> output = std::make_shared<FileStream>(
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out_deleted_entry.wad.pc");

		w.Save(*output);
		CHECK_EQ(w.Size(), output->Size());
	}

	TEST_CASE("Size" * doctest::skip(true))
	{
		std::shared_ptr<Stream> input = std::make_shared<FileStream>(
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out.wad.pc");

		WAD w{ input };
		w.Load();

		CHECK_EQ(w.Size(), input->Size());
	}

	TEST_CASE("Creator/Validator" * doctest::skip(true))
	{
		std::shared_ptr<Stream> input = std::make_shared<FileStream>(
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out.wad.pc");

		CHECK(Validator(input));
		CHECK(Creator(input) != nullptr);
		CHECK(File::FromStream(input, false) != nullptr);
	}
}
