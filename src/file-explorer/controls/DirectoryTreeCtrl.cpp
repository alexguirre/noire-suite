#include "DirectoryTreeCtrl.h"
#include "App.h"
#include "Identifiers.h"
#include "Images.h"
#include <core/devices/Device.h>
#include <string_view>
#include <unordered_map>
#include <wx/icon.h>
#include <wx/imaglist.h>
#include <wx/msgdlg.h>
#include <wx/wx.h>

namespace noire::explorer
{
	static constexpr long TreeCtrlStyle = wxTR_HAS_BUTTONS | wxTR_TWIST_BUTTONS | wxTR_NO_LINES |
										  wxTR_HIDE_ROOT | wxTR_LINES_AT_ROOT | wxTR_SINGLE;

	DirectoryTreeCtrl::DirectoryTreeCtrl(wxWindow* parent,
										 const wxWindowID id,
										 const wxPoint& pos,
										 const wxSize& size)
		: wxTreeCtrl(parent, id, pos, size, TreeCtrlStyle)
	{
		SetImageList(Images::Icons());
	}

	void DirectoryTreeCtrl::Refresh()
	{
		this->DeleteAllItems();

		Device* dev = wxGetApp().RootDevice();
		if (dev == nullptr)
		{
			return;
		}

		// initialize tree
		wxTreeItemId root = AddRoot("ROOT");
		wxTreeItemId noireItem =
			AppendItem(root, "L.A. Noire", Images::IconNoire, -1, new DirectoryItemData("/"));

		std::unordered_map<size, wxTreeItemId> items{};

		// get the path of each directory in the input path in order
		// for example "/some/deep/folder/" returns { "/some/deep/folder/", "/some/deep/", "/some/"
		// }
		const auto getDirectoriesFromPath = [](PathView path, std::vector<PathView>& directories) {
			PathView p = path;
			while (!p.IsEmpty() && !p.IsRoot())
			{
				directories.emplace_back(p);
				p = p.Parent();
			}
		};

		const auto addDirectoryToTree = [&](PathView path) {
			PathView origPath = path;

			std::vector<PathView> dirs{};
			getDirectoriesFromPath(path, dirs);

			wxTreeItemId parent = noireItem;
			// iterate in reverse order so top-most parent is processed first
			for (auto dirIt = dirs.rbegin(); dirIt != dirs.rend(); dirIt++)
			{
				const PathView p = *dirIt;
				const size pathHash = std::hash<PathView>{}(p);

				if (auto it = items.find(pathHash); it != items.end())
				{
					// already added
					parent = it->second;
				}
				else
				{
					std::string_view name = p.Name();
					parent = AppendItem(parent,
										{ name.data(), name.size() },
										Images::IconFolder,
										-1,
										new DirectoryItemData(p));
					items.emplace(pathHash, parent);
				}
			}
		};

		dev->Visit([&addDirectoryToTree](PathView p) { addDirectoryToTree(p); },
				   [](PathView) {},
				   PathView::Root,
				   true);

		const std::function<void(const wxTreeItemId&)> sortRecursively =
			[this, &sortRecursively](const wxTreeItemId& item) {
				SortChildren(item); // sort in ascending case-sensitive alphabetical order
				wxTreeItemIdValue cookie;
				for (wxTreeItemId c = GetFirstChild(item, cookie); c.IsOk();
					 c = GetNextChild(item, cookie))
				{
					sortRecursively(c);
				}
			};

		sortRecursively(root);
	}
}
