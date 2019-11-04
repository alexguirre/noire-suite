#pragma once
#include <formats/fs/FileSystem.h>
#include <memory>
#include <string>
#include <string_view>
#include <wx/event.h>
#include <wx/listctrl.h>

class wxMenu;

namespace noire
{
	class WADChildDirectory;
	class WADChildFile;
}

class CDirectoryEvent : public wxEvent
{
public:
	CDirectoryEvent();
	CDirectoryEvent(std::string_view dirPath, int winId = wxID_ANY);
	CDirectoryEvent(const CDirectoryEvent& e);

	wxEvent* Clone() const override { return new CDirectoryEvent(*this); }

	void SetDirectory(std::string_view dirPath) { mDirPath = dirPath; }
	const std::string& GetDirectory() const { return mDirPath; }

private:
	std::string mDirPath;

	wxDECLARE_DYNAMIC_CLASS(CDirectoryEvent);
};

wxDECLARE_EVENT(EVT_DIRECTORY_CHANGED, CDirectoryEvent);

class CDirectoryContentsListCtrl : public wxListCtrl
{
public:
	CDirectoryContentsListCtrl(wxWindow* parent,
							   const wxWindowID id,
							   const wxPoint& pos = wxDefaultPosition,
							   const wxSize& size = wxDefaultSize);
	~CDirectoryContentsListCtrl() override;

	void SetFileSystem(noire::fs::CFileSystem* fileSystem);
	void SetDirectory(std::string_view dirPath);

	void OnItemContextMenu(wxListEvent& event);
	void OnItemActivated(wxListEvent& event);

private:
	void ShowItemContextMenu();
	void BuildColumns();
	void UpdateContents();
	void DeleteAllItemsData();
	void OpenFile(std::string_view filePath);

	noire::fs::CFileSystem* mFileSystem;
	std::string mDirPath;

	wxDECLARE_EVENT_TABLE();
};