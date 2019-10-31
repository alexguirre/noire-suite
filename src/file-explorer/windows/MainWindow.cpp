#include "MainWindow.h"
#include "controls/DirectoryContentsListCtrl.h"
#include "controls/DirectoryTreeCtrl.h"
#include "controls/ImagePanel.h"
#include <gsl/gsl>
#include <wx/button.h>
#include <wx/splitter.h>

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

	SetMinSize({ 650, 350 });

	mDirContentsListCtrl->SetDirectory(mDirTreeCtrl->File().Root());

	mDirTreeCtrl->Bind(wxEVT_TREE_SEL_CHANGED, &CMainWindow::OnDirectoryTreeSelectionChanged, this);
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