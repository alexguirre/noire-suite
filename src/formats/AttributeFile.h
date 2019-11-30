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
		struct Array
		{
			EAttributePropertyType ItemType;
			std::vector<SAttributeProperty> Items;
		};
		using ValueVariant = std::variant<std::monostate,
										  std::int32_t,
										  std::uint32_t,
										  float,
										  bool,
										  Vec3,
										  Vec2,
										  Mat4,
										  std::string,
										  std::uint64_t,
										  Vec4,
										  Array,
										  std::unique_ptr<SAttributeObject>>;

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

		SAttributeObject mRoot;

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