#pragma once
#include "Types.h"
#include <initializer_list>
#include <string_view>
#include <utility>
#include <vector>

namespace noire::atb
{
	enum class BaseType
	{
		Invalid = 0,
		Object = 1,
		Structure = 2,
		Collection = 4,
		MetaData = 8,
		PolymorphicStructure = 16,
	};

	struct MetaData
	{
		std::vector<Object> MetaObjs{};
	};

	struct PropertyDefinition
	{
		std::string Name{};
		ValueType Type{ ValueType::Invalid };
		ValueVariant Default{ Invalid{} };
		MetaData MetaData{};

		PropertyDefinition(std::string_view name, ValueType type, ValueVariant defaultValue)
			: Name{ name }, Type{ type }, Default{ std::move(defaultValue) }
		{
		}

		PropertyDefinition(std::string_view name, ValueType type)
			: Name{ name }, Type{ type }, Default{ DefaultValue(type) }
		{
		}

		PropertyDefinition(const PropertyDefinition&) = delete;
		PropertyDefinition(PropertyDefinition&&) = default;

		PropertyDefinition& operator=(const PropertyDefinition&) = delete;
		PropertyDefinition& operator=(PropertyDefinition&&) = default;
	};

	struct Definition
	{
		std::string Name{};
		BaseType BaseType{ BaseType::Invalid };
		std::vector<PropertyDefinition> Properties{};
		std::string Hierarchy{};
		MetaData MetaData{};

		Definition(std::string name,
				   atb::BaseType baseType,
				   std::vector<PropertyDefinition> properties)
			: Name{ std::move(name) }, BaseType{ baseType }, Properties{ std::move(properties) }
		{
		}

		Definition(std::string name,
				   atb::BaseType baseType,
				   std::string hierarchy,
				   std::vector<PropertyDefinition> properties)
			: Name{ std::move(name) },
			  BaseType{ baseType },
			  Hierarchy{ std::move(hierarchy) },
			  Properties{ std::move(properties) }
		{
		}

		Definition(const Definition&) = delete;
		Definition(Definition&&) = default;

		Definition& operator=(const Definition&) = delete;
		Definition& operator=(Definition&&) = default;
	};

	// std::array<Definition, 2> defs{
	//	{ { "VehicleSpawnPoint",
	//		BaseType::Structure,
	//		{ { "Vehicle", ValueType::Link },
	//		  { "Transform", ValueType::Mat4, Mat4Identity },
	//		  { "HeadlightsOn", ValueType::Bool, false } } },
	//	  { "MapIcon",
	//		BaseType::Object,
	//		"ExposedPolymorphicStructure.ExposedObject.MapIcon",
	//		{ { "MapIconTextureAtlas", ValueType::PolyPtr },
	//		  { "TintColor", ValueType::Vec4, Vec4{ 1.0f, 1.0f, 1.0f, 1.0f } },
	//		  { "BaseAlpha", ValueType::Float, 1.0f },
	//		  { "LabelStringId", ValueType::AString },
	//		  { "LegendLabelStringId", ValueType::AString } } } }
	//};
}
