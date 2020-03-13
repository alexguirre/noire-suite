#pragma once
#include <core/devices/MultiDevice.h>
#include <filesystem>
#include <memory>
#include <wx/app.h>
#include <wx/event.h>

namespace noire::explorer
{
	class MainWindow;

	class App : public wxApp
	{
	public:
		App();

		virtual bool OnInit();
		virtual int OnExit();

		void ChangeRootPath(const std::filesystem::path& path);
		MultiDevice* RootDevice() { return mRootDevice.get(); }

		// Opens a window for viewing the file contents.
		// Returns true if the file format is supported and a window has been opened, otherwise,
		// false.
		bool OpenFile(PathView filePath);

	private:
		bool OpenDDSFile(PathView filePath);
		bool OpenShaderProgramFile(PathView filePath);
		bool OpenAttributeFile(PathView filePath);
		bool OpenTrunkFile(PathView filePath);

		std::unique_ptr<MultiDevice> mRootDevice;
		MainWindow* mMainWindow;
	};
}

wxDECLARE_APP(noire::explorer::App);
