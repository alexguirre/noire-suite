#include "MainWindow.h"
#include <filesystem>
#include <formats/WADFile.h>
#include <functional>

CMainWindow::CMainWindow()
	: wxFrame(nullptr, wxID_ANY, "noire-suite - File Explorer"),
	  mFileTreeCtrl{ new wxTreeCtrl(this, wxID_ANY, { 5, 5 }, { 150, 800 }) }
{
	wxTreeItemId root = mFileTreeCtrl->AddRoot("L.A. Noire");
	wxTreeItemId finalItem = mFileTreeCtrl->AppendItem(root, "final");
	wxTreeItemId pcItem = mFileTreeCtrl->AppendItem(finalItem, "pc");
	wxTreeItemId wadItem = mFileTreeCtrl->AppendItem(pcItem, "out.wad.pc");

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
				wxTreeItemId item = mFileTreeCtrl->AppendItem(treeParent, d.Name());
				addDirectoryToTree(d, item);
			}
			for (auto& f : root.Files())
			{
				auto name = f.Name();
				mFileTreeCtrl->AppendItem(treeParent,
										  wxString{ name.data(), name.data() + name.size() });
			}
		};

	addDirectoryToTree(file.Root(), wadItem);
}