#include "AttributeFile.h"
#include "Hash.h"
#include "atb/Reader.h"
#include "atb/Writer.h"
#include "devices/LocalDevice.h"
#include "streams/Stream.h"
#include <doctest/doctest.h>
#include <functional>
#include <iostream>
#include <string>
#include <utility>

namespace noire
{
	AttributeFile::AttributeFile(Device& parent, PathView path, bool created)
		: File(parent, path, created), mRoot{}
	{
	}

	void AttributeFile::LoadImpl()
	{
		Stream& s = Raw();
		if (s.Size() == 0)
		{
			return;
		}

		s.Seek(0, StreamSeekOrigin::Begin);

		mRoot = atb::Reader{ s }.Read();
	}

	void AttributeFile::Save()
	{
		Stream& s = Raw();
		s.Seek(0, StreamSeekOrigin::Begin);
		atb::Writer{ s }.Write(mRoot);
	}

	u64 AttributeFile::Size()
	{
		// TODO: AttributeFile::Size
		return File::Size();
	}

	bool AttributeFile::HasChanged() const
	{
		// TODO: AttributeFile::HasChanged
		return File::HasChanged();
	}

	static bool Validator(Stream& input)
	{
		if (input.Size() < 6) // min required bytes (magic + root object count)
		{
			return false;
		}

		const u32 magic = input.ReadAt<u32>(0);
		return (magic & 0x00FFFFFF) == atb::HeaderMagic;
	}

	static std::shared_ptr<File> Creator(Device& parent, PathView path, bool created)
	{
		return std::make_shared<AttributeFile>(parent, path, created);
	}

	const File::TypeDefinition AttributeFile::Type{ std::hash<std::string_view>{}("AttributeFile"),
													1,
													&Validator,
													&Creator };
}

#ifndef DOCTEST_CONFIG_DISABLE
TEST_SUITE("AttributeFile")
{
	using namespace noire;

	static void TraverseObject(const atb::Object& o, size depth = 0)
	{
		std::cout << std::string(depth * 2, ' ') << o.Name << '\n';
		for (const atb::Property& p : o.Properties)
		{
			std::cout << std::string((depth + 1) * 2, ' ') << "- "
					  << HashLookup::Instance(false).TryGetString(p.NameHash) << '\n';
		}

		if (o.IsCollection)
		{
			for (const atb::Object& c : o.Objects)
			{
				TraverseObject(c, depth + 1);
			}
		}
	}

	TEST_CASE("Load" * doctest::skip(true))
	{
		LocalDevice d{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\" };
		std::shared_ptr attr = std::dynamic_pointer_cast<AttributeFile>(d.Open("/root.atb.pc"));
		CHECK(attr != nullptr);
		AttributeFile& a = *attr;

		a.Load();

		TraverseObject(a.Root());
	}
}
#endif