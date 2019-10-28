#include "MainWindow.h"
#include "DirectoryContentsListCtrl.h"
#include "FileTreeCtrl.h"

CMainWindow::CMainWindow()
	: wxFrame(nullptr, wxID_ANY, "noire-suite - File Explorer"),
	  mMenuBar{ new wxMenuBar() },
	  mFileTreeCtrl{ new CFileTreeCtrl(this, wxID_ANY, { 5, 5 }, { 300, 800 }) },
	  mDirContentsListCtrl{
		  new CDirectoryContentsListCtrl(this, wxID_ANY, { 305, 5 }, { 500, 800 }),
	  }
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

	// TODO: change directory in mDirContentsListCtrl when selecting in mFileTreeCtrl
	mDirContentsListCtrl->SetDirectory(mFileTreeCtrl->File().Root());
}