#include "Writer.h"
#include "Common.h"
#include "Reader.h"
#include "Types.h"
#include "streams/FileStream.h"
#include "streams/Stream.h"
#include <algorithm>
#include <doctest/doctest.h>

namespace noire::atb
{
	Writer::Writer(Stream& stream) : mStream{ stream } {}

	void Writer::Write(const Object& root)
	{
		// TODO: atb::Writer::Write
		(void)root;

		Stream& s = mStream;

		s.Write<u32>(HeaderMagic | 0x04000000); // value 04 is what the game's root.atb.pc

		WriteCollection(root);

		WriteLinks();

		mLinks.clear();
	}

	void Writer::WriteCollection(const Object& collection)
	{
		Expects(collection.IsCollection);

		Stream& s = mStream;

		s.Write(gsl::narrow<u16>(collection.Objects.size()));
		for (const Object& c : collection.Objects)
		{
			WriteCollectionEntry(c);
		}
	}
	void Writer::WriteCollectionEntry(const Object& entry)
	{
		Stream& s = mStream;

		s.Write(entry.DefinitionHash);
		s.Write(gsl::narrow<u8>(entry.Name.size()));
		s.Write(entry.Name.data(), entry.Name.size());

		WriteObject(entry);

		if (entry.IsCollection)
		{
			WriteCollection(entry);
		}
		else
		{
			// this is the collection entry count, since this entry is not a collection this
			// should be 0
			s.Write<u16>(0);
		}
	}

	void Writer::WriteObject(const Object& object)
	{
		Stream& s = mStream;

		for (const Property& prop : object.Properties)
		{
			s.Write(prop.Type);
			s.Write(prop.NameHash);
			WritePropertyValue(prop);
		}

		s.Write(ValueType::Invalid);
	}

	void Writer::WritePropertyValue(const Property& prop)
	{
		Stream& s = mStream;

		switch (prop.Type)
		{
		case ValueType::Int32: s.Write(std::get<Int32>(prop.Value)); break;
		case ValueType::UInt32: s.Write(std::get<UInt32>(prop.Value)); break;
		case ValueType::Float: s.Write(std::get<Float>(prop.Value)); break;
		case ValueType::Bool: s.Write(std::get<Bool>(prop.Value)); break;
		case ValueType::Vec3: s.Write(std::get<Vec3>(prop.Value)); break;
		case ValueType::Vec2: s.Write(std::get<Vec2>(prop.Value)); break;
		case ValueType::Mat4: s.Write(std::get<Mat4>(prop.Value)); break;
		case ValueType::AString:
		{
			const AString& str = std::get<AString>(prop.Value);
			s.Write<u16>(gsl::narrow<u16>(str.AsciiString.size()));
			s.Write(str.AsciiString.data(), str.AsciiString.size());
		}
		break;
		case ValueType::UInt64: s.Write(std::get<UInt64>(prop.Value)); break;
		case ValueType::Vec4: s.Write(std::get<Vec4>(prop.Value)); break;
		case ValueType::UString:
		{
			const UString& str = std::get<UString>(prop.Value);
			s.Write<u16>(gsl::narrow<u16>(str.Utf8String.size()));
			s.Write(str.Utf8String.data(), str.Utf8String.size());
		}
		break;
		case ValueType::Bitfield:
		{
			const Bitfield& bitfield = std::get<Bitfield>(prop.Value);
			s.Write(bitfield.Mask);
			s.Write(bitfield.Flags);
		}
		break;
		case ValueType::PolyPtr:
		{
			const PolyPtr& polyPtr = std::get<PolyPtr>(prop.Value);
			if (polyPtr.Object)
			{
				s.Write(polyPtr.Object->DefinitionHash);
				WriteObject(*polyPtr.Object);
			}
			else
			{
				s.Write<u32>(0); // definition hash
			}
		}
		break;
		case ValueType::Link:
		{
			const Link& link = std::get<Link>(prop.Value);
			if (link.Storage)
			{
				s.Write(link.Storage->Id);
				mLinks.emplace_back(link.Storage.get());
			}
			else
			{
				s.Write(LinkStorage::InvalidId);
			}
		}
		break;
		case ValueType::Array:
		{
			const Array& arr = std::get<Array>(prop.Value);
			s.Write(arr.ItemType);
			s.Write(gsl::narrow<u16>(arr.Items.size()));
			for (const Property& item : arr.Items)
			{
				WritePropertyValue(item);
			}
		}
		break;
		case ValueType::Structure:
		{
			const Structure& struc = std::get<Structure>(prop.Value);
			s.Write(struc.Object->DefinitionHash);
			WriteObject(*struc.Object);
		}
		break;
		default: Expects(false); break;
		}
	}

	void Writer::WriteLinks()
	{
		Stream& s = mStream;

		std::sort(mLinks.begin(), mLinks.end(), [](const LinkStorage* a, const LinkStorage* b) {
			return a->Id < b->Id;
		});

		// remove repeated links
		mLinks.erase(
			std::unique(mLinks.begin(),
						mLinks.end(),
						[](const LinkStorage* a, const LinkStorage* b) { return a->Id == b->Id; }),
			mLinks.end());

		s.Write(gsl::narrow<u16>(mLinks.size()));
		for (const LinkStorage* l : mLinks)
		{
			Ensures(l != nullptr);

			s.Write(gsl::narrow<u8>(l->ScopedNameHashes.size()));
			for (u32 h : l->ScopedNameHashes)
			{
				s.Write(h);
			}
		}
	}
}

#ifndef DOCTEST_CONFIG_DISABLE
TEST_SUITE("atb::Writer")
{
	using namespace noire;
	using namespace noire::atb;

	TEST_CASE("Read/Write" * doctest::skip(true))
	{
		FileStream in{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\root.atb.pc" };
		const Object root = Reader{ in }.Read();

		FileStream out{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\root_copy.atb.pc" };
		Writer{ out }.Write(root);

		CHECK_EQ(in.Size(), out.Size());
	}
}
#endif
