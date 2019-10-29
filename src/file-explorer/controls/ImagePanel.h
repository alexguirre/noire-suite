#pragma once
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/panel.h>

class wxDC;

class CImagePanel : public wxPanel
{
public:
	CImagePanel(wxWindow* parent, const wxWindowID id, const wxPoint& pos, const wxSize& size);

	const wxImage& GetImage() const { return mImageOrig; }
	void SetImage(const wxImage& img);

private:
	wxSize GetImageIdealSize() const;
	void Render(wxDC& dc);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);

	wxImage mImageOrig;
	wxBitmap mImageResized;
};