#pragma once
#include <wx/menu.h>
#include <wx/wx.h>

class CFileTreeCtrl;
class CDirectoryContentsListCtrl;

class wxTreeEvent;

class CMainWindow : public wxFrame
{
public:
	CMainWindow();

private:
	wxMenuBar* mMenuBar;
	CFileTreeCtrl* mFileTreeCtrl;
	CDirectoryContentsListCtrl* mDirContentsListCtrl;

	void OnDirectoryTreeItemActivated(wxTreeEvent& event);
};