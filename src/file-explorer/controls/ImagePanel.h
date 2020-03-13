#pragma once
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/panel.h>

class wxDC;

namespace noire::explorer
{
	class ImagePanel : public wxPanel
	{
	public:
		ImagePanel(wxWindow* parent, const wxWindowID id, const wxPoint& pos, const wxSize& size);

		const wxImage& GetImage() const { return mImageOrig; }
		void SetImage(const wxImage& img);

		wxImageResizeQuality GetResizeQuality() const { return mResizeQuality; }
		void SetResizeQuality(wxImageResizeQuality quality);

	private:
		wxSize GetImageIdealSize() const;
		void Render(wxDC& dc);
		void RenderNow();
		void OnPaint(wxPaintEvent& event);

		wxImage mImageOrig;
		wxBitmap mImageResized;
		wxImageResizeQuality mResizeQuality;
		bool mForceResize;
	};
}
