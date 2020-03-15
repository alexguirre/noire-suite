#pragma once
#include <core/Common.h>
#include <memory>
#include <wx/frame.h>

namespace noire
{
	class Trunk;
}

class wxPropertyGrid;
class wxTreeCtrl;
class wxTreeEvent;
class wxSplitterWindow;

namespace noire::explorer
{
	class TrunkWindow : public wxFrame
	{
	public:
		enum class Section : uptr
		{
			None = 0,
			Graphics = 1,
			UniqueTexture = 2,
		};

		TrunkWindow(wxWindow* parent,
					wxWindowID id,
					const wxString& title,
					std::shared_ptr<noire::Trunk> file);

	private:
		void OnSectionSelected(wxCommandEvent&);
		void OnRawExport(wxCommandEvent&);

		void ShowGraphics();
		void ShowUniqueTexture();

		std::shared_ptr<noire::Trunk> mFile;
		wxSplitterWindow* mCenter;
		Section mCurrentSection;
	};
}