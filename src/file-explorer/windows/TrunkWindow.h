#pragma once
#include <memory>
#include <wx/frame.h>

namespace noire
{
	class Trunk;
}

class wxPropertyGrid;
class wxTreeCtrl;
class wxTreeEvent;
class wxScrolledWindow;

namespace noire::explorer
{
	class TrunkWindow : public wxFrame
	{
	public:
		TrunkWindow(wxWindow* parent,
					wxWindowID id,
					const wxString& title,
					std::shared_ptr<noire::Trunk> file);

	private:
		void OnSectionSelected(wxCommandEvent&);
		void OnRawExport(wxCommandEvent&);

		std::shared_ptr<noire::Trunk> mFile;
		wxScrolledWindow* mRight;
	};
}