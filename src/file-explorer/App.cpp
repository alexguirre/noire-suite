#include "App.h"
#include "windows/MainWindow.h"
#include <IL/il.h>

wxIMPLEMENT_APP(CApp);

CApp::CApp() {}

bool CApp::OnInit()
{
	ilInit();

	mMainWindow = new CMainWindow;
	mMainWindow->Show();

	return true;
}

int CApp::OnExit()
{
	ilShutDown();

	return 0;
}