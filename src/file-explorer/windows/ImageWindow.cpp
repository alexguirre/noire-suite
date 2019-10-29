#include "ImageWindow.h"
#include <wx/sizer.h>

CImageWindow::CImageWindow(wxWindow* parent,
						   wxWindowID id,
						   const wxString& title,
						   const wxImage& img)
	: wxFrame(parent, id, title),
	  mImgPanel{ new CImagePanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize) }
{
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(mImgPanel, 1, wxEXPAND);	
	SetSizer(sizer);

	mImgPanel->SetImage(img);
}