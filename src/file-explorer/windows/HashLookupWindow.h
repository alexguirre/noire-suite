#pragma once
#include <wx/frame.h>

class wxTextCtrl;
class wxCommandEvent;

class CHashLookupWindow : public wxFrame
{
public:
	CHashLookupWindow(wxWindow* parent);

private:
	void OnLookup(wxCommandEvent&);

	wxTextCtrl* mInputTextCtrl;
	wxTextCtrl* mOutputTextCtrl;
};