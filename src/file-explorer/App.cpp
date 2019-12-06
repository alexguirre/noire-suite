#include "App.h"
#include "windows/MainWindow.h"
#include <IL/il.h>
#include <formats/ContainerFile.h>
#include <formats/File.h>
#include <formats/WADFile.h>
#include <formats/fs/ContainerDevice.h>
#include <formats/fs/NativeDevice.h>
#include <formats/fs/ShaderProgramsDevice.h>
#include <formats/fs/TrunkDevice.h>
#include <formats/fs/WADDevice.h>
#include <processthreadsapi.h>
#include <thread>

wxIMPLEMENT_APP(CApp);

wxDEFINE_EVENT(EVT_FILE_SYSTEM_SCAN_STARTED, wxThreadEvent);
wxDEFINE_EVENT(EVT_FILE_SYSTEM_SCAN_COMPLETED, wxThreadEvent);

CApp::CApp() : mFileSystem{ nullptr }, mMainWindow{ nullptr } {}

bool CApp::OnInit()
{
	ilInit();

	Bind(EVT_FILE_SYSTEM_SCAN_STARTED, &CApp::OnFileSystemScanStarted, this);
	Bind(EVT_FILE_SYSTEM_SCAN_COMPLETED, &CApp::OnFileSystemScanCompleted, this);

	mMainWindow = new CMainWindow;
	mMainWindow->Show();

	return true;
}

int CApp::OnExit()
{
	ilShutDown();

	return 0;
}

void CApp::ChangeRootPath(const std::filesystem::path& path)
{
	// TODO: remember last opened folder after closing the application
	using namespace noire;
	using namespace noire::fs;

	std::thread t{ [this, path]() {
		wxQueueEvent(this, new wxThreadEvent(EVT_FILE_SYSTEM_SCAN_STARTED));

		std::unique_ptr fs = std::make_unique<CFileSystem>();

		// CContainerDevice
		fs->RegisterDeviceType(&TFileTraits<CContainerFile>::IsValid,
							   [](CFileSystem&, IDevice& d, SPathView relPath) {
								   return std::make_unique<CContainerDevice>(d, relPath);
							   });

		// CWADDevice
		fs->RegisterDeviceType(&TFileTraits<WADFile>::IsValid,
							   [](CFileSystem&, IDevice& d, SPathView relPath) {
								   return std::make_unique<CWADDevice>(d, relPath);
							   });

		// CTrunkDevice
		fs->RegisterDeviceType(&TFileTraits<CTrunkFile>::IsValid,
							   [](CFileSystem&, IDevice& d, SPathView relPath) {
								   return std::make_unique<CTrunkDevice>(d, relPath);
							   });

		// CShaderProgramsDevice
		fs->RegisterDeviceType(&TFileTraits<CShaderProgramsFile>::IsValid,
							   [](CFileSystem&, IDevice& d, SPathView relPath) {
								   return std::make_unique<CShaderProgramsDevice>(d, relPath);
							   });

		fs->EnableDeviceScanning(true);
		fs->Mount("/", std::make_unique<CNativeDevice>(path));

		mFileSystem = std::move(fs);
		wxQueueEvent(this, new wxThreadEvent(EVT_FILE_SYSTEM_SCAN_COMPLETED));
	} };

	SetThreadPriority(t.native_handle(), THREAD_PRIORITY_TIME_CRITICAL);
	t.detach();
}

void CApp::OnFileSystemScanStarted(wxThreadEvent& event)
{
	wxLogDebug(__FUNCTION__);

	event.Skip();
}

void CApp::OnFileSystemScanCompleted(wxThreadEvent& event)
{
	wxLogDebug(__FUNCTION__);

	event.Skip();
}