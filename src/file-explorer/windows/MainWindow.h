#pragma once
#include <formats/fs/FileSystem.h>
#include <memory>
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

	wxToolBar* OnCreateToolBar(long style, wxWindowID id, const wxString& name) override;

private:
	std::unique_ptr<noire::fs::CFileSystem> mFileSystem;
	wxMenuBar* mMenuBar;
	CDirectoryTreeCtrl* mDirTreeCtrl;
	CDirectoryContentsListCtrl* mDirContentsListCtrl;

	void OnDirectoryTreeSelectionChanged(wxTreeEvent& event);

	void CreateAccelTable();
};