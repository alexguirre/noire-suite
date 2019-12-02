#include "AttributeFile.h"
#include "Hash.h"
#include <Windows.h>
#include <gsl/gsl>
#include <unordered_set>

namespace noire
{
	std::string_view ToString(EAttributePropertyType type)
	{
		using namespace std::string_view_literals;

		switch (type)
		{
		case EAttributePropertyType::Int32: return "Int32"sv;
		case EAttributePropertyType::UInt32: return "UInt32"sv;
		case EAttributePropertyType::Float: return "Float"sv;
		case EAttributePropertyType::Bool: return "Bool"sv;
		case EAttributePropertyType::Vec3: return "Vec3"sv;
		case EAttributePropertyType::Vec2: return "Vec2"sv;
		case EAttributePropertyType::Mat4: return "Mat4"sv;
		case EAttributePropertyType::AString: return "AString"sv;
		case EAttributePropertyType::UInt64: return "UInt64"sv;
		case EAttributePropertyType::Vec4: return "Vec4"sv;
		case EAttributePropertyType::UString: return "UString"sv;
		case EAttributePropertyType::PolyPtr: return "PolyPtr"sv;
		case EAttributePropertyType::Link: return "Link"sv;
		case EAttributePropertyType::Bitfield: return "Bitfield"sv;
		case EAttributePropertyType::Array: return "Array"sv;
		case EAttributePropertyType::Structure: return "Structure"sv;
		default: Expects(false);
		}
	}

	CAttributeFile::CAttributeFile(fs::IFileStream& stream) : mRoot{ 0, "root", {}, true, {} }
	{
		Load(stream);
	}

	void CAttributeFile::Load(fs::IFileStream& stream)
	{
		stream.Seek(0);
		stream.Read<std::uint32_t>(); // header magic

		ReadCollection(stream, mRoot);
	}

	void CAttributeFile::ReadCollection(fs::IFileStream& stream, SAttributeObject& destCollection)
	{
		Expects(destCollection.IsCollection);

		const std::uint16_t objectCount = stream.Read<std::uint16_t>();
		destCollection.Objects.reserve(objectCount);
		for (std::size_t i = 0; i < objectCount; i++)
		{
			ReadCollectionEntry(stream, destCollection);
		}
	}

	void CAttributeFile::ReadCollectionEntry(fs::IFileStream& stream,
											 SAttributeObject& destCollection)
	{
		Expects(destCollection.IsCollection);

		SAttributeObject object{ 0, {}, {}, false, {} };
		object.DefinitionHash = stream.Read<std::uint32_t>();
		std::uint8_t nameLength = stream.Read<std::uint8_t>();
		object.Name.resize(nameLength);
		stream.Read(object.Name.data(), nameLength);

		ReadObject(stream, object);

		if (IsCollection(object.DefinitionHash))
		{
			object.IsCollection = true;
			ReadCollection(stream, object);
		}
		else
		{
			stream.Read<std::uint16_t>();
		}

		destCollection.Objects.emplace_back(std::move(object));
	}

	void CAttributeFile::ReadObject(fs::IFileStream& stream, SAttributeObject& destObject)
	{
		for (std::uint8_t v = stream.Read<std::uint8_t>(); v != 0; v = stream.Read<std::uint8_t>())
		{
			std::uint32_t propertyNameHash = stream.Read<std::uint32_t>();
			EAttributePropertyType propertyType = static_cast<EAttributePropertyType>(v);

			destObject.Properties.emplace_back(
				std::move(ReadPropertyValue(stream, propertyNameHash, propertyType)));
		}
	}

	SAttributeProperty CAttributeFile::ReadPropertyValue(fs::IFileStream& stream,
														 std::uint32_t propertyNameHash,
														 EAttributePropertyType propertyType)
	{
		SAttributeProperty prop{ propertyNameHash, propertyType, {} };
		switch (propertyType)
		{
		case EAttributePropertyType::Int32: prop.Value = stream.Read<std::int32_t>(); break;
		case EAttributePropertyType::UInt32: prop.Value = stream.Read<std::uint32_t>(); break;
		case EAttributePropertyType::Float: prop.Value = stream.Read<float>(); break;
		case EAttributePropertyType::Bool: prop.Value = stream.Read<bool>(); break;
		case EAttributePropertyType::Vec3:
			prop.Value = SAttributeProperty::Vec3{ stream.Read<float>(),
												   stream.Read<float>(),
												   stream.Read<float>() };
			break;
		case EAttributePropertyType::Vec2:
			prop.Value = SAttributeProperty::Vec2{ stream.Read<float>(), stream.Read<float>() };
			break;
		case EAttributePropertyType::Mat4:
			prop.Value = SAttributeProperty::Mat4{ stream.Read<float>(), stream.Read<float>(),
												   stream.Read<float>(), stream.Read<float>(),
												   stream.Read<float>(), stream.Read<float>(),
												   stream.Read<float>(), stream.Read<float>(),
												   stream.Read<float>(), stream.Read<float>(),
												   stream.Read<float>(), stream.Read<float>(),
												   stream.Read<float>(), stream.Read<float>(),
												   stream.Read<float>(), stream.Read<float>() };
			break;
		case EAttributePropertyType::AString:
		{
			std::uint16_t length = stream.Read<std::uint16_t>();
			std::string str{};
			str.resize(length);
			stream.Read(str.data(), length);
			prop.Value = std::move(str);
		}
		break;
		case EAttributePropertyType::UInt64: prop.Value = stream.Read<std::uint64_t>(); break;
		case EAttributePropertyType::Vec4:
			prop.Value = SAttributeProperty::Vec4{ stream.Read<float>(),
												   stream.Read<float>(),
												   stream.Read<float>(),
												   stream.Read<float>() };
			break;
		case EAttributePropertyType::UString:
		{
			const std::uint16_t byteCount = stream.Read<std::uint16_t>();
			SAttributeProperty::UString str{};
			str.Utf8String.resize(byteCount);
			stream.Read(str.Utf8String.data(), byteCount);
			prop.Value = std::move(str);
		}
		break;
		case EAttributePropertyType::Bitfield:
		{
			// TODO: support for reading EAttributePropertyType::Bitfield
			SkipProperty(stream, propertyType);
		}
		break;
		case EAttributePropertyType::PolyPtr:
		{
			SAttributeProperty::PolyPtr polyPtr{ nullptr };
			std::uint32_t definitionHash = stream.Read<std::uint32_t>();
			if (definitionHash != 0)
			{
				polyPtr.Object = std::make_unique<SAttributeObject>();
				polyPtr.Object->DefinitionHash = definitionHash;
				polyPtr.Object->IsCollection = false;
				ReadObject(stream, *polyPtr.Object);
			}

			prop.Value = std::move(polyPtr);
		}
		break;
		case EAttributePropertyType::Link:
		{
			// TODO: support for reading EAttributePropertyType::Link
			SkipProperty(stream, propertyType);
		}
		break;
		case EAttributePropertyType::Array:
		{
			SAttributeProperty::Array arr;
			arr.ItemType = static_cast<EAttributePropertyType>(stream.Read<std::uint8_t>());
			const std::size_t itemCount = stream.Read<std::uint16_t>();
			arr.Items.reserve(itemCount);
			for (std::size_t i = 0; i < itemCount; i++)
			{
				arr.Items.emplace_back(std::move(ReadPropertyValue(stream, 0, arr.ItemType)));
			}
			prop.Value = std::move(arr);
		}
		break;
		case EAttributePropertyType::Structure:
		{
			SAttributeProperty::Structure struc{ std::make_unique<SAttributeObject>() };
			struc.Object->DefinitionHash = stream.Read<std::uint32_t>();
			struc.Object->IsCollection = false;
			ReadObject(stream, *struc.Object);
			prop.Value = std::move(struc);
		}
		break;
		default: Expects(false); break;
		}

		return prop;
	}

	void CAttributeFile::SkipProperty(fs::IFileStream& stream, EAttributePropertyType propertyType)
	{
		fs::FileStreamSize offset = 0;
		switch (propertyType)
		{
		case EAttributePropertyType::Int32:
		case EAttributePropertyType::UInt32:
		case EAttributePropertyType::Float: offset = 4; break;
		case EAttributePropertyType::Bool: offset = 1; break;
		case EAttributePropertyType::Vec3: offset = 12; break;
		case EAttributePropertyType::Vec2:
		case EAttributePropertyType::UInt64:
		case EAttributePropertyType::Bitfield: offset = 8; break;
		case EAttributePropertyType::Mat4: offset = 64; break;
		case EAttributePropertyType::AString:
		case EAttributePropertyType::UString: offset = stream.Read<std::uint16_t>(); break;
		case EAttributePropertyType::Vec4: offset = 16; break;
		case EAttributePropertyType::PolyPtr:
		{
			std::uint32_t n = stream.Read<std::uint32_t>();
			if (n != 0)
			{
				for (std::uint8_t v = stream.Read<std::uint8_t>(); v != 0;
					 v = stream.Read<std::uint8_t>())
				{
					stream.Seek(stream.Tell() + 4);

					SkipProperty(stream, static_cast<EAttributePropertyType>(v));
				}
			}
		}
		break;
		case EAttributePropertyType::Link: offset = 2; break;
		case EAttributePropertyType::Array:
		{
			EAttributePropertyType type =
				static_cast<EAttributePropertyType>(stream.Read<std::uint8_t>());
			std::uint16_t count = stream.Read<std::uint16_t>();
			for (std::size_t i = 0; i < count; i++)
			{
				SkipProperty(stream, type);
			}
		}
		break;
		case EAttributePropertyType::Structure:
		{
			stream.Seek(stream.Tell() + 4);
			for (std::uint8_t v = stream.Read<std::uint8_t>(); v != 0;
				 v = stream.Read<std::uint8_t>())
			{
				stream.Seek(stream.Tell() + 4);

				SkipProperty(stream, static_cast<EAttributePropertyType>(v));
			}
		}
		break;
		default: Expects(false); break;
		}

		stream.Seek(stream.Tell() + offset);
	}

	bool CAttributeFile::IsValid(fs::IFileStream& stream)
	{
		const auto resetStreamPos = gsl::finally([&stream]() { stream.Seek(0); });

		const fs::FileStreamSize streamSize = stream.Size();
		if (streamSize <= 4) // min required bytes
		{
			return false;
		}

		const std::uint32_t magic = stream.Read<std::uint32_t>();
		constexpr std::uint32_t ExpectedMagic{ 0x00425441 }; // 'ATB'
		return (magic & 0x00FFFFFF) == ExpectedMagic;
	}

	bool CAttributeFile::IsCollection(std::uint32_t definitionHash)
	{
		static const std::unordered_set<std::uint32_t> CollectionDefinitions{
			crc32("act"),
			crc32("actormanagersettings"),
			crc32("animationgroup"),
			crc32("animationsettings"),
			crc32("assignedcase"),
			crc32("brawlinginterrogationconversation"),
			crc32("case"),
			crc32("caseactor"),
			crc32("charactermanagersettings"),
			crc32("clueconversation"),
			crc32("constrainedconversation"),
			crc32("conversationanimationgroup"),
			crc32("conversationbase"),
			crc32("customertype"),
			crc32("dlcfolder"),
			crc32("deadbodysettings"),
			crc32("debugpickersettings"),
			crc32("decalmanagersettings"),
			crc32("demographicsettings"),
			crc32("desk"),
			crc32("evadeglobalsettings"),
			crc32("exitnotebookconversation"),
			crc32("exposedcollection"),
			crc32("foliagemanagersettings"),
			crc32("gamewellconversation"),
			crc32("generalaimsettings"),
			crc32("getupanimationgroup"),
			crc32("gridswapcollection"),
			crc32("guncombatsquad"),
			crc32("inspectionsession"),
			crc32("newact"),
			crc32("notebookconversation"),
			crc32("notebookentrycollection"),
			crc32("notebookpagetemplateset"),
			crc32("onchargedconversation"),
			crc32("partnerconversation"),
			crc32("pedestriansettings"),
			crc32("policestation"),
			crc32("postprocesssettings"),
			crc32("propmanagersettings"),
			crc32("roletype"),
			crc32("savecollection"),
			crc32("scriptedsequenceconversation"),
			crc32("steeringpathsettingscollection"),
			crc32("streamedcollection"),
			crc32("streamingcollection"),
			crc32("streetcrimeresponseconversation"),
			crc32("targetrangeinstance"),
			crc32("targetrangesettings"),
			crc32("testcase"),
			crc32("tiledmapicons"),
			crc32("toggleablecollection"),
			crc32("turnuncooperativeconversation"),
			crc32("uibranchselection"),
			crc32("uibusynotification"),
			crc32("uicasecompletescreen"),
			crc32("uicasecompletionstats"),
			crc32("uicaselistlines"),
			crc32("uicasetitle"),
			crc32("uicasesmenu3d"),
			crc32("uicollection"),
			crc32("uicontrollerconfiglines"),
			crc32("uicontrollerconfiglinesx360"),
			crc32("uicredits"),
			crc32("uicreditsscroller"),
			crc32("uidlcstore"),
			crc32("uielement"),
			crc32("uiestablishingshotlayer"),
			crc32("uiextrasmenu3d"),
			crc32("uifailurescreen"),
			crc32("uifullmap"),
			crc32("uiicon"),
			crc32("uiicondynamic"),
			crc32("uiinsertdisc"),
			crc32("uiinspectionicon"),
			crc32("uiinstallscreen"),
			crc32("uilayer"),
			crc32("uilegalsscreen"),
			crc32("uilegendlayer"),
			crc32("uilogscreen"),
			crc32("uilogscreenlines"),
			crc32("uimainmenu3d"),
			crc32("uimapatlasicon"),
			crc32("uimaplegend"),
			crc32("uimaplegendicons"),
			crc32("uimaplegendlabels"),
			crc32("uimaplocationinfo"),
			crc32("uimaplocationlabel"),
			crc32("uimaplocationlabeltext"),
			crc32("uimenu"),
			crc32("uiminimap"),
			crc32("uimousepointer"),
			crc32("uinewspaper"),
			crc32("uinewspaperclose"),
			crc32("uinewspaperopen"),
			crc32("uinotebookupdate"),
			crc32("uinotebookupdateelement"),
			crc32("uioptionsaimmenu"),
			crc32("uioptionscameramenu"),
			crc32("uioptionscontrolsconfigmenu"),
			crc32("uioptionscontrolsconfigmenux360"),
			crc32("uioptionscontrolsmenu"),
			crc32("uioptionsdisplaymenu"),
			crc32("uioptionsdisplayrendersettingsmenu"),
			crc32("uioptionsgamemenu"),
			crc32("uioptionsgammamenu"),
			crc32("uioptionsmenu"),
			crc32("uioptionssoundmenu"),
			crc32("uioutfitselection"),
			crc32("uipausemenu"),
			crc32("uirendersettingslines"),
			crc32("uisaveselect"),
			crc32("uisaveselectlines"),
			crc32("uishield"),
			crc32("uisocialclub"),
			crc32("uisocialclubagecheck"),
			crc32("uisocialclubdocselect"),
			crc32("uisocialclubintro"),
			crc32("uisocialclubnews"),
			crc32("uisocialclubpasswordreset"),
			crc32("uisocialclubsignin"),
			crc32("uisocialclubtos"),
			crc32("uisocialclubwelcome"),
			crc32("uistatsscreen"),
			crc32("uistatsscreenlines"),
			crc32("uistreamedfolder"),
			crc32("uistreamedtexture"),
			crc32("uistreamedtexturescreen"),
			crc32("uistreamingscreen"),
			crc32("uistring"),
			crc32("uisubtitlelayer"),
			crc32("uisurface"),
			crc32("uitextbox"),
			crc32("uititlecardscreen"),
			crc32("uitutoriallayer"),
			crc32("uiunassignedcasetitle"),
			crc32("uiwindow"),
			crc32("uiyesno"),
			crc32("unassignedcase"),
			crc32("unconstrainedconversation"),
			crc32("unusedobjectscollection"),
			crc32("vehicleconversation"),
			crc32("vehicleshowroom"),
			crc32("vehicleshowroominfo"),
			crc32("weathermanagersettings"),
			crc32("workertype"),
			crc32("worldbookmarkcollection"),
		};

		return CollectionDefinitions.find(definitionHash) != CollectionDefinitions.end();
	}
}