#pragma once
#include <memory>
#include <wx/frame.h>

namespace noire
{
	class CAttributeFile;
}

class wxPropertyGrid;
class wxTreeCtrl;
class wxTreeEvent;

class CAttributeWindow : public wxFrame
{
public:
	CAttributeWindow(wxWindow* parent,
					 wxWindowID id,
					 const wxString& title,
					 std::unique_ptr<noire::CAttributeFile> file);

private:
	void OnObjectSelectionChanged(wxTreeEvent& event);

	std::unique_ptr<noire::CAttributeFile> mFile;
	wxTreeCtrl* mObjectsTree;
	wxPropertyGrid* mObjectPropertyGrid;
};