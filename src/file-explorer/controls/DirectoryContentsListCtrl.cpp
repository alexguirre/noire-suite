#include "DirectoryContentsListCtrl.h"
#include "Identifiers.h"
#include "Images.h"
#include "util/Format.h"
#include "util/VirtualFileDataObject.h"
#include <App.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
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

// Constants for the column indices
static constexpr long NameCol{ 0 }, TypeCol{ 1 }, SizeCol{ 2 };

static auto ShowWaitingCursor(wxWindow* w)
{
	w->SetCursor(wxCursor{ wxCURSOR_ARROWWAIT });
	return gsl::finally([w]() { w->SetCursor(wxCursor{ wxCURSOR_DEFAULT }); });
}

CDirectoryContentsListCtrl::CDirectoryContentsListCtrl(wxWindow* parent,
													   const wxWindowID id,
													   const wxPoint& pos,
													   const wxSize& size)
	: wxListCtrl(parent, id, pos, size, wxLC_REPORT | wxLC_VIRTUAL),
	  mDirPath{},
	  mDirEntries{},
	  mColSortDescending{}
{
	SetImageList(CImages::Icons(), wxIMAGE_LIST_SMALL);
	BuildColumns();

	Bind(wxEVT_LIST_BEGIN_DRAG, &CDirectoryContentsListCtrl::OnBeginDrag, this);
	Bind(wxEVT_MENU, &CDirectoryContentsListCtrl::OnCopy, this, wxID_COPY);
	Bind(wxEVT_LIST_COL_CLICK, &CDirectoryContentsListCtrl::OnColClick, this);

	// wxID_COPY
	std::array<wxAcceleratorEntry, 1> entries{};
	entries[0].Set(wxACCEL_CTRL, 'C', wxID_COPY);

	SetAcceleratorTable({ entries.size(), entries.data() });
}

void CDirectoryContentsListCtrl::SetDirectoryToRoot()
{
	mDirPath = SPathView{};
	SetDirectory({ "/" });
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
		wxGetApp().OpenFile(path);
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
		case SizeCol: return BytesToHumanReadable(wxGetApp().FileSystem()->FileSize(e.Path));
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
						   wxGetApp().FileSystem()->FileSize(path.substr(0, path.size() - 1))) :
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

void CDirectoryContentsListCtrl::OnColClick(wxListEvent& e)
{
	wxLogDebug(__FUNCTION__ ": %d", e.GetColumn());

	const auto _ = ShowWaitingCursor(this);

	const long column = e.GetColumn();
	const bool sortAscending = !(mColSortDescending[column] = !mColSortDescending[column]);
	SortContents(column, sortAscending);

	e.Skip();
}

void CDirectoryContentsListCtrl::ShowItemContextMenu()
{
	wxMenu menu{};
	menu.Append(wxID_COPY);

	PopupMenu(&menu);
}

void CDirectoryContentsListCtrl::BuildColumns()
{
	const long nameCol = AppendColumn("Name", wxLIST_FORMAT_LEFT, 250);
	const long typeCol = AppendColumn("Type");
	const long sizeCol = AppendColumn("Size", wxLIST_FORMAT_RIGHT);

	Ensures(nameCol == NameCol);
	Ensures(typeCol == TypeCol);
	Ensures(sizeCol == SizeCol);
}

void CDirectoryContentsListCtrl::UpdateContents()
{
	auto* fs = wxGetApp().FileSystem();
	if (fs == nullptr || mDirPath.IsEmpty())
	{
		SetItemCount(0);
		return;
	}

	Freeze();
	const auto _ = ShowWaitingCursor(this);

	mDirEntries = fs->GetEntries(mDirPath);

	SetItemCount(mDirEntries.size());

	std::fill(mColSortDescending.begin(), mColSortDescending.end(), false);
	SortContents(NameCol, true);

	Thaw();
}

void CDirectoryContentsListCtrl::SortContents(long column, bool ascending)
{
	static const auto typeToOrder = [](EDirectoryEntryType type) -> int {
		// get the priority for each entry type
		switch (type)
		{
		case EDirectoryEntryType::Directory: return 0;
		case EDirectoryEntryType::Collection: return 1;
		default:
		case EDirectoryEntryType::File: return 2;
		}
	};
	static const auto sortByNameAscending = [](std::vector<SDirectoryEntry>& dirEntries) {
		std::stable_sort(dirEntries.begin(),
						 dirEntries.end(),
						 [](const SDirectoryEntry& a, const SDirectoryEntry& b) {
							 return a.Path.Name() < b.Path.Name();
						 });
	};
	static const auto sortByNameDescending = [](std::vector<SDirectoryEntry>& dirEntries) {
		std::stable_sort(dirEntries.begin(),
						 dirEntries.end(),
						 [](const SDirectoryEntry& a, const SDirectoryEntry& b) {
							 return a.Path.Name() > b.Path.Name();
						 });
	};
	static const auto sortByTypeAscending = [](std::vector<SDirectoryEntry>& dirEntries) {
		std::stable_sort(dirEntries.begin(),
						 dirEntries.end(),
						 [](const SDirectoryEntry& a, const SDirectoryEntry& b) {
							 return typeToOrder(a.Type) < typeToOrder(b.Type);
						 });
	};
	static const auto sortByTypeDescending = [](std::vector<SDirectoryEntry>& dirEntries) {
		std::stable_sort(dirEntries.begin(),
						 dirEntries.end(),
						 [](const SDirectoryEntry& a, const SDirectoryEntry& b) {
							 return typeToOrder(a.Type) > typeToOrder(b.Type);
						 });
	};
	static const auto sortBySizeAscending = [](std::vector<SDirectoryEntry>& dirEntries) {
		std::stable_sort(
			dirEntries.begin(),
			dirEntries.end(),
			[](const SDirectoryEntry& a, const SDirectoryEntry& b) {
				auto* fs = wxGetApp().FileSystem();
				const SPathView pathA = a.Path;
				const SPathView pathB = b.Path;
				const FileStreamSize sizeA =
					a.Type == EDirectoryEntryType::File ?
						fs->FileSize(pathA) :
						a.Type == EDirectoryEntryType::Collection ?
						fs->FileSize(pathA.String().substr(0, pathA.String().size() - 1)) :
						0;
				const FileStreamSize sizeB =
					b.Type == EDirectoryEntryType::File ?
						fs->FileSize(pathB) :
						b.Type == EDirectoryEntryType::Collection ?
						fs->FileSize(pathB.String().substr(0, pathB.String().size() - 1)) :
						0;

				return sizeA < sizeB;
			});
	};
	static const auto sortBySizeDescending = [](std::vector<SDirectoryEntry>& dirEntries) {
		std::stable_sort(
			dirEntries.begin(),
			dirEntries.end(),
			[](const SDirectoryEntry& a, const SDirectoryEntry& b) {
				auto* fs = wxGetApp().FileSystem();
				const SPathView pathA = a.Path;
				const SPathView pathB = b.Path;
				const FileStreamSize sizeA =
					a.Type == EDirectoryEntryType::File ?
						fs->FileSize(pathA) :
						a.Type == EDirectoryEntryType::Collection ?
						fs->FileSize(pathA.String().substr(0, pathA.String().size() - 1)) :
						0;
				const FileStreamSize sizeB =
					b.Type == EDirectoryEntryType::File ?
						fs->FileSize(pathB) :
						b.Type == EDirectoryEntryType::Collection ?
						fs->FileSize(pathB.String().substr(0, pathB.String().size() - 1)) :
						0;

				return sizeA > sizeB;
			});
	};

	switch (column)
	{
	case NameCol:
	{
		ascending ? sortByNameAscending(mDirEntries) : sortByNameDescending(mDirEntries);

		sortByTypeAscending(mDirEntries);
		break;
	}
	case TypeCol:
	{
		sortByNameAscending(mDirEntries);

		ascending ? sortByTypeAscending(mDirEntries) : sortByTypeDescending(mDirEntries);
		break;
	}
	case SizeCol:
	{
		ascending ? sortBySizeAscending(mDirEntries) : sortBySizeDescending(mDirEntries);

		ascending ? sortByTypeAscending(mDirEntries) : sortByTypeDescending(mDirEntries);
		break;
	}
	default: Expects(false);
	}

	RefreshItems(0, mDirEntries.size() - 1);
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

	auto* fs = wxGetApp().FileSystem();
	// get device based on the path of the first selected item because all selected items should
	// have the same device since they are in the same directory
	IDevice* device = fs->FindDevice(entries[0]->Path);
	if (device == fs->FindDevice("/"))
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

		return new CVirtualFileDataObject{ fs, paths };
	}
}
