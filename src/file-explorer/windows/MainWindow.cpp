#include "MainWindow.h"
#include "Identifiers.h"
#include "controls/DirectoryContentsListCtrl.h"
#include "controls/DirectoryTreeCtrl.h"
#include "controls/ImagePanel.h"
#include "controls/PathToolBar.h"
#include <formats/fs/NativeDevice.h>
#include <formats/fs/WADDevice.h>
#include <gsl/gsl>
#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>

CMainWindow::CMainWindow()
	: wxFrame(nullptr, wxID_ANY, "noire-suite - File Explorer"),
	  mFileSystem{ nullptr },
	  mMenuBar{ new wxMenuBar() },
	  mDirTreeCtrl{ nullptr },
	  mDirContentsListCtrl{ nullptr }
{
#if _DEBUG
	// hardcoded path to aid in development as the last opened folder is not saved yet
	ChangeRootPath("E:\\Rockstar Games\\L.A. Noire Complete Edition\\");
#endif

	// construct menu bar
	{
		wxMenu* fileMenu = new wxMenu();
		fileMenu->Append(MenuBarOpenFolderId, "&Open folder...");
		fileMenu->AppendSeparator();
		fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4");

		mMenuBar->Append(fileMenu, "&File");
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

		mDirTreeCtrl->SetFileSystem(mFileSystem.get());
		mDirContentsListCtrl->SetFileSystem(mFileSystem.get());

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
	Bind(wxEVT_MENU, &CMainWindow::OnExit, this, wxID_EXIT);
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
	ChangeRootPath(rootPath);
}

void CMainWindow::OnExit(wxCommandEvent&)
{
	Close(true);
}

void CMainWindow::ChangeRootPath(const std::filesystem::path& path)
{
	// TODO: remember last opened folder after closing the application

	mFileSystem = std::make_unique<noire::fs::CFileSystem>();

	mFileSystem->Mount("/", std::make_unique<noire::fs::CNativeDevice>(path));
	noire::fs::IDevice* rootDevice = mFileSystem->FindDevice("/");

	// TODO: scan the directory automatically for possible IDevices as this WAD file may not exist
	// if the user opens a folder different from the regular game installation
	mFileSystem->Mount("/final/pc/out.wad.pc/",
					   std::make_unique<noire::fs::CWADDevice>(*rootDevice, "final/pc/out.wad.pc"));

	if (mDirTreeCtrl)
	{
		mDirTreeCtrl->SetFileSystem(mFileSystem.get());
	}

	if (mDirContentsListCtrl)
	{
		mDirContentsListCtrl->SetFileSystem(mFileSystem.get());
	}
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