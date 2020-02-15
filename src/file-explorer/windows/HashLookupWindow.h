#pragma once
#include <wx/frame.h>

class wxTextCtrl;
class wxCommandEvent;
class wxKeyEvent;

namespace noire::explorer
{
	class HashLookupWindow : public wxFrame
	{
	public:
		HashLookupWindow(wxWindow* parent);

	private:
		void OnLookup(wxCommandEvent&);
		void OnInputChar(wxKeyEvent&);

		wxTextCtrl* mInputTextCtrl;
		wxTextCtrl* mOutputTextCtrl;
	};
}
