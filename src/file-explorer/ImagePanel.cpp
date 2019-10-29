#include "ImagePanel.h"
#include <wx/dcclient.h>

CImagePanel::CImagePanel(wxWindow* parent,
						 const wxWindowID id,
						 const wxPoint& pos,
						 const wxSize& size)
	: wxPanel(parent, id, pos, size), mImage{}
{
	Bind(wxEVT_PAINT, &CImagePanel::OnPaint, this);
}

void CImagePanel::SetImage(const wxBitmap& bmp)
{
	mImage = bmp.GetSubBitmap({ 0, 0, bmp.GetWidth(), bmp.GetHeight() });
}

void CImagePanel::OnPaint(wxPaintEvent&)
{
	if (!mImage.IsOk())
	{
		// return if no image was set yet
		return;
	}

	wxPaintDC dc{ this };
	dc.DrawBitmap(mImage, 0, 0);
}
