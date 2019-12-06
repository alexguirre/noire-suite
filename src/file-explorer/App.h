#pragma once
#include <filesystem>
#include <formats/fs/FileSystem.h>
#include <memory>
#include <wx/app.h>
#include <wx/event.h>

class CMainWindow;

wxDECLARE_EVENT(EVT_FILE_SYSTEM_SCAN_STARTED, wxThreadEvent);
wxDECLARE_EVENT(EVT_FILE_SYSTEM_SCAN_COMPLETED, wxThreadEvent);

class CApp : public wxApp
{
public:
	CApp();

	virtual bool OnInit();
	virtual int OnExit();

	void ChangeRootPath(const std::filesystem::path& path);
	noire::fs::CFileSystem* FileSystem() { return mFileSystem.get(); }

private:
	void OnFileSystemScanStarted(wxThreadEvent& event);
	void OnFileSystemScanCompleted(wxThreadEvent& event);

	std::unique_ptr<noire::fs::CFileSystem> mFileSystem;
	CMainWindow* mMainWindow;
};

wxDECLARE_APP(CApp);
