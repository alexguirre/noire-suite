#pragma once
#include <filesystem>
#include <formats/fs/FileSystem.h>
#include <formats/fs/Path.h>
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

	// Opens a window for viewing the file contents.
	// Returns true if the file format is supported and a window has been opened, otherwise, false.
	bool OpenFile(noire::fs::SPathView filePath);

private:
	bool OpenDDSFile(noire::fs::SPathView filePath);
	bool OpenShaderProgramFile(noire::fs::SPathView filePath);
	bool OpenAttributeFile(noire::fs::SPathView filePath);
	bool OpenUniqueTextureVRamFile(noire::fs::SPathView filePath);

	std::unique_ptr<noire::fs::CFileSystem> mFileSystem;
	CMainWindow* mMainWindow;
};

wxDECLARE_APP(CApp);
