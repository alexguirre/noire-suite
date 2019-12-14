#include "MainWindow.h"
#include "Identifiers.h"
#include "controls/DirectoryContentsListCtrl.h"
#include "controls/DirectoryTreeCtrl.h"
#include "controls/ImagePanel.h"
#include "controls/PathToolBar.h"
#include "controls/StatusBar.h"
#include <App.h>
#include <gsl/gsl>
#include <windows/HashLookupWindow.h>
#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>

CMainWindow::CMainWindow()
	: wxFrame(nullptr, wxID_ANY, "noire-suite - File Explorer"),
	  mMenuBar{ new wxMenuBar() },
	  mDirTreeCtrl{ nullptr },
	  mDirContentsListCtrl{ nullptr }
{
	// construct menu bar
	{
		wxMenu* fileMenu = new wxMenu();
		fileMenu->Append(MenuBarOpenFolderId, "&Open folder...");
		fileMenu->AppendSeparator();
		fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4");

		wxMenu* toolsMenu = new wxMenu();
		toolsMenu->Append(MenuBarHashLookupId, "Hash &Lookup...");

		mMenuBar->Append(fileMenu, "&File");
		mMenuBar->Append(toolsMenu, "&Tools");
		SetMenuBar(mMenuBar);
	}

	CreateToolBar();

	CreateStatusBar();

	// create main directory tree and contents list within a splitter
	{
		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
		wxSplitterWindow* splitter = new wxSplitterWindow(this,
														  wxID_ANY,
														  wxDefaultPosition,
														  wxDefaultSize,
														  wxSP_LIVE_UPDATE);
		splitter->SetSashGravity(0.0);
		splitter->SetMinimumPaneSize(50);
		mainSizer->Add(splitter, 1, wxEXPAND);

		mDirTreeCtrl = { new CDirectoryTreeCtrl(splitter, wxID_ANY) };
		mDirContentsListCtrl = { new CDirectoryContentsListCtrl(splitter, wxID_ANY) };

		splitter->SplitVertically(mDirTreeCtrl, mDirContentsListCtrl);

		SetSizer(mainSizer);
	}

	CreateAccelTable();

	SetMinSize({ 650, 350 });

	// TODO: giving the tool bar non-const access to mDirContentsListCtrl is a bit dirty
	reinterpret_cast<CPathToolBar*>(GetToolBar())
		->SetDirectoryContentsListCtrl(mDirContentsListCtrl);

	mDirTreeCtrl->Bind(wxEVT_TREE_SEL_CHANGED, &CMainWindow::OnDirectoryTreeSelectionChanged, this);

	Bind(wxEVT_MENU, &CMainWindow::OnOpenFolder, this, MenuBarOpenFolderId);
	Bind(wxEVT_MENU, &CMainWindow::OnHashLookupTool, this, MenuBarHashLookupId);
	Bind(wxEVT_MENU, &CMainWindow::OnExit, this, wxID_EXIT);
	wxGetApp().Bind(EVT_FILE_SYSTEM_SCAN_STARTED, &CMainWindow::OnFileSystemScanStarted, this);
	wxGetApp().Bind(EVT_FILE_SYSTEM_SCAN_COMPLETED, &CMainWindow::OnFileSystemScanCompleted, this);
}

wxToolBar* CMainWindow::OnCreateToolBar(long style, wxWindowID id, const wxString& name)
{
	return new CPathToolBar(this, id, wxDefaultPosition, wxDefaultSize, style, name);
}

wxStatusBar* CMainWindow::OnCreateStatusBar(int, long style, wxWindowID id, const wxString& name)
{
	return new CStatusBar(this, id, style, name);
}

void CMainWindow::OnDirectoryTreeSelectionChanged(wxTreeEvent& event)
{
	wxTreeItemId itemId = event.GetItem();
	Expects(itemId.IsOk());

	const CDirectoryItemData* data =
		reinterpret_cast<CDirectoryItemData*>(mDirTreeCtrl->GetItemData(itemId));

	if (data)
	{
		mDirContentsListCtrl->SetDirectory(data->Path());

		event.Skip();
	}
}

void CMainWindow::OnOpenFolder(wxCommandEvent&)
{
	wxDirDialog dlg(this,
					"Select the game folder:",
					wxEmptyString, // TODO: try to find game folder (looking in registry?)
					wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

	if (dlg.ShowModal() != wxID_OK)
	{
		return;
	}

	std::filesystem::path rootPath = dlg.GetPath().c_str().AsChar();
	wxGetApp().ChangeRootPath(rootPath);
}

void CMainWindow::OnHashLookupTool(wxCommandEvent&)
{
	CHashLookupWindow* win = new CHashLookupWindow(this);
	win->Show();
}

void CMainWindow::OnExit(wxCommandEvent&)
{
	Close(true);
}

void CMainWindow::OnFileSystemScanStarted(wxThreadEvent& event)
{
	wxLogDebug(__FUNCTION__);

	if (CStatusBar* statusBar = reinterpret_cast<CStatusBar*>(GetStatusBar()); statusBar)
	{
		statusBar->SetInfo(wxString::Format("Scanning '%ls'...", "path/TBD/"));
	}

	event.Skip();
}

void CMainWindow::OnFileSystemScanCompleted(wxThreadEvent& event)
{
	if (mDirTreeCtrl)
	{
		mDirTreeCtrl->Refresh();
	}

	if (mDirContentsListCtrl)
	{
		mDirContentsListCtrl->SetDirectoryToRoot();
	}

	if (CStatusBar* statusBar = reinterpret_cast<CStatusBar*>(GetStatusBar()); statusBar)
	{
		statusBar->SetInfo("");
	}

	event.Skip();
}

void CMainWindow::CreateAccelTable()
{
	std::array<wxAcceleratorEntry, 5> entries{};
	std::size_t i = 0;

	// for path tool bar back and forward buttons
	// TODO: back/forward accelerators for mouse extra buttons, like windows file explorer
	entries[i++].Set(wxACCEL_ALT, WXK_LEFT, wxID_BACKWARD);
	entries[i++].Set(wxACCEL_NORMAL, WXK_BROWSER_BACK, wxID_BACKWARD);
	entries[i++].Set(wxACCEL_ALT, WXK_RIGHT, wxID_FORWARD);
	entries[i++].Set(wxACCEL_NORMAL, WXK_BROWSER_FORWARD, wxID_FORWARD);
	entries[i++].Set(wxACCEL_ALT, WXK_UP, wxID_UP);

	SetAcceleratorTable({ entries.size(), entries.data() });
}