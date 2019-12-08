#pragma once
#include <array>
#include <formats/fs/FileSystem.h>
#include <formats/fs/Path.h>
#include <memory>
#include <string>
#include <string_view>
#include <wx/event.h>
#include <wx/listctrl.h>

class wxDataObject;

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

	void SetDirectoryToRoot();
	void SetDirectory(noire::fs::SPathView dirPath);

	void OnItemContextMenu(wxListEvent& event);
	void OnItemActivated(wxListEvent& event);
	void OnBeginDrag(wxListEvent& event);
	void OnCopy(wxCommandEvent& event);
	wxString OnGetItemText(long item, long column) const override;
	int OnGetItemImage(long item) const override;

private:
	void OnColClick(wxListEvent& event);
	void ShowItemContextMenu();
	void BuildColumns();
	void UpdateContents();
	void SortContents(long column, bool ascending);

	wxDataObject* CreateSelectedFilesDataObject() const;

	noire::fs::SPath mDirPath;
	std::vector<noire::fs::SDirectoryEntry> mDirEntries;
	std::array<bool, 3> mColSortDescending;

	wxDECLARE_EVENT_TABLE();
};