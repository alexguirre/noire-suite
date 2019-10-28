#pragma once
#include <memory>
#include <wx/listctrl.h>

class wxMenu;

namespace noire
{
	class WADChildDirectory;
}

class CDirectoryContentsListCtrl : public wxListCtrl
{
public:
	CDirectoryContentsListCtrl(wxWindow* parent,
							   const wxWindowID id,
							   const wxPoint& pos,
							   const wxSize& size);

	void SetDirectory(const noire::WADChildDirectory& dir);

	void OnItemContextMenu(wxListEvent& event);
	void OnItemActivated(wxListEvent& event);

private:
	enum Icon
	{
		IconBlankFile,
		IconBlueFolder,
		IconFolder,
		IconNoire,

		IconCount,
	};

	void LoadIcons();
	void ShowItemContextMenu();
	void BuildColumns();
	void UpdateContents();

	const noire::WADChildDirectory* mCurrentDirectory;
	std::unique_ptr<wxMenu> mItemContextMenu;

	wxDECLARE_EVENT_TABLE();
};