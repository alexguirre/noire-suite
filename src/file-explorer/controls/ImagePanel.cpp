#include "ImagePanel.h"
#include <algorithm>
#include <core/Common.h>
#include <gsl/gsl>
#include <wx/dcbuffer.h>
#include <wx/dcclient.h>

namespace noire::explorer
{
	ImagePanel::ImagePanel(wxWindow* parent,
						   const wxWindowID id,
						   const wxPoint& pos,
						   const wxSize& size)
		: wxPanel(parent,
				  id,
				  pos,
				  size,
				  wxTAB_TRAVERSAL | wxBORDER_SIMPLE | wxFULL_REPAINT_ON_RESIZE),
		  mImageOrig{},
		  mImageResized{},
		  mResizeQuality{ wxIMAGE_QUALITY_NORMAL },
		  mForceResize{ false }
	{
		SetDoubleBuffered(true);
		SetBackgroundStyle(wxBG_STYLE_PAINT);

		Bind(wxEVT_PAINT, &ImagePanel::OnPaint, this);
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

	static void DrawBackground(wxDC& dc)
	{
		const auto color1 = std::tie(*wxLIGHT_GREY_BRUSH, *wxLIGHT_GREY_PEN);
		const auto color2 = std::tie(*wxMEDIUM_GREY_BRUSH, *wxMEDIUM_GREY_PEN);

		constexpr size SquareSize{ 10 };

		const wxSize canvasSize = dc.GetSize();
		const size wCount = canvasSize.GetWidth() / SquareSize;
		const size hCount = canvasSize.GetHeight() / SquareSize;

		for (size y = 0; y <= hCount; y++)
		{
			for (size x = 0; x <= wCount; x++)
			{
				const auto [b, p] = (x + y) % 2 == 0 ? color1 : color2;
				dc.SetBrush(b);
				dc.SetPen(p);
				dc.DrawRectangle(x * SquareSize, y * SquareSize, SquareSize, SquareSize);
			}
		}
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
		DrawBackground(dc);
		dc.DrawBitmap(mImageResized, pos);
	}

	void ImagePanel::RenderNow()
	{
		wxClientDC dcClient{ this };
		wxBufferedDC dc{ &dcClient };
		Render(dc);
	}

	void ImagePanel::OnPaint(wxPaintEvent&)
	{
		wxAutoBufferedPaintDC dc{ this };
		Render(dc);
	}
}
