#include "HashLookupWindow.h"
#include <wx/panel.h>
#include <wx/sizer.h>

CHashLookupWindow::CHashLookupWindow(wxWindow* parent)
	: wxFrame(parent, wxID_ANY, "noire-suite - Hash Lookup")
{
	wxPanel* mainPanel = new wxPanel(this);

	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(mainPanel, 1, wxEXPAND);
	SetSizer(sizer);

	mainPanel->SetBackgroundColour(*wxWHITE);

	SetSize(550, 650);
}
