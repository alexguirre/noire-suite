#include "MainWindow.h"

CMainWindow::CMainWindow()
	: wxFrame(nullptr, wxID_ANY, "noire-suite - File Explorer"),
	  mFileTreeCtrl{ new wxTreeCtrl(this, wxID_ANY, { 5, 5 }, { 150, 800 }) }
{
	wxTreeItemId root = mFileTreeCtrl->AddRoot("L.A. Noire");
	wxTreeItemId finalItem = mFileTreeCtrl->AppendItem(root, "final");
	wxTreeItemId pcItem = mFileTreeCtrl->AppendItem(finalItem, "pc");
	wxTreeItemId outWadItem = mFileTreeCtrl->AppendItem(pcItem, "out.wad.pc");
}