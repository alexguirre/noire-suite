#include "WAD.h"
#include "Hash.h"
#include "RawFile.h"
#include "streams/FileStream.h"
#include "streams/Stream.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <functional>
#include <iostream>
#include <string_view>

namespace noire
{
	WAD::WAD() : File(nullptr) {}

	WAD::WAD(std::shared_ptr<Stream> input) : File(input) {}

	// Device implementation (TODO)
	bool WAD::Exists(PathView path) const
	{
		Expects(path.IsAbsolute());

		return mVFS.Exists(path);
	}

	std::shared_ptr<File> WAD::Open(PathView path)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		return mVFS.Open(path, [this](PathView, size pathHash) { return GetEntry(pathHash).File; });
	}

	std::shared_ptr<File> WAD::Create(PathView path, size fileTypeId)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		WADEntry& newEntry = mEntries.emplace_back();
		newEntry.File = mVFS.Create(path, fileTypeId, [&newEntry](PathView path) {
			// remove first '/', paths in WAD file don't contain it
			newEntry.Path = path.String().substr(1);
			newEntry.PathHash = crc32(newEntry.Path);
			return newEntry.PathHash;
		});

		return newEntry.File;
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

	void WAD::Visit(DeviceVisitCallback visitDirectory,
					DeviceVisitCallback visitFile,
					PathView path,
					bool recursive)
	{
		mVFS.Visit(visitDirectory, visitFile, path, recursive);
	}

	// File implementation
	void WAD::LoadImpl()
	{
		if (!Input())
		{
			return;
		}

		Stream& s = *Input();

		s.Seek(0, StreamSeekOrigin::Begin);

		const u32 magic = s.Read<u32>();
		Expects(magic == HeaderMagic);

		const u32 entryCount = s.Read<u32>();

		mEntries.reserve(entryCount);

		if (entryCount)
		{
			// read entries
			for (size i = 0; i < entryCount; ++i)
			{
				const u32 hash = s.Read<u32>();
				const u32 offset = s.Read<u32>();
				const u32 size = s.Read<u32>();

				mEntries.emplace_back("", hash, offset, size).File =
					File::NewFromStream(std::make_shared<SubStream>(Input(), offset, size));
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
	}

	void WAD::Save(Stream& s)
	{
		FixUpOffsets();

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
		// TODO: hacky way to ensure child WADs also FixUpOffsets when the parent calls
		// FixUpOffsets, otherwise the total size may be wrong. Implement common way to do this in
		// Device or File
		FixUpOffsets();

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
		const size hash = mVFS.GetFileInfo(path);
		return GetEntryIndex(hash);
	}

	size WAD::GetEntryIndex(size pathHash) const
	{
		// TODO: don't use linear search here
		for (size i = 0; i < mEntries.size(); ++i)
		{
			if (mEntries[i].PathHash == pathHash)
			{
				return i;
			}
		}

		return static_cast<size>(-1);
	}

	const WADEntry& WAD::GetEntry(size pathHash) const
	{
		const size index = GetEntryIndex(pathHash);
		Expects(index != static_cast<size>(-1));
		return mEntries[index];
	}

	// TODO: FixUpOffsets is getting called more times than needed
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
		if (input->Size() < 8) // min required bytes
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

	static std::shared_ptr<File> CreatorEmpty() { return std::make_shared<WAD>(); }

	const File::Type WAD::Type{ std::hash<std::string_view>{}("WAD"),
								1,
								&Validator,
								&Creator,
								&CreatorEmpty };
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

	TEST_CASE("Load/Delete/Create/Save" * doctest::skip(true))
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

		CHECK_FALSE(w.Exists("/out/my_custom_file.wad.pc"));
		auto c1 =
			std::static_pointer_cast<WAD>(w.Create("/out/my_custom_file.wad.pc", WAD::Type.Id));
		auto c11 = std::static_pointer_cast<WAD>(c1->Create("/other.wad.pc", WAD::Type.Id));
		auto r111 = std::static_pointer_cast<RawFile>(c11->Create("/raw.bin", RawFile::Type.Id));
		for (u32 n = 0; n < 256; ++n)
		{
			r111->Stream().Write(n);
		}
		auto r1 = std::static_pointer_cast<RawFile>(w.Create("/raw.bin", RawFile::Type.Id));
		for (u32 n = 0; n < 256; ++n)
		{
			r1->Stream().Write(n);
		}

		CHECK(w.Exists("/out/my_custom_file.wad.pc"));

		std::shared_ptr<Stream> output = std::make_shared<FileStream>(
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out_modified.wad.pc");

		w.Save(*output);
		CHECK_EQ(w.Size(), output->Size());
	}

	TEST_CASE("Create new" * doctest::skip(true))
	{
		std::shared_ptr<Stream> output = std::make_shared<FileStream>(
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\custom.wad.pc");
		{
			WAD w{};

			w.Load();

			auto c1 =
				std::static_pointer_cast<WAD>(w.Create("/a/really/deep/file.wad.pc", WAD::Type.Id));
			auto c2 =
				std::static_pointer_cast<WAD>(w.Create("/wad_with_children.wad.pc", WAD::Type.Id));

			auto c21 = std::static_pointer_cast<WAD>(c2->Create("/inner1.wad.pc", WAD::Type.Id));
			auto c22 = std::static_pointer_cast<WAD>(c2->Create("/inner2.wad.pc", WAD::Type.Id));

			auto c211 =
				std::static_pointer_cast<WAD>(c21->Create("/inner_inner.wad.pc", WAD::Type.Id));
			auto c212 = std::static_pointer_cast<WAD>(
				c21->Create("/another/folder/inner_inner.wad.pc", WAD::Type.Id));

			auto r1 = std::static_pointer_cast<RawFile>(c1->Create("/raw.bin", RawFile::Type.Id));
			for (u32 n = 0; n < 256; ++n)
			{
				r1->Stream().Write(n);
			};

			w.Save(*output);
		}

		{
			WAD w{ output };
			const std::function<void(WAD&, PathView)> traverse = [&traverse](WAD& w,
																			 PathView parent) {
				w.Load();

				for (auto& e : w.GetEntries())
				{
					const Path fullPath = Path{ parent } / e.Path;
					std::cout << "'" << fullPath.String() << "'\n";

					if (std::shared_ptr<WAD> c = std::dynamic_pointer_cast<WAD>(e.File))
					{
						traverse(*c, fullPath);
					}
				}
			};
			traverse(w, PathView::Root);
		}
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
		CHECK(File::NewFromStream(input, false) != nullptr);
	}
}
