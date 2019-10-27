#include "MainWindow.h"
#include "resources/BlankFileIcon.xpm"
#include "resources/FolderIcon.xpm"
#include "resources/NoireIcon.xpm"
#include <filesystem>
#include <formats/WADFile.h>
#include <functional>
#include <wx/icon.h>

CMainWindow::CMainWindow()
	: wxFrame(nullptr, wxID_ANY, "noire-suite - File Explorer"),
	  mFileTreeCtrl{ new wxTreeCtrl(this, wxID_ANY, { 5, 5 }, { 150, 800 }) }
{
	// add icons
	{
		wxImageList* iconList = new wxImageList(17, 17);
		// clang-format off
		std::pair<int*, wxIcon> icons[]{
			{ &mFileTreeNoireIcon,     { NoireIcon }     },
			{ &mFileTreeFolderIcon,    { FolderIcon }    },
			{ &mFileTreeBlankFileIcon, { BlankFileIcon } },
		};
		// clang-format on

		for (auto& i : icons)
		{
			*i.first = iconList->Add(i.second);
		}

		mFileTreeCtrl->AssignImageList(iconList);
	}

	// initialize tree
	wxTreeItemId root = mFileTreeCtrl->AddRoot("L.A. Noire", mFileTreeNoireIcon);
	wxTreeItemId finalItem = mFileTreeCtrl->AppendItem(root, "final", mFileTreeFolderIcon);
	wxTreeItemId pcItem = mFileTreeCtrl->AppendItem(finalItem, "pc", mFileTreeFolderIcon);
	wxTreeItemId wadItem = mFileTreeCtrl->AppendItem(pcItem, "out.wad.pc", mFileTreeFolderIcon);

	// hardcoded temporarily
	std::filesystem::path wadPath{
		"E:\\Rockstar Games\\L.A. Noire Complete Edition\\final\\pc\\out.wad.pc"
	};

	using namespace noire;

	WADFile file{ wadPath };
	const std::function<void(const WADChildDirectory&, const wxTreeItemId&)> addDirectoryToTree =
		[this, &addDirectoryToTree](const WADChildDirectory& root, const wxTreeItemId& treeParent) {
			for (auto& d : root.Directories())
			{
				wxTreeItemId item =
					mFileTreeCtrl->AppendItem(treeParent, d.Name(), mFileTreeFolderIcon);
				addDirectoryToTree(d, item);
			}
			for (auto& f : root.Files())
			{
				auto name = f.Name();
				mFileTreeCtrl->AppendItem(treeParent,
										  wxString{ name.data(), name.data() + name.size() },
										  mFileTreeBlankFileIcon);
			}
		};

	addDirectoryToTree(file.Root(), wadItem);
}