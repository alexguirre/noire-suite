#include "DirectoryContentsListCtrl.h"
#include "Identifiers.h"
#include "Images.h"
#include "windows/ImageWindow.h"
#include <IL/il.h>
#include <cstdlib>
#include <cstring>
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

static wxImage CreateImageFromDDS(gsl::span<std::byte> ddsData)
{
	ILuint imgId = ilGenImage();
	ilBindImage(imgId);

	ilLoadL(IL_DDS, ddsData.data(), ddsData.size_bytes());

	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

	ILint width = ilGetInteger(IL_IMAGE_WIDTH);
	ILint height = ilGetInteger(IL_IMAGE_HEIGHT);
	ILubyte* data = ilGetData();
	ILint dataSize = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);
	// copies allocated with malloc since wxImage will take ownership of them
	ILubyte* rgbCopy = reinterpret_cast<ILubyte*>(std::malloc(width * height * 3));
	ILubyte* alphaCopy = reinterpret_cast<ILubyte*>(std::malloc(width * height * 1));

	Expects(dataSize == (width * height * 4));

	for (ILint y = 0; y < height; y++)
	{
		for (ILint x = 0; x < width; x++)
		{
			const std::size_t dataOffset = (x + y * width) * 4;

			ILubyte r = data[dataOffset + 0];
			ILubyte g = data[dataOffset + 1];
			ILubyte b = data[dataOffset + 2];
			ILubyte a = data[dataOffset + 3];

			const std::size_t rgbOffset = (x + y * width) * 3;
			rgbCopy[rgbOffset + 0] = r;
			rgbCopy[rgbOffset + 1] = g;
			rgbCopy[rgbOffset + 2] = b;

			const std::size_t alphaOffset = (x + y * width) * 1;
			alphaCopy[alphaOffset + 0] = a;
		}
	}

	// wxImage takes ownership of rgbCopy and alphaCopy
	wxImage img{ width, height, rgbCopy, alphaCopy };

	ilDeleteImage(imgId);
	return img;
}

void CDirectoryContentsListCtrl::OpenFile(const noire::WADChildFile& file)
{
	std::uint32_t headerMagic;
	const noire::WADRawFileEntry& entry = file.Owner().Entries()[file.EntryIndex()];
	const std::size_t offset = entry.Offset;
	file.Owner().Read(offset, { reinterpret_cast<std::byte*>(&headerMagic), sizeof(headerMagic) });

	if (headerMagic == 0x20534444) // == 'DDS '
	{
		const std::string& path = entry.Path;
		const std::size_t size = entry.Size;

		std::unique_ptr<std::byte[]> buffer = std::make_unique<std::byte[]>(size);
		gsl::span<std::byte> bufferSpan{ buffer.get(), gsl::narrow<std::ptrdiff_t>(size) };
		file.Owner().Read(offset, bufferSpan);

		const wxImage img = CreateImageFromDDS(bufferSpan);
		CImageWindow* imgWin = new CImageWindow(this, wxID_ANY, path, img);
		imgWin->Show();
	}
}