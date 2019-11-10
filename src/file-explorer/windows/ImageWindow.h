#pragma once
#include <controls/ImagePanel.h>
#include <wx/frame.h>
#include <wx/image.h>

class CImageWindow : public wxFrame
{
public:
	CImageWindow(wxWindow* parent, wxWindowID id, const wxString& title, const wxImage& img);

	wxToolBar* OnCreateToolBar(long style, wxWindowID id, const wxString& name) override;

private:
	void OnResizeQualityCombo(wxCommandEvent& event);

	CImagePanel* mImgPanel;
};