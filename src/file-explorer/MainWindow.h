#pragma once
#include <wx/menu.h>
#include <wx/wx.h>

class CFileTreeCtrl;

class CMainWindow : public wxFrame
{
public:
	CMainWindow();

private:
	wxMenuBar* mMenuBar;
	CFileTreeCtrl* mFileTreeCtrl;
};