#include "PathToolBar.h"
#include <wx/artprov.h>
#include <wx/textctrl.h>

CPathToolBar::CPathToolBar(wxWindow* parent,
						   wxWindowID id,
						   const wxPoint& pos,
						   const wxSize& size,
						   long style,
						   const wxString& name)
	: wxToolBar(parent, id, pos, size, style, name), mResizeQueuedRealize{ false }
{
	mPathText = new wxTextCtrl(this, wxID_ANY);
	mPathText->SetEditable(false);

	AddTool(wxID_BACKWARD,
			wxEmptyString,
			wxArtProvider::GetBitmap(wxART_GO_BACK),
			wxEmptyString,
			wxITEM_NORMAL);
	AddTool(wxID_FORWARD,
			wxEmptyString,
			wxArtProvider::GetBitmap(wxART_GO_FORWARD),
			wxEmptyString,
			wxITEM_NORMAL);
	AddTool(wxID_UP,
			wxEmptyString,
			wxArtProvider::GetBitmap(wxART_GO_UP),
			wxEmptyString,
			wxITEM_NORMAL);
	AddControl(mPathText);
	AddSeparator();

	Realize();

	parent->Bind(wxEVT_SIZE, &CPathToolBar::OnWindowResize, this);

	// just some dummy path for now
	mPathText->AppendText("/some/dummy/path");
}

// handle resizing of mPathText, wxToolBar doesn't seem to support any kind of sizer so we do it
// manually
void CPathToolBar::OnWindowResize(wxSizeEvent& e)
{
	constexpr std::size_t RightSideRequiredSpace{ 100 };

	const int newSize =
		e.GetSize().GetWidth() - mPathText->GetPosition().x - RightSideRequiredSpace;
	mPathText->SetSize(newSize, -1);
	if (!mResizeQueuedRealize)
	{
		CallAfter([this]() {
			Realize();
			mResizeQueuedRealize = false;
		});
		mResizeQueuedRealize = true;
	}
	e.Skip();
}