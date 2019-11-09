#include "DirectoryTreeCtrl.h"
#include "Identifiers.h"
#include "Images.h"
#include <filesystem>
#include <formats/fs/NativeDevice.h>
#include <gsl/gsl>
#include <string>
#include <unordered_map>
#include <wx/icon.h>
#include <wx/imaglist.h>
#include <wx/msgdlg.h>
#include <wx/wx.h>

using namespace noire::fs;

// clang-format off
wxBEGIN_EVENT_TABLE(CDirectoryTreeCtrl, wxTreeCtrl)
	// for context menu
	EVT_TREE_ITEM_MENU(wxID_ANY, CDirectoryTreeCtrl::OnItemContextMenu)
wxEND_EVENT_TABLE();
// clang-format on

static constexpr long TreeCtrlStyle = wxTR_HAS_BUTTONS | wxTR_TWIST_BUTTONS | wxTR_NO_LINES |
									  wxTR_HIDE_ROOT | wxTR_LINES_AT_ROOT | wxTR_SINGLE;

CDirectoryTreeCtrl::CDirectoryTreeCtrl(wxWindow* parent,
									   const wxWindowID id,
									   const wxPoint& pos,
									   const wxSize& size)
	: wxTreeCtrl(parent, id, pos, size, TreeCtrlStyle), mFileSystem{ nullptr }
{
	SetImageList(CImages::Icons());
}

void CDirectoryTreeCtrl::SetFileSystem(CFileSystem* fileSystem)
{
	if (fileSystem != mFileSystem)
	{
		mFileSystem = fileSystem;
		Refresh();
	}
}

void CDirectoryTreeCtrl::Refresh()
{
	this->DeleteAllItems();

	if (mFileSystem == nullptr)
	{
		return;
	}

	// initialize tree
	wxTreeItemId root = AddRoot("ROOT");
	wxTreeItemId noireItem =
		AppendItem(root, "L.A. Noire", CImages::IconNoire, -1, new CDirectoryItemData("/"));

	std::vector<SDirectoryEntry> entries = mFileSystem->GetAllEntries();
	// string_view key points to the path of a SDirectoryEntry
	std::unordered_map<std::string_view, wxTreeItemId> items{};

	// get the path of each directory in the input path in order
	// for example "/some/deep/folder/" returns { "/some/deep/folder/", "/some/deep/", "/some/" }
	const auto getDirectoriesFromPath = [](SPathView path, std::vector<SPathView>& directories) {
		SPathView p = path;
		while (!p.IsEmpty() && !p.IsRoot())
		{
			directories.emplace_back(p);
			p = p.Parent();
		}
	};

	const auto addDirectoryToTree = [&](SPathView path, EDirectoryEntryType type) {
		SPathView origPath = path;

		std::vector<SPathView> dirs{};
		getDirectoriesFromPath(path, dirs);

		wxTreeItemId parent = noireItem;
		// iterate in reverse order so top-most parent is processed first
		for (auto dirIt = dirs.rbegin(); dirIt != dirs.rend(); dirIt++)
		{
			SPathView& p = *dirIt;
			if (auto it = items.find(p.String()); it != items.end())
			{
				// already added
				parent = it->second;
			}
			else
			{
				std::string_view name = p.Name();
				parent = AppendItem(parent,
									{ name.data(), name.size() },
									CImages::IconFolder,
									-1,
									new CDirectoryItemData(p));
				items.emplace(p.String(), parent);
			}
		}

		if (type == EDirectoryEntryType::Collection)
		{
			if (auto it = items.find(path.String()); it != items.end())
			{
				SetItemImage(it->second, CImages::IconBlueFolder);
			}
		}
	};

	for (auto& entry : entries)
	{
		if (entry.Type != EDirectoryEntryType::File)
		{
			addDirectoryToTree(entry.Path, entry.Type);
		}
	}
}

void CDirectoryTreeCtrl::OnItemContextMenu(wxTreeEvent& event)
{
	wxTreeItemId itemId = event.GetItem();
	Expects(itemId.IsOk());

	ShowItemContextMenu(itemId, event.GetPoint());
	event.Skip();
}

void CDirectoryTreeCtrl::ShowItemContextMenu(wxTreeItemId, const wxPoint&)
{
	// nothing
}
