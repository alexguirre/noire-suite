#pragma once
#include <formats/WADFile.h>
#include <formats/fs/FileSystem.h>
#include <string>
#include <string_view>
#include <wx/treectrl.h>

class CDirectoryItemData : public wxTreeItemData
{
public:
	CDirectoryItemData(std::string_view path) : mPath{ path } {}

	const std::string& Path() const { return mPath; }

private:
	std::string mPath;
};

class CDirectoryTreeCtrl : public wxTreeCtrl
{
public:
	CDirectoryTreeCtrl(wxWindow* parent,
					   const wxWindowID id,
					   const wxPoint& pos = wxDefaultPosition,
					   const wxSize& size = wxDefaultSize);

	void SetFileSystem(noire::fs::CFileSystem* fileSystem);

private:
	void Refresh();
	void OnItemContextMenu(wxTreeEvent& event);
	void ShowItemContextMenu(wxTreeItemId id, const wxPoint& pos);

	noire::fs::CFileSystem* mFileSystem;

	wxDECLARE_EVENT_TABLE();
};