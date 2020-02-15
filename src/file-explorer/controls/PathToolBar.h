#pragma once
#include "util/DirectoryHistory.h"
#include <wx/toolbar.h>

class wxTextCtrl;

namespace noire::explorer
{
	class DirectoryEvent;
	class DirectoryContentsListCtrl;

	class PathToolBar : public wxToolBar
	{
	public:
		PathToolBar(wxWindow* parent,
					wxWindowID id,
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = wxDefaultSize,
					long style = wxTB_DEFAULT_STYLE,
					const wxString& name = wxToolBarNameStr);

		void SetDirectoryContentsListCtrl(DirectoryContentsListCtrl* ctrl)
		{
			mDirContentsListCtrl = ctrl;
		}

	private:
		wxTextCtrl* mPathText;
		bool mResizeQueuedRealize;
		DirectoryHistory mDirHistory;
		DirectoryContentsListCtrl* mDirContentsListCtrl;

		void OnWindowResize(wxSizeEvent& e);
		void OnDirectoryChanged(DirectoryEvent& e);
		void OnTool(wxCommandEvent& event);

		bool CanHandleCommand(int id) const;
		void HandleCommand(int id);
	};
}
