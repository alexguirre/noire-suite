#pragma once
#include <wx/menu.h>
#include <wx/wx.h>

class CFileTreeCtrl;
class CDirectoryContentsListCtrl;

class CMainWindow : public wxFrame
{
public:
	CMainWindow();

private:
	wxMenuBar* mMenuBar;
	CFileTreeCtrl* mFileTreeCtrl;
	CDirectoryContentsListCtrl* mDirContentsListCtrl;
};