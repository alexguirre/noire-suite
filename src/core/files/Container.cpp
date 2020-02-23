#include "Container.h"
#include "Hash.h"
#include "devices/LocalDevice.h"
#include "streams/FileStream.h"
#include "streams/Stream.h"
#include <doctest/doctest.h>
#include <iostream>
#include <string_view>

namespace noire
{
	Container::Container(Device& parent, PathView path) : File(parent, path) {}

	// Device implementation (TODO)
	bool Container::Exists(PathView path) const
	{
		Expects(path.IsAbsolute());

		return mVFS.Exists(path);
	}

	std::shared_ptr<File> Container::Open(PathView path)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		return mVFS.Open(path, [this](PathView, size pathHash) { return GetEntry(pathHash).File; });
	}

	std::shared_ptr<File> Container::Create(PathView path, size fileTypeId)
	{
		// TODO: Container::Create
		Expects(path.IsFile() && path.IsAbsolute());
		Expects(false && "Not implemented");
		(void)fileTypeId;
		return nullptr;
	}

	bool Container::Delete(PathView path)
	{
		// TODO: Container::Delete
		Expects(path.IsFile() && path.IsAbsolute());
		Expects(false && "Not implemented");
		return false;
	}

	void Container::Visit(DeviceVisitCallback visitDirectory,
						  DeviceVisitCallback visitFile,
						  PathView path,
						  bool recursive)
	{
		mVFS.Visit(visitDirectory, visitFile, path, recursive);
	}

	std::shared_ptr<ReadOnlyStream> Container::OpenStream(PathView path)
	{
		const ContainerEntry& e = GetEntry(path);
		const bool newEntry = e.Offset() == 0 && e.Size() == 0;
		return std::make_shared<ReadOnlyStream>(
			newEntry ? std::static_pointer_cast<Stream>(std::make_shared<EmptyStream>()) :
					   std::make_shared<SubStream>(Input(), e.Offset(), e.Size()));
	}

	// File implementation
	void Container::LoadImpl()
	{
		std::shared_ptr stream = Input();
		if (!stream || stream->Size() == 0)
		{
			return;
		}

		Stream& s = *stream;

		s.Seek(0, StreamSeekOrigin::Begin);

		const u64 streamSize = s.Size();
		const i64 pos = streamSize - 4;

		s.Seek(pos, StreamSeekOrigin::Begin);

		const u32 entriesOffset = s.Read<u32>();
		const i64 entriesPos = streamSize - entriesOffset;

		s.Seek(entriesPos, StreamSeekOrigin::Begin);
		const u32 magic = s.Read<u32>();
		Expects(magic == EntriesHeaderMagic);

		const u32 entryCount = s.Read<u32>();
		mEntries.reserve(entryCount);
		for (size i = 0; i < entryCount; ++i)
		{
			const u32 nameHash = s.Read<u32>();
			const u32 unk1 = s.Read<u32>();
			const u32 unk2 = s.Read<u32>();
			const u32 unk3 = s.Read<u32>();
			const u32 unk4 = s.Read<u32>();

			ContainerEntry& e = mEntries.emplace_back(nameHash, unk1, unk2, unk3, unk4);

			Path filePath = Path::Root / HashLookup::Instance().TryGetString(nameHash);
			mVFS.RegisterExistingFile(filePath, nameHash);

			SubStream entryStream{ Input(), e.Offset(), e.Size() };
			e.File = File::New(*this, filePath, File::FindTypeOfStream(entryStream));
		}
	}

	void Container::Save(Stream& s)
	{
		// TODO: Container::Save
		(void)s;
		Expects(false && "Not implemented");
	}

	static u64 SizeAligned(u64 s)
	{
		constexpr u64 Alignment{ 0x1000 };

		u64 r = s % Alignment;
		u64 a = r ? (s + (Alignment - r)) : s;
		return a;
	}

	u64 Container::Size()
	{
		size total = 0;
		total += sizeof(u32);                       // entries offset
		total += sizeof(u32);                       // magic
		total += sizeof(u32);                       // entry count
		total += sizeof(u32) * 5 * mEntries.size(); // entries
		for (const ContainerEntry& e : mEntries)
		{
			u64 s = e.Size();
			if (!(e.Unk1 & 0xF)) // TODO: how to determine if the entry is aligned? can't calculate
								 // the correct size otherwise
			{
				s = SizeAligned(s);
			}
			total += gsl::narrow<u32>(s); // entry data
		}
		return total;
	}

	size Container::GetEntryIndex(PathView path) const
	{
		const size hash = mVFS.GetFileInfo(path);
		return GetEntryIndex(hash);
	}

	size Container::GetEntryIndex(size nameHash) const
	{
		// TODO: don't use linear search here
		for (size i = 0; i < mEntries.size(); ++i)
		{
			if (mEntries[i].NameHash == nameHash)
			{
				return i;
			}
		}

		return static_cast<size>(-1);
	}

	const ContainerEntry& Container::GetEntry(PathView path) const
	{
		const size index = GetEntryIndex(path);
		Expects(index != static_cast<size>(-1));
		return mEntries[index];
	}

	const ContainerEntry& Container::GetEntry(size pathHash) const
	{
		const size index = GetEntryIndex(pathHash);
		Expects(index != static_cast<size>(-1));
		return mEntries[index];
	}

	static bool Validator(Stream& input)
	{
		const u64 size = input.Size();
		if (size < 12) // min required bytes
		{
			return false;
		}

		const i64 pos = size - 4;
		if (pos < 0 || pos >= gsl::narrow<i64>(size))
		{
			return false;
		}

		input.Seek(pos, StreamSeekOrigin::Begin);

		const u32 entriesOffset = input.Read<u32>();
		const i64 entriesPos = size - entriesOffset;
		if (entriesPos < 0 ||
			entriesPos + 4 >=
				gsl::narrow<i64>(size)) // +4 as it should be able to read at least 4 bytes
		{
			return false;
		}

		input.Seek(entriesPos, StreamSeekOrigin::Begin);

		const u32 magic = input.Read<u32>();
		return magic == Container::EntriesHeaderMagic;
	}

	static std::shared_ptr<File> Creator(Device& parent, PathView path)
	{
		return std::make_shared<Container>(parent, path);
	}

	const File::Type Container::Type{ std::hash<std::string_view>{}("Container"),
									  2,
									  &Validator,
									  &Creator };
}

// ifndef because line 'Container& c = *cont;' gets compiler error 'illegal indirection' when
// compiling with tests disabled
#ifndef DOCTEST_CONFIG_DISABLE
TEST_SUITE("Container")
{
	using namespace noire;

	TEST_CASE("Load" * doctest::skip(true))
	{
		LocalDevice d{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\" };
		std::shared_ptr cont = std::dynamic_pointer_cast<Container>(d.Open("/vehicles.big.pc"));
		CHECK(cont != nullptr);
		Container& c = *cont;

		c.Load();

		c.Visit([](PathView path) { std::cout << "Visiting '" << path.String() << "':\n"; },
				[&c](PathView path) {
					const ContainerEntry& e = c.GetEntry(path);

					std::array<char, 512> buffer;
					std::snprintf(
						buffer.data(),
						buffer.size(),
						"\tHash:%08X Unk1:%08X Unk2:%08X Unk3:%08X Unk4:%08X Offset:%016I64X "
						"Size:%016I64X\t",
						e.NameHash,
						e.Unk1,
						e.Unk2,
						e.Unk3,
						e.Unk4,
						e.Offset(),
						e.Size());
					std::cout << buffer.data() << '"' << path.String() << '"' << std::endl;
				},
				PathView::Root,
				true);

		std::cout << "input:  " << d.OpenStream("/vehicles.big.pc")->Size() << std::endl;
		std::cout << "size(): " << c.Size() << std::endl;

		// CHECK_EQ(input->Size(), c.Size()); // NOTE: Container::Size() doesn't work properly yet

		// std::shared_ptr<Stream> output = std::make_shared<FileStream>(
		//	"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\vehicles_copy.big.pc");

		// c.Save(*output);
		// CHECK_EQ(c.Size(), output->Size());
	}
}
#endif
