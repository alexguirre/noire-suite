#pragma once
#include <wx/frame.h>
#include <wx/image.h>
#include <controls/ImagePanel.h>

class CImageWindow : public wxFrame
{
public:
	CImageWindow(wxWindow* parent, wxWindowID id, const wxString& title, const wxImage& img);

private:
	CImagePanel* mImgPanel;
};