#include "MainWindow.h"
#include "FileTreeCtrl.h"

CMainWindow::CMainWindow()
	: wxFrame(nullptr, wxID_ANY, "noire-suite - File Explorer"),
	  mMenuBar{ new wxMenuBar() },
	  mFileTreeCtrl{ new CFileTreeCtrl(this, wxID_ANY, { 5, 5 }, { 150, 800 }) }
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
}