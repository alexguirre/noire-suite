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

	ValueVariant DefaultValue(ValueType type)
	{
		switch (type)
		{
		case ValueType::Int32: return Int32{};
		case ValueType::UInt32: return UInt32{};
		case ValueType::Float: return Float{};
		case ValueType::Bool: return Bool{};
		case ValueType::Vec3: return Vec3{};
		case ValueType::Vec2: return Vec2{};
		case ValueType::Mat4: return Mat4{};
		case ValueType::AString: return AString{};
		case ValueType::UInt64: return UInt64{};
		case ValueType::Vec4: return Vec4{};
		case ValueType::UString: return UString{};
		case ValueType::PolyPtr: return PolyPtr{};
		case ValueType::Link: return Link{};
		case ValueType::Bitfield: return Bitfield{};
		case ValueType::Array: return Array{};
		case ValueType::Structure: return Structure{};
		default: return Invalid{};
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
