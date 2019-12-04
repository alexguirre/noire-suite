#pragma once
#include <wx/app.h>

class CMainWindow;

class CApp : public wxApp
{
public:
	CApp();

	virtual bool OnInit();
	virtual int OnExit();

private:
	CMainWindow* mMainWindow{ nullptr };
};

wxDECLARE_APP(CApp);
