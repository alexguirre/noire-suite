#pragma once
#include <filesystem>
#include <formats/fs/FileSystem.h>
#include <memory>
#include <wx/menu.h>
#include <wx/thread.h>
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
	wxStatusBar* OnCreateStatusBar(int, long style, wxWindowID id, const wxString& name) override;

private:
	wxMenuBar* mMenuBar;
	CDirectoryTreeCtrl* mDirTreeCtrl;
	CDirectoryContentsListCtrl* mDirContentsListCtrl;

	void OnDirectoryTreeSelectionChanged(wxTreeEvent& event);
	void OnOpenFolder(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnFileSystemScanStarted(wxThreadEvent& event);
	void OnFileSystemScanCompleted(wxThreadEvent& event);

	void CreateAccelTable();
};