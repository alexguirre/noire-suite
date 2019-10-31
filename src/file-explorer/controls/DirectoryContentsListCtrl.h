#pragma once
#include <memory>
#include <wx/listctrl.h>

class wxMenu;

namespace noire
{
	class WADChildDirectory;
	class WADChildFile;
}

class CDirectoryContentsListCtrl : public wxListCtrl
{
public:
	CDirectoryContentsListCtrl(wxWindow* parent,
							   const wxWindowID id,
							   const wxPoint& pos = wxDefaultPosition,
							   const wxSize& size = wxDefaultSize);

	void SetDirectory(const noire::WADChildDirectory& dir);

	void OnItemContextMenu(wxListEvent& event);
	void OnItemActivated(wxListEvent& event);

private:
	void ShowItemContextMenu();
	void BuildColumns();
	void UpdateContents();
	void OpenFile(const noire::WADChildFile& file);

	const noire::WADChildDirectory* mCurrentDirectory;
	std::unique_ptr<wxMenu> mItemContextMenu;

	wxDECLARE_EVENT_TABLE();
};