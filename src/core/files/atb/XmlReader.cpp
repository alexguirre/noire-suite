#include "XmlReader.h"
#include "Common.h"
#include "Hash.h"
#include <cstdio>
#include <cstring>
#include <doctest/doctest.h>
#include <iostream>

namespace noire::atb
{
	using namespace pugi;

	static Object DefaultObject() { return { 0, {}, {}, false, {} }; };

	XmlReader::XmlReader() {}

	static Int32 ParseInt32(const char_t* str)
	{
		Int32 i;
		Expects(sscanf_s(str, "%d", &i) == 1);
		return i;
	}

	static UInt32 ParseUInt32(const char_t* str)
	{
		UInt32 u;
		Expects(sscanf_s(str, "%u", &u) == 1);
		return u;
	}

	static Float ParseFloat(const char_t* str)
	{
		Float f;
		Expects(sscanf_s(str, "%f", &f) == 1);
		return f;
	}

	static Bool ParseBool(const char_t* str)
	{
		constexpr char True[]{ "true" };
		constexpr char False[]{ "false" };

		if (std::strncmp(str, True, std::size(True)) == 0)
		{
			return true;
		}
		else if (std::strncmp(str, False, std::size(False)) == 0)
		{
			return false;
		}

		Expects(false);
	}

	static Vec2 ParseVec2(const char_t* str)
	{
		Vec2 v;
		Expects(sscanf_s(str, "%f, %f", &v[0], &v[1]) == 2);
		return v;
	}

	static Vec3 ParseVec3(const char_t* str)
	{
		Vec3 v;
		Expects(sscanf_s(str, "%f, %f, %f", &v[0], &v[1], &v[2]) == 3);
		return v;
	}

	static Mat4 ParseMat4(const char_t* str)
	{
		Mat4 v;
		// clang-format off
		Expects(sscanf_s(str,
						"%f, %f, %f, %f, "
						"%f, %f, %f, %f, "
						"%f, %f, %f, %f, "
						"%f, %f, %f, %f",
						&v[0], &v[1], &v[2], &v[3],
						&v[4], &v[5], &v[6], &v[7],
						&v[8], &v[9], &v[10], &v[11],
						&v[12], &v[13], &v[14], &v[15]) == 16);
		// clang-format on
		return v;
	}

	static AString ParseAString(const char_t* str) { return AString{ { str } }; }

	static Vec4 ParseVec4(const char_t* str)
	{
		Vec4 v;
		Expects(sscanf_s(str, "%f, %f, %f, %f", &v[0], &v[1], &v[2], &v[3]) == 4);
		return v;
	}

	static UString ParseUString(const char_t* str) { return UString{ { str } }; }

	static Bitfield ParseBitfield(const char_t* str)
	{
		Bitfield b;
		Expects(sscanf_s(str, "%u,%u", &b.Mask, &b.Flags) == 1);
		return b;
	}

	Object XmlReader::ReadObject(xml_node node)
	{
		Expects(node.type() == xml_node_type::node_element);

		Object root{ DefaultObject() };

		root.DefinitionHash = crc32Lowercase(node.name());
		for (xml_node n : node.children())
		{
			Property& prop = root.Properties.emplace_back();
			prop.NameHash = crc32Lowercase(n.attribute("name").value());
			prop.Type = ValueTypeFromString(n.name());
			const char_t* const str = n.child_value();
			switch (prop.Type)
			{
			case ValueType::Int32: prop.Value = ParseInt32(str); break;
			case ValueType::UInt32: prop.Value = ParseUInt32(str); break;
			case ValueType::Float: prop.Value = ParseFloat(str); break;
			case ValueType::Bool: prop.Value = ParseBool(str); break;
			case ValueType::Vec3: prop.Value = ParseVec3(str); break;
			case ValueType::Vec2: prop.Value = ParseVec2(str); break;
			case ValueType::Mat4: prop.Value = ParseMat4(str); break;
			case ValueType::AString: prop.Value = ParseAString(str); break;
			case ValueType::UInt64: prop.Value = UInt64{}; break; // TODO: UInt64
			case ValueType::Vec4: prop.Value = ParseVec4(str); break;
			case ValueType::UString: prop.Value = ParseUString(str); break;
			case ValueType::PolyPtr: prop.Value = PolyPtr{}; break; // TODO: UInt64
			case ValueType::Link: prop.Value = Link{}; break;       // TODO: Link
			case ValueType::Bitfield: prop.Value = ParseBitfield(str); break;
			case ValueType::Array: prop.Value = Array{}; break;         // TODO: Link
			case ValueType::Structure: prop.Value = Structure{}; break; // TODO: Link
			}
		}

		return root;
	}
}

#ifndef DOCTEST_CONFIG_DISABLE
TEST_SUITE("atb::XmlReader")
{
	using namespace noire;
	using namespace noire::atb;

	static void TraverseObject(const Object& o, size depth = 0)
	{
		std::cout << std::string(depth * 2, ' ') << o.Name << '\n';
		for (const Property& p : o.Properties)
		{
			std::cout << std::string((depth + 1) * 2, ' ') << "- "
					  << HashLookup::Instance(false).TryGetString(p.NameHash);
			if (p.Type == ValueType::AString)
			{
				std::cout << " = " << std::get<AString>(p.Value).AsciiString;
			}

			std::cout << '\n';
		}

		if (o.IsCollection)
		{
			for (const Object& c : o.Objects)
			{
				TraverseObject(c, depth + 1);
			}
		}
	}

	TEST_CASE("Read" * doctest::skip(false))
	{
		const std::string_view xml = "\
<MetaTypeIconObj>\
	<AString name=\"IconFilename\">act.ico</AString>\
	<AString name=\"FolderIconFilename\">acts.ico</AString>\
</MetaTypeIconObj>";

		pugi::xml_document doc;
		doc.load_buffer(xml.data(), xml.size());
		Object root = XmlReader{}.ReadObject(doc.root().first_child());

		TraverseObject(root);
	}
}
#endif
