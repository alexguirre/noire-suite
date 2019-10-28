#pragma once
#include <wx/menu.h>
#include <wx/wx.h>

class CDirectoryTreeCtrl;
class CDirectoryContentsListCtrl;

class wxTreeEvent;

class CMainWindow : public wxFrame
{
public:
	CMainWindow();

private:
	wxMenuBar* mMenuBar;
	CDirectoryTreeCtrl* mFileTreeCtrl;
	CDirectoryContentsListCtrl* mDirContentsListCtrl;

	void OnDirectoryTreeItemActivated(wxTreeEvent& event);
};