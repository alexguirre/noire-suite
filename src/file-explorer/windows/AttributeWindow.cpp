#include "AttributeWindow.h"
#include "Hash.h"
#include <formats/AttributeFile.h>
#include <gsl/gsl>
#include <wx/log.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/props.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>

// helper for std::visit
template<class... Ts>
struct overloaded : Ts...
{
	using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

static wxPGProperty* AppendObjectToGrid(wxPropertyGrid* propGrid,
										const noire::SAttributeObject& obj,
										wxString parentQualifiedName,
										wxPGProperty* parent,
										wxPGProperty* inProp,
										wxPGProperty** outPropertiesProp);

static void AppendPropertyToGrid(wxPropertyGrid* propGrid,
								 const noire::SAttributeProperty& p,
								 const wxString& nameOverride,
								 wxPGProperty* inProp)
{
	wxString name = !nameOverride.IsEmpty() ?
						nameOverride :
						wxString{ noire::CHashDatabase::Instance(false).GetString(p.NameHash) };
	wxPGProperty* newProp = std::visit(
		overloaded{
			[&name](auto) -> wxPGProperty* {
				return new wxStringProperty(name, wxPG_LABEL, "TBD");
			},
			[&name](std::int32_t v) -> wxPGProperty* {
				return new wxIntProperty(name, wxPG_LABEL, v);
			},
			[&name](std::uint32_t v) -> wxPGProperty* {
				return new wxUIntProperty(name, wxPG_LABEL, v);
			},
			[&name](float v) -> wxPGProperty* { return new wxFloatProperty(name, wxPG_LABEL, v); },
			[&name](bool v) -> wxPGProperty* { return new wxBoolProperty(name, wxPG_LABEL, v); },
			[&name](const noire::SAttributeProperty::Vec3& v) -> wxPGProperty* {
				return new wxStringProperty(
					name,
					wxPG_LABEL,
					wxString::Format("X:%.4f, Y:%.4f, Z:%.4f", v[0], v[1], v[2]));
			},
			[&name](const noire::SAttributeProperty::Vec2& v) -> wxPGProperty* {
				return new wxStringProperty(name,
											wxPG_LABEL,
											wxString::Format("X:%.4f, Y:%.4f", v[0], v[1]));
			},
			[&name](const noire::SAttributeProperty::Mat4&) -> wxPGProperty* {
				return new wxStringProperty(name, wxPG_LABEL, "Mat4 - TBD");
			},
			[&name](const noire::SAttributeProperty::AString& v) -> wxPGProperty* {
				return new wxStringProperty(name, wxPG_LABEL, v.AsciiString);
			},
			[&name](std::uint64_t v) -> wxPGProperty* {
				return new wxUIntProperty(name, wxPG_LABEL, v);
			},
			[&name](const noire::SAttributeProperty::Vec4& v) -> wxPGProperty* {
				return new wxStringProperty(
					name,
					wxPG_LABEL,
					wxString::Format("X:%.4f, Y:%.4f, Z:%.4f, W:%.4f", v[0], v[1], v[2], v[3]));
			},
			[&name](const noire::SAttributeProperty::UString& v) -> wxPGProperty* {
				return new wxStringProperty(name, wxPG_LABEL, wxString::FromUTF8(v.Utf8String));
			},
			[&name](const noire::SAttributeProperty::Bitfield&) -> wxPGProperty* {
				return new wxStringProperty(name, wxPG_LABEL, "<composed>");
			},
			[&name](const noire::SAttributeProperty::PolyPtr& v) -> wxPGProperty* {
				return new wxStringProperty(name, wxPG_LABEL, v.Object ? "" : "null");
			},
			[&name](const noire::SAttributeProperty::Link& v) -> wxPGProperty* {
				return new wxStringProperty(name, wxPG_LABEL, v.ScopedName());
			},
			[&name](const noire::SAttributeProperty::Structure&) -> wxPGProperty* {
				return new wxStringProperty(name, wxPG_LABEL);
			},
			[&name](const noire::SAttributeProperty::Array& v) -> wxPGProperty* {
				std::string_view type = noire::ToString(v.ItemType);
				return new wxStringProperty(name,
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
	case noire::EAttributePropertyType::Bitfield:
	{
		const noire::SAttributeProperty::Bitfield& bitfield =
			std::get<noire::SAttributeProperty::Bitfield>(p.Value);

		propGrid->AppendIn(
			newProp,
			new wxStringProperty("Mask", wxPG_LABEL, wxString::Format("0x%08X", bitfield.Mask)));
		propGrid->AppendIn(
			newProp,
			new wxStringProperty("Flags", wxPG_LABEL, wxString::Format("0x%08X", bitfield.Flags)));
	}
	break;
	case noire::EAttributePropertyType::PolyPtr:
	{
		const noire::SAttributeProperty::PolyPtr& polyPtr =
			std::get<noire::SAttributeProperty::PolyPtr>(p.Value);
		if (polyPtr.Object)
		{
			AppendObjectToGrid(propGrid, *polyPtr.Object, name, nullptr, newProp, nullptr);
		}
	}
	break;
	case noire::EAttributePropertyType::Array:
	{
		const auto& arr = std::get<noire::SAttributeProperty::Array>(p.Value);
		for (std::size_t i = 0; i < arr.Items.size(); i++)
		{
			AppendPropertyToGrid(propGrid, arr.Items[i], wxString::Format("[%zu]", i), newProp);
		}
	}
	break;
	case noire::EAttributePropertyType::Structure:
	{
		AppendObjectToGrid(propGrid,
						   *std::get<noire::SAttributeProperty::Structure>(p.Value).Object,
						   name,
						   nullptr,
						   newProp,
						   nullptr);
	}
	break;
	}
}

static wxPGProperty* AppendObjectToGrid(wxPropertyGrid* propGrid,
										const noire::SAttributeObject& obj,
										wxString parentQualifiedName,
										wxPGProperty* parent,
										wxPGProperty* inProp,
										wxPGProperty** outPropertiesProp)
{
	if (!parentQualifiedName.IsEmpty())
	{
		parentQualifiedName += '.';
	}
	parentQualifiedName += obj.Name;

	wxPGProperty* prop =
		inProp ?
			inProp :
			parent ?
			propGrid->AppendIn(parent,
							   new wxStringProperty(obj.Name, wxPG_LABEL, parentQualifiedName)) :
			propGrid->Append(new wxStringProperty(obj.Name, wxPG_LABEL, parentQualifiedName));

	if (!obj.Name.empty())
	{
		propGrid->AppendIn(prop, new wxStringProperty("Name", wxPG_LABEL, obj.Name));
	}
	propGrid->AppendIn(
		prop,
		new wxStringProperty("Definition",
							 wxPG_LABEL,
							 noire::CHashDatabase::Instance(false).GetString(obj.DefinitionHash)));
	wxPGProperty* propertiesProp =
		propGrid->AppendIn(prop, new wxStringProperty("Properties", wxPG_LABEL));
	for (const noire::SAttributeProperty& p : obj.Properties)
	{
		AppendPropertyToGrid(propGrid, p, "", propertiesProp);
	}

	if (outPropertiesProp)
	{
		*outPropertiesProp = propertiesProp;
	}
	return prop;
}

static void FillObjectPropertyGrid(wxPropertyGrid* propGrid, const noire::SAttributeObject& obj)
{
	propGrid->Clear();
	wxPGProperty* p2;
	wxPGProperty* p = AppendObjectToGrid(propGrid, obj, "", nullptr, nullptr, &p2);
	propGrid->SetPropertyReadOnly(propGrid->GetRoot(), true);
	propGrid->CollapseAll();
	// only expand root and properties
	propGrid->Expand(p);
	propGrid->Expand(p2);
}

class CObjectTreeItemData : public wxTreeItemData
{
public:
	CObjectTreeItemData(const noire::SAttributeObject& obj) : mObject{ obj } {}

	const noire::SAttributeObject& Object() const { return mObject; }

private:
	const noire::SAttributeObject& mObject;
};

static void
AppendObjectToTree(wxTreeCtrl* tree, wxTreeItemId parent, const noire::SAttributeObject& obj)
{
	wxTreeItemId objItem =
		tree->AppendItem(parent, obj.Name, -1, -1, new CObjectTreeItemData{ obj });

	if (obj.IsCollection)
	{
		for (const noire::SAttributeObject& o : obj.Objects)
		{
			AppendObjectToTree(tree, objItem, o);
		}
	}
}

static void FillObjectsTree(wxTreeCtrl* tree, const noire::SAttributeObject& obj)
{
	wxTreeItemId root = tree->AddRoot("ROOT");
	AppendObjectToTree(tree, root, obj);
}

CAttributeWindow::CAttributeWindow(wxWindow* parent,
								   wxWindowID id,
								   const wxString& title,
								   std::unique_ptr<noire::CAttributeFile> file)
	: wxFrame(parent, id, title),
	  mFile{ std::move(file) },
	  mObjectsTree{ nullptr },
	  mObjectPropertyGrid{ nullptr }
{
	Expects(mFile != nullptr);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	wxSplitterWindow* splitter =
		new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
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

	FillObjectsTree(mObjectsTree, mFile->Root());

	SetSize(800, 500);

	mObjectsTree->Bind(wxEVT_TREE_SEL_CHANGED, &CAttributeWindow::OnObjectSelectionChanged, this);
}

void CAttributeWindow::OnObjectSelectionChanged(wxTreeEvent& event)
{
	wxTreeItemId itemId = event.GetItem();
	Expects(itemId.IsOk());

	const CObjectTreeItemData* data =
		reinterpret_cast<CObjectTreeItemData*>(mObjectsTree->GetItemData(itemId));

	FillObjectPropertyGrid(mObjectPropertyGrid, data->Object());

	event.Skip();
}
