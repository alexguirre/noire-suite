#include "DirectoryContentsListCtrl.h"
#include "Identifiers.h"
#include "Images.h"
#include "util/Format.h"
#include "windows/ImageWindow.h"
#include <IL/il.h>
#include <cstdlib>
#include <cstring>
#include <formats/WADFile.h>
#include <gsl/gsl>
#include <wx/msgdlg.h>
#include <wx/wx.h>

using namespace noire::fs;

CDirectoryEvent::CDirectoryEvent() : wxEvent(wxID_ANY, EVT_DIRECTORY_CHANGED) {}
CDirectoryEvent::CDirectoryEvent(std::string_view dirPath, int winId)
	: wxEvent(winId, EVT_DIRECTORY_CHANGED)
{
	SetDirectory(dirPath);
}

CDirectoryEvent::CDirectoryEvent(const CDirectoryEvent& e) : wxEvent(e)
{
	SetDirectory(e.GetDirectory());
}

wxIMPLEMENT_DYNAMIC_CLASS(CDirectoryEvent, wxEvent);
wxDEFINE_EVENT(EVT_DIRECTORY_CHANGED, CDirectoryEvent);

// clang-format off
wxBEGIN_EVENT_TABLE(CDirectoryContentsListCtrl, wxListCtrl)
	// for context menu
	EVT_LIST_ITEM_RIGHT_CLICK(wxID_ANY, CDirectoryContentsListCtrl::OnItemContextMenu)
	EVT_LIST_ITEM_ACTIVATED(wxID_ANY, CDirectoryContentsListCtrl::OnItemActivated)
wxEND_EVENT_TABLE();
// clang-format on

struct SDirectoryItemData
{
	std::string Path;
	EDirectoryEntryType Type;

	SDirectoryItemData(std::string_view path, EDirectoryEntryType type) : Path{ path }, Type{ type }
	{
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
	: wxListCtrl(parent, id, pos, size, wxLC_REPORT), mFileSystem{ nullptr }, mDirPath{}
{
	SetImageList(CImages::Icons(), wxIMAGE_LIST_SMALL);
	BuildColumns();
}

CDirectoryContentsListCtrl::~CDirectoryContentsListCtrl()
{
	DeleteAllItemsData();
}

void CDirectoryContentsListCtrl::SetFileSystem(CFileSystem* fileSystem)
{
	if (fileSystem != mFileSystem)
	{
		mFileSystem = fileSystem;
		mDirPath = "";
		SetDirectory("/");
	}
}

void CDirectoryContentsListCtrl::SetDirectory(std::string_view dirPath)
{
	if (mDirPath != dirPath)
	{
		mDirPath = dirPath;
		UpdateContents();

		CDirectoryEvent event{ mDirPath };
		event.SetEventObject(this);
		wxPostEvent(wxGetTopLevelParent(this)->GetEventHandler(), event);
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

	std::string_view path = data->Path;
	if (data->Type == EDirectoryEntryType::File)
	{
		OpenFile(path);
	}
	else
	{
		SetDirectory(path);
	}

	event.Skip();
}

void CDirectoryContentsListCtrl::ShowItemContextMenu()
{
	//PopupMenu(mItemContextMenu.get());
}

void CDirectoryContentsListCtrl::BuildColumns()
{
	AppendColumn("Name", wxLIST_FORMAT_LEFT, 250);
	AppendColumn("Type");
	AppendColumn("Size", wxLIST_FORMAT_RIGHT);
}

void CDirectoryContentsListCtrl::UpdateContents()
{
	DeleteAllItemsData();
	DeleteAllItems();

	if (mFileSystem == nullptr || mDirPath.empty())
	{
		return;
	}

	Freeze();
	SetCursor(wxCursor{ wxCURSOR_ARROWWAIT });

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

		int icon = -1;
		switch (data->Type)
		{
		case EDirectoryEntryType::Directory: icon = CImages ::IconFolder; break;
		case EDirectoryEntryType::Collection: icon = CImages ::IconBlueFolder; break;
		default:
		case EDirectoryEntryType::File: icon = CImages::IconBlankFile; break;
		}
		SetItemImage(idx, icon);
	};

	const auto getNameFromPath = [](std::string_view path) {
		std::size_t namePos = path.rfind(CFileSystem::DirectorySeparator, path.size() - 2);
		if (namePos != std::string_view::npos)
		{
			path = path.substr(namePos + 1);
			return path[path.size() - 1] == CFileSystem::DirectorySeparator ?
					   path.substr(0, path.size() - 1) :
					   path;
		}
		else
		{
			return path;
		}
	};

	auto entries = mFileSystem->GetEntries(mDirPath);
	// add directories first
	for (auto& d : entries)
	{
		if (d.Type != EDirectoryEntryType::File)
		{
			std::string_view nameView = getNameFromPath(d.Path);

			wxString name{ nameView.data(), nameView.size() };
			wxString type{ "Folder" };
			wxString size{ d.Type == EDirectoryEntryType::Collection ?
							   BytesToHumanReadable(mFileSystem->FileSize(
								   std::string_view{ d.Path.c_str(), d.Path.size() - 1 })) :
							   "TBD" };
			// size.Printf("%u items", d.Files().size() + d.Directories().size());

			addItem(name, type, size, new SDirectoryItemData(d.Path, d.Type));
		}
	}

	// add files
	for (auto& f : entries)
	{
		if (f.Type == EDirectoryEntryType::File)
		{
			std::string_view nameView = getNameFromPath(f.Path);

			wxString name{ nameView.data(), nameView.size() };
			wxString type{ "File" };
			wxString size{ BytesToHumanReadable(mFileSystem->FileSize(f.Path)) };

			addItem(name, type, size, new SDirectoryItemData(f.Path, f.Type));
		}
	}

	Thaw();
	SetCursor(wxCursor{ wxCURSOR_DEFAULT });
}

void CDirectoryContentsListCtrl::DeleteAllItemsData()
{
	for (int i = 0; i < GetItemCount(); i++)
	{
		SDirectoryItemData* d = reinterpret_cast<SDirectoryItemData*>(GetItemData(i));
		if (d)
		{
			delete d;
		}
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

void CDirectoryContentsListCtrl::OpenFile(std::string_view filePath)
{
	std::uint32_t headerMagic;
	std::unique_ptr<IFileStream> file = mFileSystem->OpenFile(filePath);
	file->Read(&headerMagic, sizeof(headerMagic));

	if (headerMagic == 0x20534444) // == 'DDS '
	{
		const std::size_t size = file->Size();

		std::unique_ptr<std::byte[]> buffer = std::make_unique<std::byte[]>(size);
		file->Seek(0);
		file->Read(buffer.get(), size);

		const wxImage img = CreateImageFromDDS({ buffer.get(), gsl::narrow<std::ptrdiff_t>(size) });
		CImageWindow* imgWin =
			new CImageWindow(this, wxID_ANY, { filePath.data(), filePath.size() }, img);
		imgWin->Show();
	}
}