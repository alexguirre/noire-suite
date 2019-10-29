#include "ImagePanel.h"
#include <algorithm>
#include <gsl/gsl>
#include <wx/dcclient.h>

CImagePanel::CImagePanel(wxWindow* parent,
						 const wxWindowID id,
						 const wxPoint& pos,
						 const wxSize& size)
	: wxPanel(parent, id, pos, size), mImageOrig{}, mImageResized{}
{
	Bind(wxEVT_PAINT, &CImagePanel::OnPaint, this);
	Bind(wxEVT_SIZE, &CImagePanel::OnSize, this);
}

void CImagePanel::SetImage(const wxImage& img)
{
	mImageOrig = img.Copy();
	mImageResized = mImageOrig;
}

wxSize CImagePanel::GetImageIdealSize() const
{
	const wxSize thisSize = GetSize();
	const wxSize imgOrigSize = mImageOrig.GetSize();
	float wRatio = thisSize.GetWidth() / static_cast<float>(imgOrigSize.GetWidth());
	float hRatio = thisSize.GetHeight() / static_cast<float>(imgOrigSize.GetHeight());

	float ratio = std::min(wRatio, hRatio);

	return { std::max(1, static_cast<int>(imgOrigSize.GetWidth() * ratio)),
			 std::max(1, static_cast<int>(imgOrigSize.GetHeight() * ratio)) };
}

void CImagePanel::Render(wxDC& dc)
{
	if (!mImageResized.IsOk())
	{
		// return if no image was set yet
		return;
	}

	const wxSize idealSize = GetImageIdealSize();
	if (mImageResized.GetSize() != idealSize)
	{
		mImageResized =
			mImageOrig.Scale(idealSize.GetWidth(), idealSize.GetHeight(), wxIMAGE_QUALITY_HIGH);
	}

	const wxSize thisSize = GetSize();
	const wxSize imgSize = idealSize;
	const wxPoint pos{ thisSize.GetWidth() / 2 - imgSize.GetWidth() / 2,
					   thisSize.GetHeight() / 2 - imgSize.GetHeight() / 2 };
	dc.Clear();
	dc.DrawBitmap(mImageResized, pos);
}

void CImagePanel::OnPaint(wxPaintEvent&)
{
	wxPaintDC dc{ this };
	Render(dc);
}

void CImagePanel::OnSize(wxSizeEvent&)
{
	wxClientDC dc{ this };
	Render(dc);
}
