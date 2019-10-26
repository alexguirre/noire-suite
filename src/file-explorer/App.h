#pragma once
#include "MainWindow.h"
#include <memory>
#include <wx/windowptr.h>
#include <wx/wx.h>

class CApp : public wxApp
{
public:
	CApp();

	virtual bool OnInit();

private:
	CMainWindow* mMainWindow{ nullptr };
};