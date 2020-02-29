#include "Reader.h"
#include "Common.h"
#include "Hash.h"
#include "streams/FileStream.h"
#include "streams/Stream.h"
#include <doctest/doctest.h>
#include <iostream>
#include <unordered_set>
#include <utility>

namespace noire::atb
{
	static bool DefIsCollection(u32 definitionHash);
	static Object DefaultRoot() { return { 0, "root", {}, true, {} }; };
	static Object DefaultObject() { return { 0, {}, {}, false, {} }; };

	Reader::Reader(Stream& stream) : mStream{ stream }, mLinksToResolve{} {}

	Object Reader::Read()
	{
		Stream& s = mStream;

		const u32 magic = s.Read<u32>();
		Expects((magic & 0x00FFFFFF) == HeaderMagic);
		// TODO: what does this byte mean? vanilla 'root.atb.pc' has 0x04 in here
		// const u32 unk = (magic & 0xFF000000) >> 24;

		Object root{ DefaultRoot() };
		ReadCollection(root);

		ResolveLinks();

		mLinksToResolve.clear();

		return root;
	}

	void Reader::ReadCollection(Object& destCollection)
	{
		Expects(destCollection.IsCollection);

		Stream& s = mStream;

		const u16 objectCount = s.Read<u16>();
		destCollection.Objects.reserve(objectCount);
		for (size i = 0; i < objectCount; i++)
		{
			ReadCollectionEntry(destCollection);
		}
	}

	void Reader::ReadCollectionEntry(Object& destCollection)
	{
		Expects(destCollection.IsCollection);

		Stream& s = mStream;

		Object obj{ DefaultObject() };
		obj.DefinitionHash = s.Read<u32>();
		const u8 nameLength = s.Read<u8>();
		obj.Name.resize(nameLength);
		s.Read(obj.Name.data(), nameLength);

		ReadObject(obj);

		if (DefIsCollection(obj.DefinitionHash))
		{
			obj.IsCollection = true;
			ReadCollection(obj);
		}
		else
		{
			// this is the collection object count, since this object is not a collection this
			// should be 0 (if it is not DefIsCollection may be missing some hash)
			const u16 c = s.Read<u16>();
			Expects(c == 0);
		}

		destCollection.Objects.emplace_back(std::move(obj));
	}

	void Reader::ReadObject(Object& destObject)
	{
		Stream& s = mStream;

		for (ValueType propType = s.Read<ValueType>(); propType != ValueType::Invalid;
			 propType = s.Read<ValueType>())
		{
			const u32 propNameHash = s.Read<u32>();

			destObject.Properties.emplace_back(
				std::move(ReadPropertyValue(propNameHash, propType)));
		}
	}

	Property Reader::ReadPropertyValue(u32 propNameHash, ValueType propType)
	{
		Stream& s = mStream;

		Property prop{ propNameHash, propType, {} };
		switch (propType)
		{
		case ValueType::Int32: prop.Value = s.Read<Int32>(); break;
		case ValueType::UInt32: prop.Value = s.Read<UInt32>(); break;
		case ValueType::Float: prop.Value = s.Read<Float>(); break;
		case ValueType::Bool: prop.Value = s.Read<Bool>(); break;
		case ValueType::Vec3: prop.Value = s.Read<Vec3>(); break;
		case ValueType::Vec2: prop.Value = s.Read<Vec2>(); break;
		case ValueType::Mat4: prop.Value = s.Read<Mat4>(); break;
		case ValueType::AString:
		{
			const u16 length = s.Read<u16>();
			AString str{};
			str.AsciiString.resize(length);
			s.Read(str.AsciiString.data(), length);
			prop.Value = std::move(str);
		}
		break;
		case ValueType::UInt64: prop.Value = s.Read<UInt64>(); break;
		case ValueType::Vec4: prop.Value = s.Read<Vec4>(); break;
		case ValueType::UString:
		{
			const u16 byteCount = s.Read<u16>();
			UString str{};
			str.Utf8String.resize(byteCount);
			s.Read(str.Utf8String.data(), byteCount);
			prop.Value = std::move(str);
		}
		break;
		case ValueType::Bitfield:
		{
			Bitfield bitfield{};
			bitfield.Mask = s.Read<u32>();
			bitfield.Flags = s.Read<u32>();

			prop.Value = std::move(bitfield);
		}
		break;
		case ValueType::PolyPtr:
		{
			PolyPtr polyPtr{ nullptr };
			const u32 definitionHash = s.Read<u32>();
			if (definitionHash != 0)
			{
				polyPtr.Object = std::make_unique<Object>();
				polyPtr.Object->DefinitionHash = definitionHash;
				polyPtr.Object->IsCollection = false;
				ReadObject(*polyPtr.Object);
			}

			prop.Value = std::move(polyPtr);
		}
		break;
		case ValueType::Link:
		{
			const u16 id = s.Read<u16>();
			Link link{ nullptr };
			if (id != LinkStorage::InvalidId)
			{
				link.Storage = std::make_unique<LinkStorage>();
				link.Storage->Id = id;
				mLinksToResolve.emplace_back(link.Storage.get());
			}

			prop.Value = std::move(link);
		}
		break;
		case ValueType::Array:
		{
			Array arr;
			arr.ItemType = s.Read<ValueType>();
			const u16 itemCount = s.Read<u16>();
			arr.Items.reserve(itemCount);
			for (size i = 0; i < itemCount; i++)
			{
				arr.Items.emplace_back(std::move(ReadPropertyValue(0, arr.ItemType)));
			}

			prop.Value = std::move(arr);
		}
		break;
		case ValueType::Structure:
		{
			Structure struc{ std::make_unique<Object>() };
			struc.Object->DefinitionHash = s.Read<u32>();
			struc.Object->IsCollection = false;
			ReadObject(*struc.Object);
			prop.Value = std::move(struc);
		}
		break;
		default: Expects(false); break;
		}

		return prop;
	}

	void Reader::ResolveLinks()
	{
		Stream& s = mStream;

		std::vector<std::vector<u32>> linkNames{};
		const u16 linkNamesCount = s.Read<u16>();
		linkNames.reserve(linkNamesCount);
		for (size i = 0; i < linkNamesCount; i++)
		{
			std::vector<u32>& hashes = linkNames.emplace_back();
			const u8 hashesCount = s.Read<u8>();
			hashes.reserve(hashesCount);
			for (size j = 0; j < hashesCount; j++)
			{
				hashes.emplace_back(s.Read<u32>());
			}
		}

		for (LinkStorage* l : mLinksToResolve)
		{
			Ensures(l != nullptr);
			l->ScopedNameHashes = linkNames.at(l->Id);
		}
	}

	// TODO: get possible collections from attributes .xml files
	static bool DefIsCollection(u32 definitionHash)
	{
		static const std::unordered_set<u32> CollectionDefinitions{
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

#ifndef DOCTEST_CONFIG_DISABLE
TEST_SUITE("atb::Reader")
{
	using namespace noire;
	using namespace noire::atb;

	static void TraverseObject(const Object& o, size depth = 0)
	{
		std::cout << std::string(depth * 2, ' ') << o.Name << '\n';
		for (const Property& p : o.Properties)
		{
			std::cout << std::string((depth + 1) * 2, ' ') << "- "
					  << HashLookup::Instance(false).TryGetString(p.NameHash) << '\n';
		}

		if (o.IsCollection)
		{
			for (const Object& c : o.Objects)
			{
				TraverseObject(c, depth + 1);
			}
		}
	}

	TEST_CASE("Read" * doctest::skip(true))
	{
		FileStream fs{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\root.atb.pc" };
		Object root = Reader{ fs }.Read();

		TraverseObject(root);
	}
}
#endif
