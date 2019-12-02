#pragma once
#include "File.h"
#include "fs/FileStream.h"
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace noire
{
	enum class EAttributePropertyType : std::uint8_t
	{
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

	std::string_view ToString(EAttributePropertyType type);

	struct SAttributeObject;

	struct SAttributeProperty
	{
		using Vec2 = std::array<float, 2>;
		using Vec3 = std::array<float, 3>;
		using Vec4 = std::array<float, 4>;
		using Mat4 = std::array<float, 16>;
		struct AString
		{
			std::string AsciiString;
		};
		struct UString
		{
			std::string Utf8String;
		};
		struct PolyPtr
		{
			std::unique_ptr<SAttributeObject> Object;
		};
		struct LinkStorage
		{
			std::uint16_t Id;
			std::vector<std::uint32_t> ScopedNameHashes;
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
			EAttributePropertyType ItemType;
			std::vector<SAttributeProperty> Items;
		};
		struct Structure
		{
			std::unique_ptr<SAttributeObject> Object;
		};

		using ValueVariant = std::variant<std::monostate,
										  std::int32_t,
										  std::uint32_t,
										  float,
										  bool,
										  Vec3,
										  Vec2,
										  Mat4,
										  AString,
										  std::uint64_t,
										  Vec4,
										  UString,
										  PolyPtr,
										  Link,
										  Array,
										  Structure>;

		std::uint32_t NameHash;
		EAttributePropertyType Type;
		ValueVariant Value;
	};

	struct SAttributeObject
	{
		std::uint32_t DefinitionHash;
		std::string Name;
		std::vector<SAttributeProperty> Properties;
		bool IsCollection;
		std::vector<SAttributeObject> Objects;
	};

	class CAttributeFile
	{
	public:
		CAttributeFile(fs::IFileStream& stream);

		const SAttributeObject& Root() const { return mRoot; }

	private:
		void Load(fs::IFileStream& stream);
		void ReadCollection(fs::IFileStream& stream, SAttributeObject& destCollection);
		void ReadCollectionEntry(fs::IFileStream& stream, SAttributeObject& destCollection);
		void ReadObject(fs::IFileStream& stream, SAttributeObject& destObject);
		SAttributeProperty ReadPropertyValue(fs::IFileStream& stream,
											 std::uint32_t propertyNameHash,
											 EAttributePropertyType propertyType);
		void SkipProperty(fs::IFileStream& stream, EAttributePropertyType propertyType);
		void ResolveLinks(fs::IFileStream& stream);

		SAttributeObject mRoot;
		std::vector<SAttributeProperty::LinkStorage*> mLinksToResolve;

	public:
		static bool IsValid(fs::IFileStream& stream);

	private:
		static bool IsCollection(std::uint32_t definitionHash);
	};

	template<>
	struct TFileTraits<CAttributeFile>
	{
		static constexpr bool IsCollection{ false };
		static bool IsValid(fs::IFileStream& stream) { return CAttributeFile::IsValid(stream); }
	};
}