#pragma once
#include <formats/WADFile.h>
#include <formats/fs/FileStream.h>
#include <wx/treectrl.h>

class CDirectoryItemData : public wxTreeItemData
{
public:
	CDirectoryItemData(const noire::WADChildDirectory& dir) : mDir{ dir } {}

	const noire::WADChildDirectory& Directory() const { return mDir; }

private:
	const noire::WADChildDirectory& mDir;
};

class CDirectoryTreeCtrl : public wxTreeCtrl
{
public:
	CDirectoryTreeCtrl(wxWindow* parent,
					   const wxWindowID id,
					   const wxPoint& pos = wxDefaultPosition,
					   const wxSize& size = wxDefaultSize);

	const noire::WADFile& File() const { return mFile; }

private:
	void LoadDummyData();

public:
	void OnItemContextMenu(wxTreeEvent& event);

private:
	void ShowItemContextMenu(wxTreeItemId id, const wxPoint& pos);

	std::unique_ptr<noire::fs::IFileStream> mFileStream;
	noire::WADFile mFile;

	wxDECLARE_EVENT_TABLE();
};