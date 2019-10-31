#pragma once
#include "util/DirectoryHistory.h"
#include <wx/toolbar.h>

class wxTextCtrl;
class CDirectoryEvent;
class CDirectoryContentsListCtrl;

class CPathToolBar : public wxToolBar
{
public:
	CPathToolBar(wxWindow* parent,
				 wxWindowID id,
				 const wxPoint& pos = wxDefaultPosition,
				 const wxSize& size = wxDefaultSize,
				 long style = wxTB_DEFAULT_STYLE,
				 const wxString& name = wxToolBarNameStr);

	void SetDirectoryContentsListCtrl(CDirectoryContentsListCtrl* ctrl)
	{
		mDirContentsListCtrl = ctrl;
	}

private:
	wxTextCtrl* mPathText;
	bool mResizeQueuedRealize;
	CDirectoryHistory mDirHistory;
	CDirectoryContentsListCtrl* mDirContentsListCtrl;

	void OnWindowResize(wxSizeEvent& e);
	void OnDirectoryChanged(CDirectoryEvent& e);
	void OnTool(wxCommandEvent& event);

	bool CanHandleCommand(int id) const;
	void HandleCommand(int id);
};