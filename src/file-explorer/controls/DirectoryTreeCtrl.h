#pragma once
#include <formats/WADFile.h>
#include <formats/fs/FileSystem.h>
#include <string>
#include <string_view>
#include <wx/treectrl.h>

class CDirectoryItemData : public wxTreeItemData
{
public:
	CDirectoryItemData(noire::fs::SPathView path) : mPath{ path } {}

	noire::fs::SPathView Path() const { return mPath; }

private:
	noire::fs::SPath mPath;
};

class CDirectoryTreeCtrl : public wxTreeCtrl
{
public:
	CDirectoryTreeCtrl(wxWindow* parent,
					   const wxWindowID id,
					   const wxPoint& pos = wxDefaultPosition,
					   const wxSize& size = wxDefaultSize);

	void Refresh();

private:
	void OnItemContextMenu(wxTreeEvent& event);
	void ShowItemContextMenu(wxTreeItemId id, const wxPoint& pos);

	wxDECLARE_EVENT_TABLE();
};