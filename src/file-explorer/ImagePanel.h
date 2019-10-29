#pragma once
#include <wx/bitmap.h>
#include <wx/panel.h>

class CImagePanel : public wxPanel
{
public:
	CImagePanel(wxWindow* parent, const wxWindowID id, const wxPoint& pos, const wxSize& size);

	const wxBitmap& GetImage() const { return mImage; }
	void SetImage(const wxBitmap& bmp);

private:
	void OnPaint(wxPaintEvent& event);

	wxBitmap mImage;
};