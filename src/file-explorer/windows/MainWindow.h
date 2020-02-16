#pragma once
#include <filesystem>
#include <memory>
#include <wx/menu.h>
#include <wx/thread.h>
#include <wx/wx.h>

class wxTreeEvent;
class wxTextCtrl;

namespace noire::explorer
{
	class DirectoryTreeCtrl;
	class DirectoryContentsListCtrl;

	class MainWindow : public wxFrame
	{
	public:
		MainWindow();

		wxToolBar* OnCreateToolBar(long style, wxWindowID id, const wxString& name) override;
		wxStatusBar*
		OnCreateStatusBar(int, long style, wxWindowID id, const wxString& name) override;

	private:
		wxMenuBar* mMenuBar;
		DirectoryTreeCtrl* mDirTreeCtrl;
		DirectoryContentsListCtrl* mDirContentsListCtrl;

		void OnDirectoryTreeSelectionChanged(wxTreeEvent& event);
		void OnOpenFolder(wxCommandEvent& event);
		void OnHashLookupTool(wxCommandEvent& event);
		void OnExit(wxCommandEvent& event);
		void OnFileSystemScanStarted(wxThreadEvent& event);
		void OnFileSystemScanCompleted(wxThreadEvent& event);

		void CreateAccelTable();
	};
}
