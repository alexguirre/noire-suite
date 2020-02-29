#include "Types.h"
#include "Hash.h"

namespace noire::atb
{
	std::string Link::ScopedName() const
	{
		if (!Storage)
		{
			using namespace std::string_literals;
			return "null"s;
		}

		std::string str{};
		char sep = '\0';
		for (u32 h : Storage->ScopedNameHashes)
		{
			if (!sep)
			{
				sep = '.';
			}
			else
			{
				str += sep;
			}
			str += HashLookup::Instance(false).TryGetString(h);
		}

		return str;
	}

	std::string_view ValueTypeToString(ValueType type)
	{
		using namespace std::string_view_literals;

		switch (type)
		{
		case ValueType::Int32: return "Int32"sv;
		case ValueType::UInt32: return "UInt32"sv;
		case ValueType::Float: return "Float"sv;
		case ValueType::Bool: return "Bool"sv;
		case ValueType::Vec3: return "Vec3"sv;
		case ValueType::Vec2: return "Vec2"sv;
		case ValueType::Mat4: return "Mat4"sv;
		case ValueType::AString: return "AString"sv;
		case ValueType::UInt64: return "UInt64"sv;
		case ValueType::Vec4: return "Vec4"sv;
		case ValueType::UString: return "UString"sv;
		case ValueType::PolyPtr: return "PolyPtr"sv;
		case ValueType::Link: return "Link"sv;
		case ValueType::Bitfield: return "Bitfield"sv;
		case ValueType::Array: return "Array"sv;
		case ValueType::Structure: return "Structure"sv;
		default: Expects(false);
		}
	}

	Property& Object::Get(std::string_view propName)
	{
		return const_cast<Property&>(const_cast<const Object*>(this)->Get(propName));
	}

	const Property& Object::Get(std::string_view propName) const
	{
		const u32 propNameHash = crc32Lowercase(propName);
		for (const Property& prop : Properties)
		{
			if (prop.NameHash == propNameHash)
			{
				return prop;
			}
		}

		Expects(false);
	}

	Object& Object::operator[](std::string_view objName)
	{
		return const_cast<Object&>(const_cast<const Object*>(this)->operator[](objName));
	}

	const Object& Object::operator[](std::string_view objName) const
	{
		Expects(IsCollection);

		for (const Object& obj : Objects)
		{
			if (obj.Name == objName)
			{
				return obj;
			}
		}

		Expects(false);
	}
}
