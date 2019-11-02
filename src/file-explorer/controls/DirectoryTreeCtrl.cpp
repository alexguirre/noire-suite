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

void CDirectoryTreeCtrl::SetFileSystem(noire::fs::CFileSystem* fileSystem)
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

	std::vector<noire::fs::SDirectoryEntry> entries = mFileSystem->GetAllEntries();
	// string_view key points to the path of a SDirectoryEntry
	std::unordered_map<std::string_view, wxTreeItemId> items{};

	// get the path of each directory in the input path in order
	// for example "some/deep/folder" returns { "some/", "some/deep", "some/deep/folder" }
	const auto getDirectoriesFromPath = [](std::string_view path,
										   std::vector<std::string_view>& directories) {
		std::size_t pos = 0;
		do
		{
			pos = path.find('/', pos + 1);
			if (pos != std::string_view::npos)
			{
				directories.emplace_back(path.substr(0, pos));
			}
			else
			{
				directories.emplace_back(path);
			}

		} while (pos != std::string_view::npos);
	};

	const auto getNameFromPath = [](std::string_view path) {
		std::size_t namePos =
			path.rfind(noire::fs::CFileSystem::DirectorySeparator, path.size() - 2);
		if (namePos != std::string_view::npos)
		{
			return path.substr(
				namePos + (path[namePos] == noire::fs::CFileSystem::DirectorySeparator ? 1 : 0));
		}
		else
		{
			return path;
		}
	};

	const auto addDirectoryToTree = [&](std::string_view path) {
		std::string_view origPath = path;
		path = path.substr(1, path.size() - 2); // remove first and last '/'

		std::vector<std::string_view> dirs{};
		getDirectoriesFromPath(path, dirs);

		wxTreeItemId parent = noireItem;
		for (auto& p : dirs)
		{
			if (auto it = items.find(p); it != items.end())
			{
				// already added
				parent = it->second;
			}
			else
			{
				std::string_view name = getNameFromPath(p);
				std::string normalizedPath =
					std::string{ noire::fs::CFileSystem::DirectorySeparator } + std::string{ p } +
					noire::fs::CFileSystem::DirectorySeparator;
				parent = AppendItem(parent,
									{ name.data(), name.size() },
									CImages::IconFolder,
									-1,
									new CDirectoryItemData(normalizedPath));
				items.emplace(p, parent);
			}
		}
	};

	for (auto& entry : entries)
	{
		if (!entry.IsFile)
		{
			addDirectoryToTree(entry.Path);
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
