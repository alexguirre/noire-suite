#include "FileTreeCtrl.h"
#include "Identifiers.h"
#include "Images.h"
#include <filesystem>
#include <gsl/gsl>
#include <wx/icon.h>
#include <wx/imaglist.h>
#include <wx/msgdlg.h>
#include <wx/wx.h>

// clang-format off
wxBEGIN_EVENT_TABLE(CFileTreeCtrl, wxTreeCtrl)
	// for context menu
	EVT_TREE_ITEM_MENU(wxID_ANY, CFileTreeCtrl::OnItemContextMenu)
wxEND_EVENT_TABLE();
// clang-format on

CFileTreeCtrl::CFileTreeCtrl(wxWindow* parent,
							 const wxWindowID id,
							 const wxPoint& pos,
							 const wxSize& size)
	: wxTreeCtrl(parent, id, pos, size),
	  mFile{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\final\\pc\\out.wad.pc" }
{
	SetImageList(CImages::Icons());
	LoadDummyData();
}

void CFileTreeCtrl::LoadDummyData()
{
	// initialize tree
	wxTreeItemId root = AddRoot("L.A. Noire", CImages::IconNoire);
	wxTreeItemId finalItem = AppendItem(root, "final", CImages::IconFolder);
	wxTreeItemId pcItem = AppendItem(finalItem, "pc", CImages::IconFolder);
	wxTreeItemId wadItem = AppendItem(pcItem, "out.wad.pc", CImages::IconBlueFolder);

	using namespace noire;

	const std::function<void(const WADChildDirectory&, const wxTreeItemId&)> addDirectoryToTree =
		[this, &addDirectoryToTree](const WADChildDirectory& root, const wxTreeItemId& treeParent) {
			for (auto& d : root.Directories())
			{
				wxTreeItemId item = AppendItem(treeParent,
											   d.Name(),
											   CImages::IconFolder,
											   -1,
											   new CDirectoryItemData(d));
				addDirectoryToTree(d, item);
			}
		};

	addDirectoryToTree(mFile.Root(), wadItem);
}

void CFileTreeCtrl::OnItemContextMenu(wxTreeEvent& event)
{
	wxTreeItemId itemId = event.GetItem();
	Expects(itemId.IsOk());

	ShowItemContextMenu(itemId, event.GetPoint());
	event.Skip();
}

void CFileTreeCtrl::ShowItemContextMenu(wxTreeItemId, const wxPoint&)
{
	// nothing
}
