#pragma once
#include <array>
#include <core/Path.h>
#include <memory>
#include <string>
#include <string_view>
#include <wx/event.h>
#include <wx/listctrl.h>

class wxDataObject;

namespace noire::explorer
{
	class DirectoryEvent : public wxEvent
	{
	public:
		DirectoryEvent();
		DirectoryEvent(PathView dirPath, int winId = wxID_ANY);
		DirectoryEvent(const DirectoryEvent& e);

		wxEvent* Clone() const override { return new DirectoryEvent(*this); }

		void SetDirectory(PathView dirPath) { mDirPath = dirPath; }
		PathView GetDirectory() const { return mDirPath; }

	private:
		Path mDirPath;

		wxDECLARE_DYNAMIC_CLASS(DirectoryEvent);
	};

	wxDECLARE_EVENT(EVT_DIRECTORY_CHANGED, DirectoryEvent);

	class DirectoryContentsListCtrl : public wxListCtrl
	{
	public:
		DirectoryContentsListCtrl(wxWindow* parent,
								  const wxWindowID id,
								  const wxPoint& pos = wxDefaultPosition,
								  const wxSize& size = wxDefaultSize);

		void SetDirectoryToRoot();
		void SetDirectory(PathView dirPath);

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

		struct Entry
		{
			Path Path;
			bool IsDirectory;
			u64 Size;
		};

		Path mDirPath;
		std::vector<Entry> mDirEntries;
		std::array<bool, 3> mColSortDescending;

		wxDECLARE_EVENT_TABLE();
	};
}
