#pragma once
#include <wx/menu.h>
#include <wx/wx.h>

class CDirectoryTreeCtrl;
class CDirectoryContentsListCtrl;

class wxTreeEvent;
class wxTextCtrl;

class CMainWindow : public wxFrame
{
public:
	CMainWindow();

private:
	wxMenuBar* mMenuBar;
	CDirectoryTreeCtrl* mDirTreeCtrl;
	CDirectoryContentsListCtrl* mDirContentsListCtrl;
	wxTextCtrl* mDirPathText;

	void OnDirectoryTreeSelectionChanged(wxTreeEvent& event);
};