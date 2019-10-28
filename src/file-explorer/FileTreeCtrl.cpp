#include "FileTreeCtrl.h"
#include "Identifiers.h"
#include "Images.h"
#include <filesystem>
#include <formats/WADFile.h>
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

class CFileItemData : public wxTreeItemData
{
public:
	CFileItemData(const noire::WADRawFileEntry& entry) : mEntry{ entry } {}

	const noire::WADRawFileEntry& FileEntry() const { return mEntry; }

private:
	noire::WADRawFileEntry mEntry;
};

class CFileItemContextMenu : public wxMenu
{
public:
	CFileItemContextMenu(CFileItemData* data) : wxMenu(), mData{ data }
	{
		Expects(data != nullptr);

		Append(FileTreeExportMenuId, "Export");

		Connect(wxEVT_COMMAND_MENU_SELECTED,
				wxCommandEventHandler(CFileItemContextMenu::OnExport),
				nullptr,
				this);
	}

private:
	CFileItemData* mData;

	void OnExport(wxCommandEvent&)
	{
		// TODO: implement file Export
		const noire::WADRawFileEntry& entry = mData->FileEntry();
		wxMessageBox("Exporting " + entry.Path);
	}
};

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
				wxTreeItemId item = AppendItem(treeParent, d.Name(), CImages::IconFolder);
				addDirectoryToTree(d, item);
			}
			for (auto& f : root.Files())
			{
				auto name = f.Name();
				AppendItem(treeParent,
						   wxString{ name.data(), name.data() + name.size() },
						   CImages::IconBlankFile,
						   -1,
						   new CFileItemData(mFile.Entries()[f.EntryIndex()]));
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

void CFileTreeCtrl::ShowItemContextMenu(wxTreeItemId id, const wxPoint& pos)
{
	CFileItemData* data = reinterpret_cast<CFileItemData*>(GetItemData(id));
	if (data)
	{
		CFileItemContextMenu menu{ data };
		PopupMenu(&menu, pos);
	}
}
