#include "MainWindow.h"
#include "controls/DirectoryContentsListCtrl.h"
#include "controls/DirectoryTreeCtrl.h"
#include "controls/ImagePanel.h"
#include "controls/PathToolBar.h"
#include <gsl/gsl>
#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>

CMainWindow::CMainWindow()
	: wxFrame(nullptr, wxID_ANY, "noire-suite - File Explorer"), mMenuBar{ new wxMenuBar() }
{
	// construct menu bar
	{
		wxMenu* fileMenu = new wxMenu();
		fileMenu->Append(wxID_ANY, "Open folder...");
		fileMenu->AppendSeparator();
		fileMenu->Append(wxID_ANY, "Exit");

		mMenuBar->Append(fileMenu, "File");
		SetMenuBar(mMenuBar);
	}

	CreateToolBar();

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

	mDirContentsListCtrl->SetDirectory(mDirTreeCtrl->File().Root());

	mDirTreeCtrl->Bind(wxEVT_TREE_SEL_CHANGED, &CMainWindow::OnDirectoryTreeSelectionChanged, this);
}

wxToolBar* CMainWindow::OnCreateToolBar(long style, wxWindowID id, const wxString& name)
{
	return new CPathToolBar(this, id, wxDefaultPosition, wxDefaultSize, style, name);
}

void CMainWindow::OnDirectoryTreeSelectionChanged(wxTreeEvent& event)
{
	wxTreeItemId itemId = event.GetItem();
	Expects(itemId.IsOk());

	const CDirectoryItemData* data =
		reinterpret_cast<CDirectoryItemData*>(mDirTreeCtrl->GetItemData(itemId));

	if (data)
	{
		mDirContentsListCtrl->SetDirectory(data->Directory());

		event.Skip();
	}
}

void CMainWindow::CreateAccelTable()
{
	std::array<wxAcceleratorEntry, 4> entries{};
	std::size_t i = 0;

	// for path tool bar back and forward buttons
	// TODO: back/forward accelerators for mouse extra buttons, like windows file explorer
	entries[i++].Set(wxACCEL_ALT, WXK_LEFT, wxID_BACKWARD);
	entries[i++].Set(wxACCEL_NORMAL, WXK_BROWSER_BACK, wxID_BACKWARD);
	entries[i++].Set(wxACCEL_ALT, WXK_RIGHT, wxID_FORWARD);
	entries[i++].Set(wxACCEL_NORMAL, WXK_BROWSER_FORWARD, wxID_FORWARD);

	SetAcceleratorTable({ entries.size(), entries.data() });
}