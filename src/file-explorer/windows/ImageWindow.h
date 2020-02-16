#pragma once
#include <controls/ImagePanel.h>
#include <wx/frame.h>
#include <wx/image.h>

namespace noire::explorer
{
	class ImageWindow : public wxFrame
	{
	public:
		ImageWindow(wxWindow* parent, wxWindowID id, const wxString& title, const wxImage& img);

		wxToolBar* OnCreateToolBar(long style, wxWindowID id, const wxString& name) override;

	private:
		void OnResizeQualityCombo(wxCommandEvent& event);

		ImagePanel* mImgPanel;
	};
}
