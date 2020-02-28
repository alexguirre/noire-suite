#include "WAD.h"
#include "Hash.h"
#include "devices/LocalDevice.h"
#include "streams/FileStream.h"
#include "streams/Stream.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <string_view>

namespace noire
{
	WAD::WAD(Device& parent, PathView path, bool created)
		: File(parent, path, created), mHasChanged{ created }
	{
	}

	// Device implementation
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
		newEntry.FileType = fileTypeId;
		newEntry.File = mVFS.Create(*this, path, fileTypeId, [&newEntry](PathView path) {
			// remove first '/', paths in WAD file don't contain it
			newEntry.Path = path.String().substr(1);
			newEntry.PathHash = crc32(newEntry.Path);
			return newEntry.PathHash;
		});
		mHasChanged = true;

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
			mHasChanged = true;
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

	ReadOnlyStream WAD::OpenStream(PathView path)
	{
		const WADEntry& e = GetEntry(path);
		const bool emptyEntry = e.Size == 0;
		return emptyEntry ? ReadOnlyStream{ std::make_unique<EmptyStream>() } :
							ReadOnlyStream{ std::make_unique<SubStream>(Raw(), e.Offset, e.Size) };
	}

	// File implementation
	void WAD::LoadImpl()
	{
		Stream& s = Raw();
		if (s.Size() == 0)
		{
			return;
		}

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

				mEntries.emplace_back("", hash, offset, size);
			}

			Ensures(IsSorted());

			// read paths
			const WADEntry& lastEntry = mEntries.back();
			s.Seek(gsl::narrow<i64>(lastEntry.Offset) + lastEntry.Size, StreamSeekOrigin::Begin);
			for (size i = 0; i < entryCount; ++i)
			{
				const u64 t = s.Tell();
				const u16 strLength = s.Read<u16>();
				WADEntry& e = mEntries[i];
				std::string& str = e.Path;
				str.resize(strLength);

				s.Read(str.data(), strLength);

				const noire::Path filePath = Path::Root / str;
				mVFS.RegisterExistingFile(filePath, mEntries[i].PathHash);

				SubStream entryStream{ s, e.Offset, e.Size };
				e.FileType = File::FindTypeOfStream(entryStream);
				e.File = File::New(*this, filePath, false, e.FileType);
			}
		}
	}

	void WAD::Save()
	{
		if (!HasChanged())
		{
			return;
		}

		FixUpOffsets();

		Ensures(IsSorted());

		Stream& s = Raw();
		s.Seek(0, StreamSeekOrigin::Begin);
		s.Write<u32>(HeaderMagic);

		const u32 entryCount = gsl::narrow<u32>(mEntries.size());
		s.Write(entryCount);

		// write entries
		for (size i = 0; i < entryCount; ++i)
		{
			const WADEntry& e = mEntries[i];
			s.Write<u32>(e.PathHash);
			s.Write<u32>(e.NewOffset);
			s.Write<u32>(e.NewSize);
		}

		// write data
		std::ofstream fs{ "output.log" };
		for (size i = 0; i < entryCount; ++i)
		{
			WADEntry& e = mEntries[i];
			fs << e.Path << ':' << e.Offset << ", " << e.Size << ", " << e.NewOffset << ", "
			   << e.NewSize << ", " << s.Tell() << ", ";
			Ensures(e.NewOffset == s.Tell());
			{
				auto ef = e.File;
				if (ef->HasChanged())
				{
					ef->Save();
				}
				ef->Raw().CopyTo(s);
			}
			fs << (e.NewOffset + e.NewSize) << ", " << s.Tell() << '\n';
			if ((e.NewOffset + e.NewSize) != s.Tell())
			{
				Ensures((e.NewOffset + e.NewSize) == s.Tell());
			}
			e.Offset = e.NewOffset;
			e.Size = e.NewSize;
			e.NewOffset = 0;
			e.NewSize = 0;
			e.File = File::New(*this, Path::Root / e.Path, false, e.FileType);
		}

		// write path
		for (size i = 0; i < entryCount; ++i)
		{
			const WADEntry& e = mEntries[i];
			s.Write<u16>(gsl::narrow<u16>(e.Path.size()));
			s.Write(e.Path.data(), e.Path.size());
		}

		mHasChanged = false;
	}

	u64 WAD::Size()
	{
		// if (!HasChanged())
		//{
		//	return Raw()->Size();
		//}

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
			total += e.NewSize;                   // entry data
			total += sizeof(u16) + e.Path.size(); // entry path
		}
		return total;
	}

	bool WAD::HasChanged() const
	{
		return mHasChanged || std::any_of(mEntries.begin(), mEntries.end(), [](const WADEntry& e) {
				   return e.File->HasChanged();
			   });
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

	const WADEntry& WAD::GetEntry(PathView path) const
	{
		const size index = GetEntryIndex(path);
		Expects(index != static_cast<size>(-1));
		return mEntries[index];
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
			// NOTE: writing to NewOffset/NewSize because we still need the original Offset/Size
			// when calling Save() to open the streams at the appropriate locations
			e.NewOffset = currOffset;
			e.NewSize = gsl::narrow<u32>(e.File->Size());
			currOffset += e.NewSize;
		}

		Ensures(IsSorted());
	}

	static bool WADEntryComparer(const WADEntry& a, const WADEntry& b)
	{
		const bool useNewValuesA = a.NewOffset != 0;
		const bool useNewValuesB = b.NewOffset != 0;
		const u32 offsetA = useNewValuesA ? a.NewOffset : a.Offset;
		const u32 offsetB = useNewValuesB ? b.NewOffset : b.Offset;
		return offsetA < offsetB;
	}

	bool WAD::IsSorted() const
	{
		return std::is_sorted(mEntries.begin(), mEntries.end(), &WADEntryComparer);
	}

	static bool Validator(Stream& input)
	{
		if (input.Size() < 8) // min required bytes
		{
			return false;
		}

		const u32 magic = input.ReadAt<u32>(0);
		return magic == WAD::HeaderMagic;
	}

	static std::shared_ptr<File> Creator(Device& parent, PathView path, bool created)
	{
		return std::make_shared<WAD>(parent, path, created);
	}

	const File::TypeDefinition WAD::Type{ std::hash<std::string_view>{}("WAD"),
										  1,
										  &Validator,
										  &Creator };
}

// ifndef because line 'WAD& w = *wad;' gets compiler error 'illegal indirection' when
// compiling with tests disabled
#ifndef DOCTEST_CONFIG_DISABLE
TEST_SUITE("WAD")
{
	using namespace noire;

	TEST_CASE("Load/Delete/Create/Save" * doctest::skip(true))
	{
		if (std::filesystem::is_regular_file(
				"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out_modified.wad.pc"))
		{
			std::filesystem::remove(
				"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out_modified.wad.pc");
		}
		std::filesystem::copy_file(
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out.wad.pc",
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out_modified.wad.pc");

		LocalDevice d{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\" };
		std::shared_ptr wad = std::dynamic_pointer_cast<WAD>(d.Open("/out.wad.pc"));
		CHECK(wad != nullptr);
		WAD& w = *wad;

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
		auto r111 = std::static_pointer_cast<File>(c11->Create("/raw.bin", File::Type.Id));
		{
			auto& r111S = r111->Raw();
			for (u32 n = 0; n < 256; ++n)
			{
				r111S.Write(n);
			}
		}
		auto r1 = std::static_pointer_cast<File>(w.Create("/raw.bin", File::Type.Id));
		{
			auto& r1S = r1->Raw();
			for (u32 n = 0; n < 256; ++n)
			{
				r1S.Write(n);
			}
		}

		CHECK(w.Exists("/out/my_custom_file.wad.pc"));

		d.Commit();
	}

	TEST_CASE("Create new" * doctest::skip(true))
	{
		LocalDevice d{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\" };
		{
			d.Delete("/custom.wad.pc");

			std::shared_ptr wad =
				std::dynamic_pointer_cast<WAD>(d.Create("/custom.wad.pc", WAD::Type.Id));
			CHECK(wad != nullptr);
			WAD& w = *wad;

			w.Load();

			auto c1 =
				std::static_pointer_cast<WAD>(w.Create("/a/really/deep/file.wad.pc", WAD::Type.Id));
			c1->Load();
			auto c2 =
				std::static_pointer_cast<WAD>(w.Create("/wad_with_children.wad.pc", WAD::Type.Id));
			c2->Load();

			auto c21 = std::static_pointer_cast<WAD>(c2->Create("/inner1.wad.pc", WAD::Type.Id));
			c21->Load();
			auto c22 = std::static_pointer_cast<WAD>(c2->Create("/inner2.wad.pc", WAD::Type.Id));
			c22->Load();

			auto c211 =
				std::static_pointer_cast<WAD>(c21->Create("/inner_inner.wad.pc", WAD::Type.Id));
			c211->Load();
			auto c212 = std::static_pointer_cast<WAD>(
				c21->Create("/another/folder/inner_inner.wad.pc", WAD::Type.Id));
			c212->Load();

			auto r1 = std::static_pointer_cast<File>(c1->Create("/raw.bin", File::Type.Id));
			r1->Load();
			{
				auto& r1S = r1->Raw();
				for (u32 n = 0; n < 256; ++n)
				{
					r1S.Write(n);
				}
			}

			d.Commit();
		}

		{
			std::shared_ptr wad = std::dynamic_pointer_cast<WAD>(d.Open("/custom.wad.pc"));
			CHECK(wad != nullptr);
			WAD& w = *wad;

			const std::function<void(WAD&, PathView)> traverse = [&traverse](WAD& w,
																			 PathView parent) {
				if (!w.IsLoaded())
				{
					w.Load();
				}

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
		LocalDevice d{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\" };
		std::shared_ptr wad = std::dynamic_pointer_cast<WAD>(d.Open("/out.wad.pc"));
		CHECK(wad != nullptr);
		WAD& w = *wad;

		w.Load();

		CHECK_EQ(w.Size(), 151'574'057);
	}

	TEST_CASE("Modify 'atlas01.dds'" * doctest::skip(true))
	{
		if (std::filesystem::is_regular_file(
				"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out_atlas_modified.wad.pc"))
		{
			std::filesystem::remove(
				"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out_atlas_modified.wad.pc");
		}
		std::filesystem::copy_file(
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out.wad.pc",
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out_atlas_modified.wad.pc");

		LocalDevice d{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\" };
		std::shared_ptr wad = std::dynamic_pointer_cast<WAD>(d.Open("/out_atlas_modified.wad.pc"));
		CHECK(wad != nullptr);
		WAD& w = *wad;

		w.Load();

		Path ddsPath{ "/out/textures/dyndecals/atlas01.dds" };

		CHECK(w.Exists(ddsPath));

		auto rf = std::static_pointer_cast<File>(w.Open(ddsPath));

		CHECK(w.Exists(ddsPath));

		std::shared_ptr newDDS = std::make_shared<FileStream>("atlas01_custom.dds");
		auto& s = rf->Raw();
		newDDS->CopyTo(s);

		d.Commit();
	}

	TEST_CASE("Delete and create 'atlas01.dds'" * doctest::skip(true))
	{
		if (std::filesystem::is_regular_file("E:\\Rockstar Games\\L.A. Noire Complete "
											 "Edition\\test\\out_atlas_recreated.wad.pc"))
		{
			std::filesystem::remove("E:\\Rockstar Games\\L.A. Noire Complete "
									"Edition\\test\\out_atlas_recreated.wad.pc");
		}
		std::filesystem::copy_file(
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out.wad.pc",
			"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out_atlas_recreated.wad.pc");

		LocalDevice d{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\" };
		std::shared_ptr wad = std::dynamic_pointer_cast<WAD>(d.Open("/out_atlas_recreated.wad.pc"));
		CHECK(wad != nullptr);
		WAD& w = *wad;

		w.Load();

		Path ddsPath{ "/out/textures/dyndecals/atlas01.dds" };

		CHECK(w.Exists(ddsPath));

		w.Delete(ddsPath);

		CHECK(!w.Exists(ddsPath));

		auto rf = std::static_pointer_cast<File>(w.Create(ddsPath, File::Type.Id));

		CHECK(w.Exists(ddsPath));

		std::shared_ptr newDDS = std::make_shared<FileStream>("atlas01_custom.dds");
		auto& s = rf->Raw();
		newDDS->CopyTo(s);

		d.Commit();
	}
}
#endif
