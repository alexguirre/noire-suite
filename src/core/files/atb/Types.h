#pragma once
#include "Common.h"
#include "files/File.h"
#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace noire::atb
{
	inline constexpr u32 HeaderMagic{ 0x00425441 }; // 'ATB'

	enum class ValueType : u8
	{
		Invalid = 0,
		Int32 = 1,
		UInt32 = 2,
		Float = 3,
		Bool = 4,
		Vec3 = 5,
		Vec2 = 6,
		Mat4 = 7,
		AString = 8,
		UInt64 = 9,
		Vec4 = 10,
		UString = 11,
		PolyPtr = 30,
		Link = 40,
		Bitfield = 50,
		Array = 60,
		Structure = 70,
	};

	std::string_view ValueTypeToString(ValueType type);

	struct Object;
	struct Property;

	using Invalid = std::monostate;

	using Int32 = i32;
	using UInt32 = u32;
	using Float = float;
	using Bool = bool;
	using UInt64 = u64;

	using Vec2 = std::array<Float, 2>;
	using Vec3 = std::array<Float, 3>;
	using Vec4 = std::array<Float, 4>;
	using Mat4 = std::array<Float, 16>;

	struct AString
	{
		std::string AsciiString;
	};

	struct UString
	{
		std::string Utf8String;
	};

	struct Bitfield
	{
		/* Note:
		 * Value used at runtime is calculated with
		 *        (Flags & Mask) | (defaultValue & ~Mask)
		 * where defaultValue is the value set in the constructor of the object (not really
		 * possible to extract it automatically since it is not included in the Attribute System
		 * definitions).
		 */

		u32 Mask{ 0 };
		u32 Flags{ 0 };
	};

	struct PolyPtr
	{
		std::unique_ptr<Object> Object;
	};

	struct LinkStorage
	{
		static constexpr u16 InvalidId{ 0xFFFF };

		u16 Id{ InvalidId };
		std::vector<u32> ScopedNameHashes;
	};

	struct Link
	{
		// A pointer so we can keep a reference to the link for resolving it after reading the
		// root.
		std::unique_ptr<LinkStorage> Storage;

		std::string ScopedName() const;
	};

	struct Array
	{
		ValueType ElementType{ ValueType::Invalid };
		std::vector<Property> Elements;
	};

	struct Structure
	{
		std::unique_ptr<Object> Object;
	};

	using ValueVariant = std::variant<Invalid,
									  Int32,
									  UInt32,
									  Float,
									  Bool,
									  Vec3,
									  Vec2,
									  Mat4,
									  AString,
									  UInt64,
									  Vec4,
									  UString,
									  Bitfield,
									  PolyPtr,
									  Link,
									  Array,
									  Structure>;

	struct Property
	{
		u32 NameHash{ 0 };
		ValueType Type{ ValueType::Invalid };
		ValueVariant Value{ Invalid{} };
	};

	struct Object
	{
		u32 DefinitionHash{ 0 };
		std::string Name;
		std::vector<Property> Properties;
		bool IsCollection{ false };
		std::vector<Object> Objects;

		Property& Get(std::string_view propName);
		const Property& Get(std::string_view propName) const;

		Object& operator[](std::string_view objName);
		const Object& operator[](std::string_view objName) const;
	};
}
