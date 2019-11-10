#include "ImageWindow.h"
#include <Identifiers.h>
#include <wx/combobox.h>
#include <wx/sizer.h>
#include <wx/toolbar.h>

static const wxArrayString ResizeQualityNames = []() {
	wxArrayString arr{};
	arr.resize(4);
	arr.at(wxIMAGE_QUALITY_NEAREST) = "Nearest";
	arr.at(wxIMAGE_QUALITY_BILINEAR) = "Bilinear";
	arr.at(wxIMAGE_QUALITY_BICUBIC) = "Bicubic";
	arr.at(wxIMAGE_QUALITY_BOX_AVERAGE) = "Box average";
	return arr;
}();

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
	mImgPanel->SetBackgroundColour(wxColour{ 255, 255, 255 });

	CreateToolBar();
}

wxToolBar* CImageWindow::OnCreateToolBar(long style, wxWindowID id, const wxString& name)
{
	wxToolBar* toolBar = wxFrame::OnCreateToolBar(style, id, name);

	wxComboBox* qualityCB = new wxComboBox(toolBar,
										   wxID_ANY,
										   wxEmptyString,
										   wxDefaultPosition,
										   wxDefaultSize,
										   ResizeQualityNames,
										   wxCB_READONLY);
	qualityCB->SetSelection(mImgPanel->GetResizeQuality());
	qualityCB->SetToolTip("Resize Quality");
	qualityCB->Bind(wxEVT_COMBOBOX, &CImageWindow::OnResizeQualityCombo, this);

	toolBar->AddControl(qualityCB);
	toolBar->Realize();

	return toolBar;
}

void CImageWindow::OnResizeQualityCombo(wxCommandEvent& event)
{
	mImgPanel->SetResizeQuality(static_cast<wxImageResizeQuality>(event.GetSelection()));
}