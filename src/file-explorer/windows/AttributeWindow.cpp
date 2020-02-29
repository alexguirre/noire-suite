#include "AttributeWindow.h"
#include "Hash.h"
#include <core/files/AttributeFile.h>
#include <core/files/atb/Types.h>
#include <gsl/gsl>
#include <wx/log.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/props.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>

namespace noire::explorer
{
	// helper for std::visit
	template<class... Ts>
	struct overloaded : Ts...
	{
		using Ts::operator()...;
	};
	template<class... Ts>
	overloaded(Ts...)->overloaded<Ts...>;

	static wxPGProperty* AppendObjectToGrid(wxPropertyGrid* propGrid,
											const atb::Object& obj,
											wxPGProperty* parent,
											wxPGProperty* inProp,
											wxPGProperty** outPropertiesProp);

	static void AppendPropertyToGrid(wxPropertyGrid* propGrid,
									 const atb::Property& p,
									 const wxString& nameOverride,
									 wxPGProperty* inProp)
	{
		wxString name = !nameOverride.IsEmpty() ?
							nameOverride :
							wxString{ HashLookup::Instance(false).TryGetString(p.NameHash) };
		wxPGProperty* newProp = std::visit(
			overloaded{
				[&name](auto) -> wxPGProperty* {
					return new wxStringProperty(name, wxPG_LABEL, "TBD");
				},
				[&name](atb::Int32 v) -> wxPGProperty* {
					return new wxIntProperty(name, wxPG_LABEL, v);
				},
				[&name](atb::UInt32 v) -> wxPGProperty* {
					return new wxUIntProperty(name, wxPG_LABEL, v);
				},
				[&name](atb::Float v) -> wxPGProperty* {
					return new wxFloatProperty(name, wxPG_LABEL, v);
				},
				[&name](atb::Bool v) -> wxPGProperty* {
					return new wxBoolProperty(name, wxPG_LABEL, v);
				},
				[&name](const atb::Vec3& v) -> wxPGProperty* {
					return new wxStringProperty(
						name,
						wxPG_LABEL,
						wxString::Format("X:%.4f, Y:%.4f, Z:%.4f", v[0], v[1], v[2]));
				},
				[&name](const atb::Vec2& v) -> wxPGProperty* {
					return new wxStringProperty(name,
												wxPG_LABEL,
												wxString::Format("X:%.4f, Y:%.4f", v[0], v[1]));
				},
				[&name](const atb::Mat4&) -> wxPGProperty* {
					return new wxStringProperty(name, wxPG_LABEL, "Mat4 - TBD");
				},
				[&name](const atb::AString& v) -> wxPGProperty* {
					return new wxStringProperty(name, wxPG_LABEL, v.AsciiString);
				},
				[&name](atb::UInt64 v) -> wxPGProperty* {
					return new wxUIntProperty(name, wxPG_LABEL, v);
				},
				[&name](const atb::Vec4& v) -> wxPGProperty* {
					return new wxStringProperty(
						name,
						wxPG_LABEL,
						wxString::Format("X:%.4f, Y:%.4f, Z:%.4f, W:%.4f", v[0], v[1], v[2], v[3]));
				},
				[&name](const atb::UString& v) -> wxPGProperty* {
					return new wxStringProperty(name, wxPG_LABEL, wxString::FromUTF8(v.Utf8String));
				},
				[&name](const atb::Bitfield&) -> wxPGProperty* {
					return new wxStringProperty(name, wxPG_LABEL, "<composed>");
				},
				[&name](const atb::PolyPtr& v) -> wxPGProperty* {
					return new wxStringProperty(name, wxPG_LABEL, v.Object ? "" : "null");
				},
				[&name](const atb::Link& v) -> wxPGProperty* {
					return new wxStringProperty(name, wxPG_LABEL, v.ScopedName());
				},
				[&name](const atb::Structure&) -> wxPGProperty* {
					return new wxStringProperty(name, wxPG_LABEL);
				},
				[&name](const atb::Array& v) -> wxPGProperty* {
					const std::string_view type = atb::ValueTypeToString(v.ItemType);
					return new wxStringProperty(
						name,
						wxPG_LABEL,
						wxString::Format("%zu %s (type: %.*s)",
										 v.Items.size(),
										 v.Items.size() == 1 ? "item" : "items",
										 type.size(),
										 type.data()));
				} },
			p.Value);

		if (newProp)
		{
			newProp = propGrid->AppendIn(inProp, newProp);
		}

		switch (p.Type)
		{
		case atb::ValueType::Bitfield:
		{
			const atb::Bitfield& bitfield = std::get<atb::Bitfield>(p.Value);

			propGrid->AppendIn(newProp,
							   new wxStringProperty("Mask",
													wxPG_LABEL,
													wxString::Format("0x%08X", bitfield.Mask)));
			propGrid->AppendIn(newProp,
							   new wxStringProperty("Flags",
													wxPG_LABEL,
													wxString::Format("0x%08X", bitfield.Flags)));
		}
		break;
		case atb::ValueType::PolyPtr:
		{
			const atb::PolyPtr& polyPtr = std::get<atb::PolyPtr>(p.Value);
			if (polyPtr.Object)
			{
				AppendObjectToGrid(propGrid, *polyPtr.Object, nullptr, newProp, nullptr);
			}
		}
		break;
		case atb::ValueType::Array:
		{
			const auto& arr = std::get<atb::Array>(p.Value);
			for (std::size_t i = 0; i < arr.Items.size(); i++)
			{
				AppendPropertyToGrid(propGrid, arr.Items[i], wxString::Format("[%zu]", i), newProp);
			}
		}
		break;
		case atb::ValueType::Structure:
		{
			AppendObjectToGrid(propGrid,
							   *std::get<atb::Structure>(p.Value).Object,
							   nullptr,
							   newProp,
							   nullptr);
		}
		break;
		}
	}

	static wxPGProperty* AppendObjectToGrid(wxPropertyGrid* propGrid,
											const atb::Object& obj,
											wxPGProperty* parent,
											wxPGProperty* inProp,
											wxPGProperty** outPropertiesProp)
	{
		wxPGProperty* prop =
			inProp ?
				inProp :
				parent ? propGrid->AppendIn(parent, new wxStringProperty(obj.Name, wxPG_LABEL)) :
						 propGrid->Append(new wxStringProperty(obj.Name, wxPG_LABEL));

		if (!obj.Name.empty())
		{
			propGrid->AppendIn(prop, new wxStringProperty("Name", wxPG_LABEL, obj.Name));
		}
		propGrid->AppendIn(
			prop,
			new wxStringProperty("Definition",
								 wxPG_LABEL,
								 HashLookup::Instance(false).TryGetString(obj.DefinitionHash)));
		wxPGProperty* propertiesProp =
			propGrid->AppendIn(prop, new wxStringProperty("Properties", wxPG_LABEL));
		for (const atb::Property& p : obj.Properties)
		{
			AppendPropertyToGrid(propGrid, p, "", propertiesProp);
		}

		if (outPropertiesProp)
		{
			*outPropertiesProp = propertiesProp;
		}
		return prop;
	}

	static void FillObjectPropertyGrid(wxPropertyGrid* propGrid, const atb::Object& obj)
	{
		propGrid->Clear();
		wxPGProperty* p2;
		wxPGProperty* p = AppendObjectToGrid(propGrid, obj, nullptr, nullptr, &p2);
		propGrid->SetPropertyReadOnly(propGrid->GetRoot(), true);
		propGrid->CollapseAll();
		// only expand root and properties
		propGrid->Expand(p);
		propGrid->Expand(p2);
	}

	class ObjectTreeItemData : public wxTreeItemData
	{
	public:
		ObjectTreeItemData(const atb::Object& obj) : mObject{ obj } {}

		const atb::Object& Object() const { return mObject; }

	private:
		const atb::Object& mObject;
	};

	static void AppendObjectToTree(wxTreeCtrl* tree, wxTreeItemId parent, const atb::Object& obj)
	{
		wxTreeItemId objItem =
			tree->AppendItem(parent, obj.Name, -1, -1, new ObjectTreeItemData{ obj });

		if (obj.IsCollection)
		{
			for (const atb::Object& o : obj.Objects)
			{
				AppendObjectToTree(tree, objItem, o);
			}
		}
	}

	static void FillObjectsTree(wxTreeCtrl* tree, const atb::Object& obj)
	{
		wxTreeItemId root = tree->AddRoot("ROOT");
		AppendObjectToTree(tree, root, obj);
	}

	AttributeWindow::AttributeWindow(wxWindow* parent,
									 wxWindowID id,
									 const wxString& title,
									 std::shared_ptr<AttributeFile> file)
		: wxFrame(parent, id, title),
		  mFile{ file },
		  mObjectsTree{ nullptr },
		  mObjectPropertyGrid{ nullptr }
	{
		Expects(mFile != nullptr);

		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
		wxSplitterWindow* splitter = new wxSplitterWindow(this,
														  wxID_ANY,
														  wxDefaultPosition,
														  wxDefaultSize,
														  wxSP_LIVE_UPDATE);
		splitter->SetSashGravity(0.0);
		splitter->SetMinimumPaneSize(50);
		mainSizer->Add(splitter, 1, wxEXPAND);

		mObjectsTree = new wxTreeCtrl(splitter,
									  wxID_ANY,
									  wxDefaultPosition,
									  wxDefaultSize,
									  wxTR_HAS_BUTTONS | wxTR_TWIST_BUTTONS | wxTR_NO_LINES |
										  wxTR_HIDE_ROOT | wxTR_LINES_AT_ROOT | wxTR_SINGLE);

		mObjectPropertyGrid = new wxPropertyGrid(splitter,
												 wxID_ANY,
												 wxDefaultPosition,
												 wxDefaultSize,
												 wxPG_SPLITTER_AUTO_CENTER | wxPG_DEFAULT_STYLE);

		splitter->SplitVertically(mObjectsTree, mObjectPropertyGrid);

		SetSizer(mainSizer);

		if (!mFile->IsLoaded())
		{
			// TODO: show loading cursor
			mFile->Load();
		}

		FillObjectsTree(mObjectsTree, mFile->Root());

		SetSize(800, 500);

		mObjectsTree->Bind(wxEVT_TREE_SEL_CHANGED,
						   &AttributeWindow::OnObjectSelectionChanged,
						   this);
	}

	void AttributeWindow::OnObjectSelectionChanged(wxTreeEvent& event)
	{
		wxTreeItemId itemId = event.GetItem();
		Expects(itemId.IsOk());

		const ObjectTreeItemData* data =
			reinterpret_cast<ObjectTreeItemData*>(mObjectsTree->GetItemData(itemId));

		FillObjectPropertyGrid(mObjectPropertyGrid, data->Object());

		event.Skip();
	}
}
