#include "DirectoryContentsListCtrl.h"
#include "Identifiers.h"
#include "Images.h"
#include "util/Format.h"
#include "util/VirtualFileDataObject.h"
#include <App.h>
#include <core/devices/Device.h>
#include <core/files/File.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <gsl/gsl>
#include <vector>
#include <wx/clipbrd.h>
#include <wx/dnd.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/wx.h>

namespace noire::explorer
{
	DirectoryEvent::DirectoryEvent() : wxEvent(wxID_ANY, EVT_DIRECTORY_CHANGED) {}
	DirectoryEvent::DirectoryEvent(PathView dirPath, int winId)
		: wxEvent(winId, EVT_DIRECTORY_CHANGED), mDirPath{ dirPath }
	{
	}

	DirectoryEvent::DirectoryEvent(const DirectoryEvent& e)
		: wxEvent(e), mDirPath{ e.GetDirectory() }
	{
	}

	wxIMPLEMENT_DYNAMIC_CLASS(DirectoryEvent, wxEvent);
	wxDEFINE_EVENT(EVT_DIRECTORY_CHANGED, DirectoryEvent);

	// clang-format off
wxBEGIN_EVENT_TABLE(DirectoryContentsListCtrl, wxListCtrl)
	// for context menu
	EVT_LIST_ITEM_RIGHT_CLICK(wxID_ANY, DirectoryContentsListCtrl::OnItemContextMenu)
	EVT_LIST_ITEM_ACTIVATED(wxID_ANY, DirectoryContentsListCtrl::OnItemActivated)
wxEND_EVENT_TABLE();
	// clang-format on

	class DirectoryItemContextMenu : public wxMenu
	{
	public:
		DirectoryItemContextMenu() : wxMenu()
		{
			Append(DirectoryContentsExportMenuId, "Export");

			Connect(wxEVT_COMMAND_MENU_SELECTED,
					wxCommandEventHandler(DirectoryItemContextMenu::OnExport),
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

	DirectoryContentsListCtrl::DirectoryContentsListCtrl(wxWindow* parent,
														 const wxWindowID id,
														 const wxPoint& pos,
														 const wxSize& size)
		: wxListCtrl(parent, id, pos, size, wxLC_REPORT | wxLC_VIRTUAL),
		  mDirPath{},
		  mDirEntries{},
		  mColSortDescending{}
	{
		SetImageList(Images::Icons(), wxIMAGE_LIST_SMALL);
		BuildColumns();

		Bind(wxEVT_LIST_BEGIN_DRAG, &DirectoryContentsListCtrl::OnBeginDrag, this);
		Bind(wxEVT_MENU, &DirectoryContentsListCtrl::OnCopy, this, wxID_COPY);
		Bind(wxEVT_LIST_COL_CLICK, &DirectoryContentsListCtrl::OnColClick, this);

		// wxID_COPY
		std::array<wxAcceleratorEntry, 1> entries{};
		entries[0].Set(wxACCEL_CTRL, 'C', wxID_COPY);

		SetAcceleratorTable({ entries.size(), entries.data() });
	}

	void DirectoryContentsListCtrl::SetDirectoryToRoot()
	{
		mDirPath = PathView{};
		SetDirectory("/");
	}

	void DirectoryContentsListCtrl::SetDirectory(PathView dirPath)
	{
		Expects(dirPath.IsDirectory());

		if (mDirPath != dirPath)
		{
			mDirPath = dirPath;
			UpdateContents();

			DirectoryEvent event{ mDirPath };
			event.SetEventObject(this);
			wxPostEvent(wxGetTopLevelParent(this)->GetEventHandler(), event);
		}
	}

	void DirectoryContentsListCtrl::OnItemContextMenu(wxListEvent& event)
	{
		ShowItemContextMenu();
		event.Skip();
	}

	void DirectoryContentsListCtrl::OnItemActivated(wxListEvent& event)
	{
		const Entry& entry = mDirEntries[event.GetIndex()];

		if (entry.IsDirectory)
		{
			SetDirectory(entry.Path);
		}
		else if (entry.IsDevice)
		{
			const Path p = entry.Path + Path::DirectorySeparator;
			if (MultiDevice* dev = wxGetApp().RootDevice(); !dev->Exists(p))
			{
				std::shared_ptr f = dev->Open(entry.Path);
				if (!f->IsLoaded())
				{
					auto _ = ShowWaitingCursor(this);
					f->Load();
				}

				dev->Mount(p, std::dynamic_pointer_cast<Device>(f));
			}

			SetDirectory(p);
		}
		else
		{
			wxGetApp().OpenFile(entry.Path);
		}

		event.Skip();
	}

	void DirectoryContentsListCtrl::OnBeginDrag(wxListEvent& event)
	{
		wxLogDebug(__FUNCTION__);

		if (std::unique_ptr<wxDataObject> dataObj{ CreateSelectedFilesDataObject() }; dataObj)
		{
			wxDropSource src{ *dataObj, this };
			src.DoDragDrop(wxDrag_CopyOnly);
		}
		event.Skip();
	}

	void DirectoryContentsListCtrl::OnCopy(wxCommandEvent& event)
	{
		wxLogDebug(__FUNCTION__);

		if (wxDataObject* dataObj = CreateSelectedFilesDataObject();
			dataObj && wxTheClipboard->Open())
		{
			// wxTheClipboard takes ownership of the wxDataObject so don't delete it
			wxTheClipboard->SetData(dataObj);
			wxTheClipboard->Close();
		}
		event.Skip();
	}

	wxString DirectoryContentsListCtrl::OnGetItemText(long item, long column) const
	{
		const Entry& e = mDirEntries[item];
		if (!e.IsDirectory)
		{
			switch (column)
			{
			case NameCol:
			{
				std::string_view name = e.Path.Name();
				return { name.data(), name.size() };
			}
			case TypeCol: return "File";
			case SizeCol: return BytesToHumanReadable(e.Size);
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
			case SizeCol: return "";
			}
		}

		// should never get here
		Expects(false);
	}

	int DirectoryContentsListCtrl::OnGetItemImage(long item) const
	{
		const Entry& e = mDirEntries[item];
		return e.IsDirectory ? Images::IconFolder :
							   (e.IsDevice ? Images::IconBlueFolder : Images::IconBlankFile);
	}

	void DirectoryContentsListCtrl::OnColClick(wxListEvent& e)
	{
		wxLogDebug(__FUNCTION__ ": %d", e.GetColumn());

		const auto _ = ShowWaitingCursor(this);

		const long column = e.GetColumn();
		const bool sortAscending = !(mColSortDescending[column] = !mColSortDescending[column]);
		SortContents(column, sortAscending);

		e.Skip();
	}

	void DirectoryContentsListCtrl::ShowItemContextMenu()
	{
		wxMenu menu{};
		menu.Append(wxID_COPY);

		PopupMenu(&menu);
	}

	void DirectoryContentsListCtrl::BuildColumns()
	{
		const long nameCol = AppendColumn("Name", wxLIST_FORMAT_LEFT, 250);
		const long typeCol = AppendColumn("Type");
		const long sizeCol = AppendColumn("Size", wxLIST_FORMAT_RIGHT);

		Ensures(nameCol == NameCol);
		Ensures(typeCol == TypeCol);
		Ensures(sizeCol == SizeCol);
	}

	void DirectoryContentsListCtrl::UpdateContents()
	{
		Device* dev = wxGetApp().RootDevice();
		if (!dev || mDirPath.IsEmpty())
		{
			SetItemCount(0);
			return;
		}

		Freeze();
		const auto _ = ShowWaitingCursor(this);

		// TODO: mounted devices not considered as directories yet
		mDirEntries.clear();
		dev->Visit(
			[this](PathView p) {
				mDirEntries.emplace_back(Entry{ Path{ p }, true, false, 0 });
			},
			[this, dev](PathView p) {
				std::shared_ptr f = dev->Open(p);
				const bool isDevice = std::dynamic_pointer_cast<Device>(f) != nullptr;
				mDirEntries.emplace_back(Entry{ Path{ p }, false, isDevice, f->Size() });
			},
			mDirPath,
			false);

		SetItemCount(mDirEntries.size());

		std::fill(mColSortDescending.begin(), mColSortDescending.end(), false);
		SortContents(NameCol, true);

		Thaw();
	}

	void DirectoryContentsListCtrl::SortContents(long column, bool ascending)
	{
		static const auto typeToOrder = [](bool isDirectory, bool isDevice) -> int {
			// priority so directories appear before files by default
			return isDirectory ? 0 : (isDevice ? 1 : 2);
		};
		static const auto sortByNameAscending = [](std::vector<Entry>& dirEntries) {
			std::stable_sort(
				dirEntries.begin(),
				dirEntries.end(),
				[](const Entry& a, const Entry& b) { return a.Path.Name() < b.Path.Name(); });
		};
		static const auto sortByNameDescending = [](std::vector<Entry>& dirEntries) {
			std::stable_sort(
				dirEntries.begin(),
				dirEntries.end(),
				[](const Entry& a, const Entry& b) { return a.Path.Name() > b.Path.Name(); });
		};
		static const auto sortByTypeAscending = [](std::vector<Entry>& dirEntries) {
			std::stable_sort(dirEntries.begin(),
							 dirEntries.end(),
							 [](const Entry& a, const Entry& b) {
								 return typeToOrder(a.IsDirectory, a.IsDevice) <
										typeToOrder(b.IsDirectory, a.IsDevice);
							 });
		};
		static const auto sortByTypeDescending = [](std::vector<Entry>& dirEntries) {
			std::stable_sort(dirEntries.begin(),
							 dirEntries.end(),
							 [](const Entry& a, const Entry& b) {
								 return typeToOrder(a.IsDirectory, a.IsDevice) >
										typeToOrder(b.IsDirectory, a.IsDevice);
							 });
		};
		static const auto sortBySizeAscending = [](std::vector<Entry>& dirEntries) {
			std::stable_sort(dirEntries.begin(),
							 dirEntries.end(),
							 [](const Entry& a, const Entry& b) { return a.Size < b.Size; });
		};
		static const auto sortBySizeDescending = [](std::vector<Entry>& dirEntries) {
			std::stable_sort(dirEntries.begin(),
							 dirEntries.end(),
							 [](const Entry& a, const Entry& b) { return a.Size > b.Size; });
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

	wxDataObject* DirectoryContentsListCtrl::CreateSelectedFilesDataObject() const
	{
		wxLogDebug(__FUNCTION__);

		std::vector<const Entry*> selected{};
		long item = -1;
		while ((item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND)
		{
			wxLogDebug("> '%s'", GetItemText(item));
			const Entry* entry = &mDirEntries[item];
			if (entry && !entry->IsDirectory)
			{
				selected.push_back(entry);
			}
		}

		if (selected.size() == 0)
		{
			return nullptr;
		}

		Device& dev = *wxGetApp().RootDevice();
		// get device based on the path of the first selected item because all selected items should
		// have the same device since they are in the same directory

		// if physical files selected
		//{
		//	wxFileDataObject* dataObj = new wxFileDataObject;
		//	const std::filesystem::path& rootPath =
		//		reinterpret_cast<noire::fs::CNativeDevice*>(device)->RootDirectory();
		//	for (const Entry* e : selected)
		//	{
		//		PathView relPath = e->Path.RelativeTo("/");
		//		std::filesystem::path fullPath = rootPath / relPath.String();

		//		dataObj->AddFile(fullPath.c_str());
		//	}

		//	return dataObj;
		//}
		// else
		{
			std::vector<PathView> paths{};
			paths.reserve(selected.size());
			std::transform(selected.begin(),
						   selected.end(),
						   std::back_inserter(paths),
						   [](const Entry* e) { return PathView{ e->Path }; });

			return new VirtualFileDataObject{ dev, paths };
		}
	}
}
