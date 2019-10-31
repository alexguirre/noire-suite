#pragma once
#include <wx/toolbar.h>

class wxTextCtrl;

class CPathToolBar : public wxToolBar
{
public:
	CPathToolBar(wxWindow* parent,
				 wxWindowID id,
				 const wxPoint& pos = wxDefaultPosition,
				 const wxSize& size = wxDefaultSize,
				 long style = wxTB_DEFAULT_STYLE,
				 const wxString& name = wxToolBarNameStr);

private:
	wxTextCtrl* mPathText;
	bool mResizeQueuedRealize;

	void OnWindowResize(wxSizeEvent& e);
};