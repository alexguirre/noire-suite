#include "DirectoryContentsListCtrl.h"
#include "Identifiers.h"
#include "Images.h"
#include "util/Format.h"
#include "util/VirtualFileDataObject.h"
#include "windows/AttributeWindow.h"
#include "windows/ImageWindow.h"
#include "windows/ShaderProgramWindow.h"
#include <IL/il.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <formats/AttributeFile.h>
#include <formats/Hash.h>
#include <formats/ShaderProgramFile.h>
#include <formats/WADFile.h>
#include <formats/fs/NativeDevice.h>
#include <functional>
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
	: wxListCtrl(parent, id, pos, size, wxLC_REPORT | wxLC_VIRTUAL),
	  mFileSystem{ nullptr },
	  mDirPath{},
	  mDirEntries{}
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
	const SDirectoryEntry& entry = mDirEntries[event.GetIndex()];

	SPathView path = entry.Path;
	if (entry.Type == EDirectoryEntryType::File)
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

wxString CDirectoryContentsListCtrl::OnGetItemText(long item, long column) const
{
	constexpr long NameCol{ 0 }, TypeCol{ 1 }, SizeCol{ 2 };

	const SDirectoryEntry& e = mDirEntries[item];
	if (e.Type == EDirectoryEntryType::File)
	{
		switch (column)
		{
		case NameCol:
		{
			std::string_view name = e.Path.Name();
			return { name.data(), name.size() };
		}
		case TypeCol: return "File";
		case SizeCol: return BytesToHumanReadable(mFileSystem->FileSize(e.Path));
		}
	}
	else
	{
		switch (column)
		{
		case NameCol:
		{
			std::string_view name = e.Path.Name();
			return { name.data(), name.size() };
		}
		case TypeCol: return "Folder";
		case SizeCol:
		{
			std::string_view path = e.Path.String();
			return e.Type == EDirectoryEntryType::Collection ?
					   BytesToHumanReadable(
						   // remove last '/' and pass it to FileSize
						   mFileSystem->FileSize(path.substr(0, path.size() - 1))) :
					   "TBD";
		}
		}
	}

	// should never get here
	Expects(false);
}

int CDirectoryContentsListCtrl::OnGetItemImage(long item) const
{
	const SDirectoryEntry& e = mDirEntries[item];
	switch (e.Type)
	{
	case EDirectoryEntryType::Directory: return CImages::IconFolder;
	case EDirectoryEntryType::Collection: return CImages::IconBlueFolder;
	default:
	case EDirectoryEntryType::File: return CImages::IconBlankFile;
	}
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
	if (mFileSystem == nullptr || mDirPath.IsEmpty())
	{
		SetItemCount(0);
		return;
	}

	Freeze();
	SetCursor(wxCursor{ wxCURSOR_ARROWWAIT });

	mDirEntries = mFileSystem->GetEntries(mDirPath);

	// sort the entries so directories appear before files and in alphabetical order
	std::stable_sort(mDirEntries.begin(),
					 mDirEntries.end(),
					 [](const SDirectoryEntry& a, const SDirectoryEntry& b) {
						 return a.Path.Name() < b.Path.Name();
					 });
	std::stable_sort(mDirEntries.begin(),
					 mDirEntries.end(),
					 [](const SDirectoryEntry& a, const SDirectoryEntry& b) {
						 static const auto toOrder = [](EDirectoryEntryType type) -> int {
							 // get the priority for each entry type
							 switch (type)
							 {
							 case EDirectoryEntryType::Directory: return 0;
							 case EDirectoryEntryType::Collection: return 1;
							 default:
							 case EDirectoryEntryType::File: return 2;
							 }
						 };

						 return toOrder(a.Type) < toOrder(b.Type);
					 });

	SetItemCount(mDirEntries.size());
	RefreshItems(0, mDirEntries.size() - 1);

	Thaw();
	SetCursor(wxCursor{ wxCURSOR_DEFAULT });
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
	using namespace noire;

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

		if (TFileTraits<CShaderProgramFile>::IsValid(*file))
		{
			wxString title = "Shader Program - " +
							 wxString{ filePath.String().data(), filePath.String().size() };
			CShaderProgramWindow* shaderWin =
				new CShaderProgramWindow(this,
										 wxID_ANY,
										 title,
										 std::make_unique<CShaderProgramFile>(*file));
			shaderWin->Show();
		}
		else if (TFileTraits<CAttributeFile>::IsValid(*file))
		{
			wxString title =
				"Attribute - " + wxString{ filePath.String().data(), filePath.String().size() };
			CAttributeWindow* atbWin =
				new CAttributeWindow(this,
									 wxID_ANY,
									 title,
									 std::make_unique<CAttributeFile>(*file));
			atbWin->Show();
		}
		else
		{
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
}

wxDataObject* CDirectoryContentsListCtrl::CreateSelectedFilesDataObject() const
{
	wxLogDebug(__FUNCTION__);

	std::vector<const SDirectoryEntry*> entries{};
	long item = -1;
	while ((item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND)
	{
		wxLogDebug("> '%s'", GetItemText(item));
		const SDirectoryEntry* entry = &mDirEntries[item];
		if (entry && entry->Type == EDirectoryEntryType::File)
		{
			entries.push_back(entry);
		}
	}

	if (entries.size() == 0)
	{
		return nullptr;
	}

	// get device based on the path of the first selected item because all selected items should
	// have the same device since they are in the same directory
	IDevice* device = mFileSystem->FindDevice(entries[0]->Path);
	if (device == mFileSystem->FindDevice("/"))
	{
		wxFileDataObject* dataObj = new wxFileDataObject;
		const std::filesystem::path& rootPath =
			reinterpret_cast<noire::fs::CNativeDevice*>(device)->RootDirectory();
		for (const SDirectoryEntry* e : entries)
		{
			SPathView relPath = e->Path.RelativeTo("/");
			std::filesystem::path fullPath = rootPath / relPath.String();

			dataObj->AddFile(fullPath.c_str());
		}

		return dataObj;
	}
	else
	{
		std::vector<SPathView> paths{};
		paths.reserve(entries.size());
		std::transform(entries.begin(),
					   entries.end(),
					   std::back_inserter(paths),
					   [](const SDirectoryEntry* e) { return SPathView{ e->Path }; });

		return new CVirtualFileDataObject{ mFileSystem, paths };
	}
}
