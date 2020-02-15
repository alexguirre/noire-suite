#include "PathToolBar.h"
#include "DirectoryContentsListCtrl.h"
#include <array>
#include <formats/WADFile.h>
#include <wx/artprov.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

namespace noire::explorer
{
	PathToolBar::PathToolBar(wxWindow* parent,
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

		EnableTool(wxID_BACKWARD, false);
		EnableTool(wxID_FORWARD, false);
		EnableTool(wxID_UP, false);

		parent->Bind(wxEVT_SIZE, &PathToolBar::OnWindowResize, this);
		parent->Bind(EVT_DIRECTORY_CHANGED, &PathToolBar::OnDirectoryChanged, this);
		parent->Bind(wxEVT_TOOL, &PathToolBar::OnTool, this);
	}

	// handle resizing of mPathText, wxToolBar doesn't seem to support any kind of sizer so we do it
	// manually
	void PathToolBar::OnWindowResize(wxSizeEvent& e)
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

	void PathToolBar::OnDirectoryChanged(DirectoryEvent& e)
	{
		wxLogDebug("OnDirectoryChanged");

		PathView dir = e.GetDirectory();
		if (!mDirHistory.HasCurrent() || mDirHistory.Current() != dir)
		{
			mDirHistory.Push(dir);
		}

		mPathText->SetValue({ dir.String().data(), dir.String().size() });

		EnableTool(wxID_BACKWARD, mDirHistory.CanGoBack());
		EnableTool(wxID_FORWARD, mDirHistory.CanGoForward());
		EnableTool(wxID_UP, mDirHistory.CanGoUp());

		e.Skip();
	}

	void PathToolBar::OnTool(wxCommandEvent& event)
	{
		wxLogDebug("OnTool");

		if (CanHandleCommand(event.GetId()))
		{
			HandleCommand(event.GetId());
		}

		event.Skip();
	}

	bool PathToolBar::CanHandleCommand(int id) const
	{
		return (id == wxID_BACKWARD && mDirHistory.CanGoBack()) ||
			   (id == wxID_FORWARD && mDirHistory.CanGoForward()) ||
			   (id == wxID_UP && mDirHistory.CanGoUp());
	}

	void PathToolBar::HandleCommand(int id)
	{
		Expects(CanHandleCommand(id));

		switch (id)
		{
		case wxID_BACKWARD:
		{
			wxLogDebug("> Backward");
			mDirHistory.GoBack();
		}
		break;
		case wxID_FORWARD:
		{
			wxLogDebug("> Forward");
			mDirHistory.GoForward();
		}
		break;
		case wxID_UP:
		{
			wxLogDebug("> Up");
			mDirHistory.GoUp();
		}
		break;
		}

		mDirContentsListCtrl->SetDirectory(mDirHistory.Current());
	}
}
