#include "App.h"

wxIMPLEMENT_APP(CApp);

CApp::CApp() {}

bool CApp::OnInit()
{
	mMainWindow = new CMainWindow;
	mMainWindow->Show();

	return true;
}