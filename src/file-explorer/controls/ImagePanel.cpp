#include "ImagePanel.h"
#include <algorithm>
#include <gsl/gsl>
#include <wx/dcclient.h>

namespace noire::explorer
{
	ImagePanel::ImagePanel(wxWindow* parent,
						   const wxWindowID id,
						   const wxPoint& pos,
						   const wxSize& size)
		: wxPanel(parent, id, pos, size, wxTAB_TRAVERSAL | wxBORDER_SIMPLE),
		  mImageOrig{},
		  mImageResized{},
		  mResizeQuality{ wxIMAGE_QUALITY_NORMAL },
		  mForceResize{ false }
	{
		Bind(wxEVT_PAINT, &ImagePanel::OnPaint, this);
		Bind(wxEVT_SIZE, &ImagePanel::OnSize, this);
	}

	void ImagePanel::SetImage(const wxImage& img)
	{
		mImageOrig = img.Copy();
		mImageResized = mImageOrig;
	}

	void ImagePanel::SetResizeQuality(wxImageResizeQuality quality)
	{
		if (mResizeQuality != quality)
		{
			mResizeQuality = quality;
			mForceResize = true;
			RenderNow();
		}
	}

	wxSize ImagePanel::GetImageIdealSize() const
	{
		const wxSize thisSize = GetSize();
		const wxSize imgOrigSize = mImageOrig.GetSize();
		float wRatio = thisSize.GetWidth() / static_cast<float>(imgOrigSize.GetWidth());
		float hRatio = thisSize.GetHeight() / static_cast<float>(imgOrigSize.GetHeight());

		float ratio = std::min(wRatio, hRatio);

		return { std::max(1, static_cast<int>(imgOrigSize.GetWidth() * ratio)),
				 std::max(1, static_cast<int>(imgOrigSize.GetHeight() * ratio)) };
	}

	void ImagePanel::Render(wxDC& dc)
	{
		if (!mImageResized.IsOk())
		{
			// return if no image was set yet
			return;
		}

		const wxSize idealSize = GetImageIdealSize();
		if (mImageResized.GetSize() != idealSize || mForceResize)
		{
			mImageResized =
				mImageOrig.Scale(idealSize.GetWidth(), idealSize.GetHeight(), mResizeQuality);
			mForceResize = false;
		}

		const wxSize thisSize = GetSize();
		const wxSize imgSize = idealSize;
		const wxPoint pos{ thisSize.GetWidth() / 2 - imgSize.GetWidth() / 2,
						   thisSize.GetHeight() / 2 - imgSize.GetHeight() / 2 };
		dc.Clear();
		dc.DrawBitmap(mImageResized, pos);
	}

	void ImagePanel::RenderNow()
	{
		wxClientDC dc{ this };
		Render(dc);
	}

	void ImagePanel::OnPaint(wxPaintEvent&)
	{
		wxPaintDC dc{ this };
		Render(dc);
	}

	void ImagePanel::OnSize(wxSizeEvent&) { RenderNow(); }
}
