#include "AttributeWindow.h"
#include "Hash.h"
#include <formats/AttributeFile.h>
#include <gsl/gsl>
#include <wx/log.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/props.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stc/stc.h>

// helper for std::visit
template<class... Ts>
struct overloaded : Ts...
{
	using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

static void AppendObjectToGrid(wxPropertyGrid* propGrid,
							   const noire::SAttributeObject& obj,
							   wxString parentQualifiedName,
							   wxPGProperty* parent,
							   wxPGProperty* inProp);

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
			[&name](const std::string& v) -> wxPGProperty* {
				return new wxStringProperty(name, wxPG_LABEL, v);
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
			[&name](const std::unique_ptr<noire::SAttributeObject>&) -> wxPGProperty* {
				return new wxStringProperty(name, wxPG_LABEL);
			},
			[&name](const std::vector<noire::SAttributeProperty>& v) -> wxPGProperty* {
				// TODO: show type of array
				return new wxStringProperty(
					name,
					wxPG_LABEL,
					wxString::Format("%zu %s", v.size(), v.size() == 1 ? "item" : "items"));
			} },
		p.Value);

	if (newProp)
	{
		newProp = propGrid->AppendIn(inProp, newProp);
	}

	if (p.Type == noire::EAttributePropertyType::Array)
	{
		const auto& items = std::get<std::vector<noire::SAttributeProperty>>(p.Value);
		for (std::size_t i = 0; i < items.size(); i++)
		{
			AppendPropertyToGrid(propGrid, items[i], wxString::Format("[%zu]", i), newProp);
		}
	}
	else if (p.Type == noire::EAttributePropertyType::Structure)
	{
		AppendObjectToGrid(propGrid,
						   *std::get<std::unique_ptr<noire::SAttributeObject>>(p.Value),
						   name,
						   nullptr,
						   newProp);
	}
}

static void AppendObjectToGrid(wxPropertyGrid* propGrid,
							   const noire::SAttributeObject& obj,
							   wxString parentQualifiedName,
							   wxPGProperty* parent,
							   wxPGProperty* inProp)
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

	if (obj.IsCollection)
	{
		wxPGProperty* objectsProp =
			propGrid->AppendIn(prop, new wxStringProperty("Objects", wxPG_LABEL));

		for (const noire::SAttributeObject& o : obj.Objects)
		{
			AppendObjectToGrid(propGrid, o, parentQualifiedName, objectsProp, nullptr);
		}
	}
}

CAttributeWindow::CAttributeWindow(wxWindow* parent,
								   wxWindowID id,
								   const wxString& title,
								   std::unique_ptr<noire::CAttributeFile> file)
	: wxFrame(parent, id, title), mFile{ std::move(file) }
{
	Expects(mFile != nullptr);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	wxPropertyGrid* propGrid = new wxPropertyGrid(this,
												  wxID_ANY,
												  wxDefaultPosition,
												  wxDefaultSize,
												  wxPG_SPLITTER_AUTO_CENTER | wxPG_DEFAULT_STYLE);
	mainSizer->Add(propGrid, 1, wxEXPAND);

	SetSizer(mainSizer);

	wxPGProperty* topProp = propGrid->Append(new wxPropertyCategory("Attributes"));
	AppendObjectToGrid(propGrid, mFile->Root(), "", nullptr, nullptr);

	propGrid->SetPropertyReadOnly(topProp, true);
	propGrid->CollapseAll();

	SetSize(800, 500);
}
