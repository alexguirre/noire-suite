#pragma once
#include <formats/fs/FileSystem.h>
#include <formats/fs/Path.h>
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
	CDirectoryEvent(noire::fs::SPathView dirPath, int winId = wxID_ANY);
	CDirectoryEvent(const CDirectoryEvent& e);

	wxEvent* Clone() const override { return new CDirectoryEvent(*this); }

	void SetDirectory(noire::fs::SPathView dirPath) { mDirPath = dirPath; }
	noire::fs::SPathView GetDirectory() const { return mDirPath; }

private:
	noire::fs::SPath mDirPath;

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
	void SetDirectory(noire::fs::SPathView dirPath);

	void OnItemContextMenu(wxListEvent& event);
	void OnItemActivated(wxListEvent& event);

private:
	void ShowItemContextMenu();
	void BuildColumns();
	void UpdateContents();
	void DeleteAllItemsData();
	void OpenFile(noire::fs::SPathView filePath);

	noire::fs::CFileSystem* mFileSystem;
	noire::fs::SPath mDirPath;

	wxDECLARE_EVENT_TABLE();
};