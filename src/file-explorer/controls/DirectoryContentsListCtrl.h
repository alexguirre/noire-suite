#pragma once
#include <memory>
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
	CDirectoryEvent(const noire::WADChildDirectory& dir, int winId = wxID_ANY);
	CDirectoryEvent(const CDirectoryEvent& e);

	wxEvent* Clone() const override { return new CDirectoryEvent(*this); }

	void SetDirectory(const noire::WADChildDirectory& dir) { mDir = &dir; }
	const noire::WADChildDirectory& GetDirectory() const { return *mDir; }

private:
	const noire::WADChildDirectory* mDir;

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