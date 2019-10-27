#pragma once
#include <wx/imaglist.h>
#include <wx/menu.h>
#include <wx/treectrl.h>
#include <wx/wx.h>

class CMainWindow : public wxFrame
{
public:
	CMainWindow();

private:
	wxMenuBar* mMenuBar;
	wxTreeCtrl* mFileTreeCtrl;
	int mFileTreeNoireIcon;
	int mFileTreeFolderIcon;
	int mFileTreeBlueFolderIcon;
	int mFileTreeBlankFileIcon;
};