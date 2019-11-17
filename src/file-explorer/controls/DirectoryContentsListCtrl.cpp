#include "DirectoryContentsListCtrl.h"
#include "Identifiers.h"
#include "Images.h"
#include "util/Format.h"
#include "util/VirtualFileDataObject.h"
#include "windows/ImageWindow.h"
#include <IL/il.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <formats/Hash.h>
#include <formats/WADFile.h>
#include <formats/fs/NativeDevice.h>
#include <gsl/gsl>
#include <vector>
#include <wx/clipbrd.h>
#include <wx/dnd.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/wx.h>

using namespace noire::fs;

CDirectoryEvent::CDirectoryEvent() : wxEvent(wxID_ANY, EVT_DIRECTORY_CHANGED) {}
CDirectoryEvent::CDirectoryEvent(noire::fs::SPathView dirPath, int winId)
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
	SPath Path;
	EDirectoryEntryType Type;

	SDirectoryItemData(SPathView path, EDirectoryEntryType type) : Path{ path }, Type{ type } {}
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

	Bind(wxEVT_LIST_BEGIN_DRAG, &CDirectoryContentsListCtrl::OnBeginDrag, this);
	Bind(wxEVT_MENU, &CDirectoryContentsListCtrl::OnCopy, this, wxID_COPY);

	// wxID_COPY
	std::array<wxAcceleratorEntry, 1> entries{};
	entries[0].Set(wxACCEL_CTRL, 'C', wxID_COPY);

	SetAcceleratorTable({ entries.size(), entries.data() });
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
		mDirPath = SPathView{};
		SetDirectory({ "/" });
	}
}

void CDirectoryContentsListCtrl::SetDirectory(noire::fs::SPathView dirPath)
{
	Expects(dirPath.IsDirectory());

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

	SPathView path = data->Path;
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

void CDirectoryContentsListCtrl::OnBeginDrag(wxListEvent& event)
{
	wxLogDebug(__FUNCTION__);

	if (std::unique_ptr<wxDataObject> dataObj{ CreateSelectedFilesDataObject() }; dataObj)
	{
		wxDropSource src{ *dataObj, this };
		src.DoDragDrop(wxDrag_CopyOnly);
	}
	event.Skip();
}

void CDirectoryContentsListCtrl::OnCopy(wxCommandEvent& event)
{
	wxLogDebug(__FUNCTION__);

	if (wxDataObject* dataObj = CreateSelectedFilesDataObject(); dataObj && wxTheClipboard->Open())
	{
		// wxTheClipboard takes ownership of the wxDataObject so don't delete it
		wxTheClipboard->SetData(dataObj);
		wxTheClipboard->Close();
	}
	event.Skip();
}

void CDirectoryContentsListCtrl::ShowItemContextMenu()
{
	wxMenu menu{};
	menu.Append(wxID_COPY);

	PopupMenu(&menu);
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

	if (mFileSystem == nullptr || mDirPath.IsEmpty())
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

	auto entries = mFileSystem->GetEntries(mDirPath);
	// add directories first
	for (auto& d : entries)
	{
		if (d.Type != EDirectoryEntryType::File)
		{
			std::string_view nameView = d.Path.Name();

			wxString name{ nameView.data(), nameView.size() };
			wxString type{ "Folder" };
			wxString size{
				d.Type == EDirectoryEntryType::Collection ?
					// remove last '/' and pass it to FileSize
					BytesToHumanReadable(mFileSystem->FileSize(
						std::string_view{ d.Path.String().data(), d.Path.String().size() - 1 })) :
					"TBD"
			};
			// size.Printf("%u items", d.Files().size() + d.Directories().size());

			addItem(name, type, size, new SDirectoryItemData(d.Path, d.Type));
		}
	}

	// add files
	for (auto& f : entries)
	{
		if (f.Type == EDirectoryEntryType::File)
		{
			std::string_view nameView = f.Path.Name();

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
	// ILint dataSize = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);
	// copies allocated with malloc since wxImage will take ownership of them
	ILubyte* rgbCopy = reinterpret_cast<ILubyte*>(std::malloc(width * height * 3));
	ILubyte* alphaCopy = reinterpret_cast<ILubyte*>(std::malloc(width * height * 1));

	// TODO: figure out why with `/final/pc/out.wad.pc/out/graphicsdata/dirt.dds` this precondition
	// isn't met
	// Expects(dataSize == (width * height * 4));

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

void CDirectoryContentsListCtrl::OpenFile(SPathView filePath)
{
	if (filePath.Name() == "uniquetexturevram")
	{
		// TODO: move this loading somewhere else, possibly to noire-formats
		// TODO: show all textures in the same window
		SPath mainPath = filePath.Parent() / "uniquetexturemain";
		Expects(mFileSystem->FileExists(mainPath)); // TODO: 'uniquetexturemain' may not exist along
													// with a 'uniquetexturevram' file
		std::unique_ptr<IFileStream> mainStream = mFileSystem->OpenFile(mainPath);
		std::unique_ptr<IFileStream> vramStream = mFileSystem->OpenFile(filePath);
		const std::size_t vramStreamSize = vramStream->Size();

		mainStream->Read<std::uint32_t>(); // these 4 bytes are used by the game at runtime to
										   // indicate if it already loaded the texture, the file
										   // should always have 0 here

		// read entries
		const std::uint32_t textureCount = mainStream->Read<std::uint32_t>();
		struct TextureEntry
		{
			std::uint32_t Offset;
			std::uint32_t UnkZero;
			std::uint32_t NameHash;
		};
		std::vector<TextureEntry> entries{};
		entries.reserve(textureCount);
		for (std::size_t i = 0; i < textureCount; i++)
		{
			entries.emplace_back(mainStream->Read<TextureEntry>());
		}

		// open a window for each texture
		for (std::size_t i = 0; i < entries.size(); i++)
		{
			const TextureEntry& e = entries[i];
			// this expects the entries to be sorted by offset, not sure if that is always the case
			const std::size_t size = (i < (entries.size() - 1)) ?
										 (entries[i + 1].Offset - e.Offset) :
										 (vramStreamSize - e.Offset);
			std::unique_ptr<std::byte[]> buffer = std::make_unique<std::byte[]>(size);
			vramStream->Seek(e.Offset);
			vramStream->Read(buffer.get(), size);

			const wxImage img =
				CreateImageFromDDS({ buffer.get(), gsl::narrow<std::ptrdiff_t>(size) });

			std::string title = noire::CHashDatabase::Instance().GetString(e.NameHash);
			title += " | " + std::string{ filePath.String() };
			CImageWindow* imgWin = new CImageWindow(this, wxID_ANY, title, img);
			imgWin->Show();
		}
	}
	else
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

			const wxImage img =
				CreateImageFromDDS({ buffer.get(), gsl::narrow<std::ptrdiff_t>(size) });
			CImageWindow* imgWin =
				new CImageWindow(this,
								 wxID_ANY,
								 { filePath.String().data(), filePath.String().size() },
								 img);
			imgWin->Show();
		}
	}
}

wxDataObject* CDirectoryContentsListCtrl::CreateSelectedFilesDataObject() const
{
	wxLogDebug(__FUNCTION__);

	std::vector<SDirectoryItemData*> datas{};
	long item = -1;
	while ((item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND)
	{
		wxLogDebug("> '%s'", GetItemText(item));
		SDirectoryItemData* data = reinterpret_cast<SDirectoryItemData*>(GetItemData(item));
		if (data && data->Type == EDirectoryEntryType::File)
		{
			datas.push_back(data);
		}
	}

	if (datas.size() == 0)
	{
		return nullptr;
	}

	// get device based on the path of the first selected item because all selected items should
	// have the same device since they are in the same directory
	IDevice* device = mFileSystem->FindDevice(datas[0]->Path);
	if (device == mFileSystem->FindDevice("/"))
	{
		wxFileDataObject* dataObj = new wxFileDataObject;
		const std::filesystem::path& rootPath =
			reinterpret_cast<noire::fs::CNativeDevice*>(device)->RootDirectory();
		for (SDirectoryItemData* d : datas)
		{
			SPathView relPath = d->Path.RelativeTo("/");
			std::filesystem::path fullPath = rootPath / relPath.String();

			dataObj->AddFile(fullPath.c_str());
		}

		return dataObj;
	}
	else
	{
		std::vector<SPathView> paths{};
		paths.reserve(datas.size());
		std::transform(datas.begin(),
					   datas.end(),
					   std::back_inserter(paths),
					   [](SDirectoryItemData* d) { return SPathView{ d->Path }; });

		return new CVirtualFileDataObject{ mFileSystem, paths };
	}
}
