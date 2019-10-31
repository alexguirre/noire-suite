#include "PathToolBar.h"
#include "DirectoryContentsListCtrl.h"
#include <array>
#include <formats/WADFile.h>
#include <wx/artprov.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

CPathToolBar::CPathToolBar(wxWindow* parent,
						   wxWindowID id,
						   const wxPoint& pos,
						   const wxSize& size,
						   long style,
						   const wxString& name)
	: wxToolBar(parent, id, pos, size, style, name),
	  mResizeQueuedRealize{ false },
	  mDirHistory{},
	  mDirContentsListCtrl{ nullptr }
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
	parent->Bind(EVT_DIRECTORY_CHANGED, &CPathToolBar::OnDirectoryChanged, this);
	parent->Bind(wxEVT_TOOL, &CPathToolBar::OnTool, this);
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

void CPathToolBar::OnDirectoryChanged(CDirectoryEvent& e)
{
	wxLogDebug("OnDirectoryChanged");

	if (!mDirHistory.HasCurrent() || &mDirHistory.Current() != &e.GetDirectory())
	{
		mDirHistory.Push(e.GetDirectory());
	}

	// TODO: show full path in toolbar
	mPathText->SetValue(e.GetDirectory().Name());

	FindById(wxID_BACKWARD)->Enable(mDirHistory.CanGoBack());
	FindById(wxID_FORWARD)->Enable(mDirHistory.CanGoForward());
	FindById(wxID_UP)->Enable(false);

	Realize();

	e.Skip();
}

void CPathToolBar::OnTool(wxCommandEvent& event)
{
	wxLogDebug("OnTool");

	if (CanHandleCommand(event.GetId()))
	{
		HandleCommand(event.GetId());
	}

	event.Skip();
}

bool CPathToolBar::CanHandleCommand(int id) const
{
	return (id == wxID_BACKWARD && mDirHistory.CanGoBack()) ||
		   (id == wxID_FORWARD && mDirHistory.CanGoForward()) || id == wxID_UP;
}

void CPathToolBar::HandleCommand(int id)
{
	Expects(CanHandleCommand(id));

	switch (id)
	{
	case wxID_BACKWARD:
	{
		wxLogDebug("> Backward");
		mDirHistory.GoBack();
		mDirContentsListCtrl->SetDirectory(mDirHistory.Current());
	}
	break;
	case wxID_FORWARD:
	{
		wxLogDebug("> Forward");
		mDirHistory.GoForward();
		mDirContentsListCtrl->SetDirectory(mDirHistory.Current());
	}
	break;
	case wxID_UP:
	{
		wxLogDebug("> Up");
		// TODO: implement CDirectoryHistory::GoUp
		// mDirHistory.GoUp();
	}
	break;
	}
}