#include "DirectoryContentsListCtrl.h"
#include "Identifiers.h"
#include "resources/BlankFileIcon.xpm"
#include "resources/BlueFolderIcon.xpm"
#include "resources/FolderIcon.xpm"
#include "resources/NoireIcon.xpm"
#include <formats/WADFile.h>
#include <gsl/gsl>
#include <wx/msgdlg.h>
#include <wx/wx.h>

// clang-format off
wxBEGIN_EVENT_TABLE(CDirectoryContentsListCtrl, wxListCtrl)
	// for context menu
	EVT_LIST_ITEM_RIGHT_CLICK(wxID_ANY, CDirectoryContentsListCtrl::OnItemContextMenu)
	EVT_LIST_ITEM_ACTIVATED(wxID_ANY, CDirectoryContentsListCtrl::OnItemActivated)
wxEND_EVENT_TABLE();
// clang-format on

struct SDirectoryItemData
{
	enum class eType
	{
		Unknown = 0,
		File = 1,
		Directory = 2,
	};

	union
	{
		const noire::WADChildFile* File;
		const noire::WADChildDirectory* Directory;
	} Content;
	eType Type;

	SDirectoryItemData(const noire::WADChildFile* file) : Content{ file }, Type{ eType::File } {}
	SDirectoryItemData(const noire::WADChildDirectory* dir) : Type{ eType::Directory }
	{
		Content.Directory = dir;
	}
};

class CDirectoryItemContextMenu : public wxMenu
{
public:
	CDirectoryItemContextMenu() : wxMenu()
	{
		Append(DirectoryContentsExportMenuId, "Export");

		Connect(wxEVT_COMMAND_MENU_SELECTED,
				wxCommandEventHandler(CDirectoryItemContextMenu::OnExport),
				nullptr,
				this);
	}

private:
	void OnExport(wxCommandEvent&)
	{
		// TODO: implement file Export
		wxMessageBox("Exporting");
	}
};

CDirectoryContentsListCtrl::CDirectoryContentsListCtrl(wxWindow* parent,
													   const wxWindowID id,
													   const wxPoint& pos,
													   const wxSize& size)
	: wxListCtrl(parent, id, pos, size, wxLC_REPORT),
	  mCurrentDirectory{ nullptr },
	  mItemContextMenu{ std::make_unique<CDirectoryItemContextMenu>() }
{
	LoadIcons();
	BuildColumns();
}

void CDirectoryContentsListCtrl::LoadIcons()
{
	// TODO: share same wxImageList with CDirectoryContentsListCtrl and CFileTreeCtrl
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
		iconList->Add(icons[i]);
	}

	AssignImageList(iconList, wxIMAGE_LIST_SMALL);
}

void CDirectoryContentsListCtrl::SetDirectory(const noire::WADChildDirectory& dir)
{
	if (mCurrentDirectory != std::addressof(dir))
	{
		mCurrentDirectory = std::addressof(dir);
		UpdateContents();
	}
}

void CDirectoryContentsListCtrl::OnItemContextMenu(wxListEvent& event)
{
	ShowItemContextMenu();
	event.Skip();
}

void CDirectoryContentsListCtrl::OnItemActivated(wxListEvent& event)
{
	SDirectoryItemData* data = reinterpret_cast<SDirectoryItemData*>(event.GetItem().GetData());
	Expects(data != nullptr);

	if (data->Type == SDirectoryItemData::eType::Directory)
	{
		const noire::WADChildDirectory& dir = *data->Content.Directory;
		SetDirectory(dir);
	}

	event.Skip();
}

void CDirectoryContentsListCtrl::ShowItemContextMenu()
{
	PopupMenu(mItemContextMenu.get());
}

void CDirectoryContentsListCtrl::BuildColumns()
{
	AppendColumn("Name");
	AppendColumn("Type");
	AppendColumn("Size");
}

void CDirectoryContentsListCtrl::UpdateContents()
{
	for (int i = 0; i < GetItemCount(); i++)
	{
		SDirectoryItemData* d = reinterpret_cast<SDirectoryItemData*>(GetItemData(i));
		delete d;
	}
	DeleteAllItems();

	if (!mCurrentDirectory)
	{
		return;
	}

	long itemId = 0;
	const auto addItem = [this, &itemId](const wxString& name,
										 const wxString& type,
										 const wxString& size,
										 SDirectoryItemData* data) {
		wxListItem item{};
		item.SetId(itemId++);
		item.SetText(name);

		long idx = InsertItem(item);
		SetItem(idx, 1, type);
		SetItem(idx, 2, size);

		SetItemPtrData(idx, reinterpret_cast<wxUIntPtr>(data));

		SetItemImage(idx,
					 data->Type == SDirectoryItemData::eType::Directory ? IconFolder :
																		  IconBlankFile);
	};

	for (auto& d : mCurrentDirectory->Directories())
	{
		wxString name{ d.Name() };
		wxString type{ "Folder" };
		wxString size{};
		size.Printf("%u items", d.Files().size() + d.Directories().size());

		addItem(name, type, size, new SDirectoryItemData(&d));
	}

	for (auto& f : mCurrentDirectory->Files())
	{
		wxString name{ f.Name().data(), f.Name().data() + f.Name().size() };
		wxString type{ "File" };
		wxString size{};
		size.Printf("%u bytes", f.Owner().Entries()[f.EntryIndex()].Size);

		addItem(name, type, size, new SDirectoryItemData(&f));
	}
}
