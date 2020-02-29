#pragma once
#include <memory>
#include <wx/frame.h>

namespace noire
{
	class AttributeFile;
}

class wxPropertyGrid;
class wxTreeCtrl;
class wxTreeEvent;

namespace noire::explorer
{
	class AttributeWindow : public wxFrame
	{
	public:
		AttributeWindow(wxWindow* parent,
						wxWindowID id,
						const wxString& title,
						std::shared_ptr<noire::AttributeFile> file);

	private:
		void OnObjectSelectionChanged(wxTreeEvent& event);

		std::shared_ptr<noire::AttributeFile> mFile;
		wxTreeCtrl* mObjectsTree;
		wxPropertyGrid* mObjectPropertyGrid;
	};
}
