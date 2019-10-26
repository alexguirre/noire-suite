#pragma once
#include <wx/treectrl.h>
#include <wx/wx.h>

class CMainWindow : public wxFrame
{
public:
	CMainWindow();

private:
	wxTreeCtrl* mFileTreeCtrl;
};