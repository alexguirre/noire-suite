#include "DirectoryContentsListCtrl.h"
#include "Identifiers.h"
#include "Images.h"
#include "windows/ImageWindow.h"
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
	SetImageList(CImages::Icons(), wxIMAGE_LIST_SMALL);
	BuildColumns();
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
	else if (data->Type == SDirectoryItemData::eType::File)
	{
		const noire::WADChildFile& file = *data->Content.File;
		OpenFile(file);
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
					 data->Type == SDirectoryItemData::eType::Directory ? CImages::IconFolder :
																		  CImages::IconBlankFile);
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

void CDirectoryContentsListCtrl::OpenFile(const noire::WADChildFile& file)
{
	std::uint32_t headerMagic;
	const std::size_t offset = file.Owner().Entries()[file.EntryIndex()].Offset;
	file.Owner().Read(offset, { reinterpret_cast<std::byte*>(&headerMagic), sizeof(headerMagic) });

	if (headerMagic == 0x20534444) // == 'DDS '
	{
		const std::string& path = file.Owner().Entries()[file.EntryIndex()].Path;
		CImageWindow* imgWin =
			new CImageWindow(this, wxID_ANY, path, { "E:\\Desktop\\test.bmp", wxBITMAP_TYPE_BMP }); // TODO: get wxImage from .dds file
		imgWin->Show();
	}
}