#include "FileTreeCtrl.h"
#include "resources/BlankFileIcon.xpm"
#include "resources/BlueFolderIcon.xpm"
#include "resources/FolderIcon.xpm"
#include "resources/NoireIcon.xpm"
#include <filesystem>
#include <formats/WADFile.h>
#include <wx/icon.h>
#include <wx/imaglist.h>

CFileTreeCtrl::CFileTreeCtrl(wxWindow* parent,
							 const wxWindowID id,
							 const wxPoint& pos,
							 const wxSize& size)
	: wxTreeCtrl(parent, id, pos, size), mIconIds{}
{
	LoadIcons();
	LoadDummyData();
}

void CFileTreeCtrl::LoadIcons()
{
	wxImageList* iconList = new wxImageList(17, 17);
	// clang-format off
	wxIcon icons[IconCount];
	icons[IconBlankFile]  = { BlankFileIcon };
	icons[IconBlueFolder] = { BlueFolderIcon };
	icons[IconFolder]     = { FolderIcon };
	icons[IconNoire]      = { NoireIcon };
	// clang-format on

	for (std::size_t i = 0; i < IconCount; i++)
	{
		mIconIds[i] = iconList->Add(icons[i]);
	}

	AssignImageList(iconList);
}

void CFileTreeCtrl::LoadDummyData()
{
	// initialize tree
	wxTreeItemId root = AddRoot("L.A. Noire", mIconIds[IconNoire]);
	wxTreeItemId finalItem = AppendItem(root, "final", mIconIds[IconFolder]);
	wxTreeItemId pcItem = AppendItem(finalItem, "pc", mIconIds[IconFolder]);
	wxTreeItemId wadItem = AppendItem(pcItem, "out.wad.pc", mIconIds[IconBlueFolder]);

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
				wxTreeItemId item = AppendItem(treeParent, d.Name(), mIconIds[IconFolder]);
				addDirectoryToTree(d, item);
			}
			for (auto& f : root.Files())
			{
				auto name = f.Name();
				AppendItem(treeParent,
						   wxString{ name.data(), name.data() + name.size() },
						   mIconIds[IconBlankFile]);
			}
		};

	addDirectoryToTree(file.Root(), wadItem);
}
