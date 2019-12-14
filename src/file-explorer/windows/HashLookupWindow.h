#pragma once
#include <wx/frame.h>

class wxTextCtrl;
class wxCommandEvent;
class wxKeyEvent;

class CHashLookupWindow : public wxFrame
{
public:
	CHashLookupWindow(wxWindow* parent);

private:
	void OnLookup(wxCommandEvent&);
	void OnInputChar(wxKeyEvent&);

	wxTextCtrl* mInputTextCtrl;
	wxTextCtrl* mOutputTextCtrl;
};