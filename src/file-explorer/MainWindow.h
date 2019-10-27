#pragma once
#include <wx/imaglist.h>
#include <wx/treectrl.h>
#include <wx/wx.h>

class CMainWindow : public wxFrame
{
public:
	CMainWindow();

private:
	wxTreeCtrl* mFileTreeCtrl;
	int mFileTreeNoireIcon;
	int mFileTreeFolderIcon;
	int mFileTreeBlankFileIcon;
};