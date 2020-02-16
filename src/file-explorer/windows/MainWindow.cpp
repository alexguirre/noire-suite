#include "MainWindow.h"
#include "Identifiers.h"
#include "controls/DirectoryContentsListCtrl.h"
#include "controls/DirectoryTreeCtrl.h"
#include "controls/ImagePanel.h"
#include "controls/PathToolBar.h"
#include "controls/StatusBar.h"
#include "rendering/Renderer.h"
#include <App.h>
#include <gsl/gsl>
#include <windows/HashLookupWindow.h>
#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>

namespace noire::explorer
{
	MainWindow::MainWindow()
		: wxFrame(nullptr, wxID_ANY, "noire-suite - File Explorer"),
		  mMenuBar{ new wxMenuBar() },
		  mDirTreeCtrl{ nullptr },
		  mDirContentsListCtrl{ nullptr }
	{
		// construct menu bar
		{
			wxMenu* fileMenu = new wxMenu();
			fileMenu->Append(MenuBarOpenFolderId, "&Open folder...");
			fileMenu->AppendSeparator();
			fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4");

			wxMenu* toolsMenu = new wxMenu();
			toolsMenu->Append(MenuBarHashLookupId, "Hash &Lookup...");
			toolsMenu->Append(MenuBarTestRendererId, "Test &Renderer...");

			mMenuBar->Append(fileMenu, "&File");
			mMenuBar->Append(toolsMenu, "&Tools");
			SetMenuBar(mMenuBar);
		}

		CreateToolBar();

		CreateStatusBar();

		// create main directory tree and contents list within a splitter
		{
			wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
			wxSplitterWindow* splitter = new wxSplitterWindow(this,
															  wxID_ANY,
															  wxDefaultPosition,
															  wxDefaultSize,
															  wxSP_LIVE_UPDATE);
			splitter->SetSashGravity(0.0);
			splitter->SetMinimumPaneSize(50);
			mainSizer->Add(splitter, 1, wxEXPAND);

			mDirTreeCtrl = { new DirectoryTreeCtrl(splitter, wxID_ANY) };
			mDirContentsListCtrl = { new DirectoryContentsListCtrl(splitter, wxID_ANY) };

			splitter->SplitVertically(mDirTreeCtrl, mDirContentsListCtrl);

			SetSizer(mainSizer);
		}

		CreateAccelTable();

		SetMinSize({ 650, 350 });

		// TODO: giving the tool bar non-const access to mDirContentsListCtrl is a bit dirty
		static_cast<PathToolBar*>(GetToolBar())->SetDirectoryContentsListCtrl(mDirContentsListCtrl);

		mDirTreeCtrl->Bind(wxEVT_TREE_SEL_CHANGED,
						   &MainWindow::OnDirectoryTreeSelectionChanged,
						   this);

		Bind(wxEVT_MENU, &MainWindow::OnOpenFolder, this, MenuBarOpenFolderId);
		Bind(wxEVT_MENU, &MainWindow::OnHashLookupTool, this, MenuBarHashLookupId);
		Bind(wxEVT_MENU, &MainWindow::OnTestRenderer, this, MenuBarTestRendererId);
		Bind(wxEVT_MENU, &MainWindow::OnExit, this, wxID_EXIT);
		wxGetApp().Bind(EVT_FILE_SYSTEM_SCAN_STARTED, &MainWindow::OnFileSystemScanStarted, this);
		wxGetApp().Bind(EVT_FILE_SYSTEM_SCAN_COMPLETED,
						&MainWindow::OnFileSystemScanCompleted,
						this);
	}

	wxToolBar* MainWindow::OnCreateToolBar(long style, wxWindowID id, const wxString& name)
	{
		return new PathToolBar(this, id, wxDefaultPosition, wxDefaultSize, style, name);
	}

	wxStatusBar* MainWindow::OnCreateStatusBar(int, long style, wxWindowID id, const wxString& name)
	{
		return new StatusBar(this, id, style, name);
	}

	void MainWindow::OnDirectoryTreeSelectionChanged(wxTreeEvent& event)
	{
		wxTreeItemId itemId = event.GetItem();
		Expects(itemId.IsOk());

		const DirectoryItemData* data =
			reinterpret_cast<DirectoryItemData*>(mDirTreeCtrl->GetItemData(itemId));

		if (data)
		{
			mDirContentsListCtrl->SetDirectory(data->Path());

			event.Skip();
		}
	}

	void MainWindow::OnOpenFolder(wxCommandEvent&)
	{
		wxDirDialog dlg(this,
						"Select the game folder:",
						wxEmptyString, // TODO: try to find game folder (looking in registry?)
						wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

		if (dlg.ShowModal() != wxID_OK)
		{
			return;
		}

		std::filesystem::path rootPath = dlg.GetPath().c_str().AsChar();
		wxGetApp().ChangeRootPath(rootPath);
	}

	void MainWindow::OnHashLookupTool(wxCommandEvent&)
	{
		HashLookupWindow* win = new HashLookupWindow(this);
		win->Show();
	}

	void MainWindow::OnTestRenderer(wxCommandEvent&)
	{
		class RendererWindow : public wxFrame
		{
		public:
			RendererWindow(wxWindow* parent) : wxFrame(parent, wxID_ANY, "D3D11 Renderer")
			{
				wxWindow* renderPanel = new wxWindow(this, wxID_ANY);
				mRenderer = std::make_unique<CRenderer>(
					renderPanel->GetHWND(),
					[](CRenderer& r, float) { r.Clear(0.2f, 0.2f, 0.7f); });
			}

			std::unique_ptr<CRenderer> mRenderer;
		};

		(new RendererWindow(this))->Show();
	}

	void MainWindow::OnExit(wxCommandEvent&) { Close(true); }

	void MainWindow::OnFileSystemScanStarted(wxThreadEvent& event)
	{
		wxLogDebug(__FUNCTION__);

		if (StatusBar* statusBar = static_cast<StatusBar*>(GetStatusBar()); statusBar)
		{
			statusBar->SetInfo(wxString::Format("Scanning '%ls'...", "path/TBD/"));
		}

		event.Skip();
	}

	void MainWindow::OnFileSystemScanCompleted(wxThreadEvent& event)
	{
		if (mDirTreeCtrl)
		{
			mDirTreeCtrl->Refresh();
		}

		if (mDirContentsListCtrl)
		{
			mDirContentsListCtrl->SetDirectoryToRoot();
		}

		if (StatusBar* statusBar = static_cast<StatusBar*>(GetStatusBar()); statusBar)
		{
			statusBar->SetInfo("");
		}

		event.Skip();
	}

	void MainWindow::CreateAccelTable()
	{
		std::array<wxAcceleratorEntry, 5> entries{};
		std::size_t i = 0;

		// for path tool bar back and forward buttons
		// TODO: back/forward accelerators for mouse extra buttons, like windows file explorer
		entries[i++].Set(wxACCEL_ALT, WXK_LEFT, wxID_BACKWARD);
		entries[i++].Set(wxACCEL_NORMAL, WXK_BROWSER_BACK, wxID_BACKWARD);
		entries[i++].Set(wxACCEL_ALT, WXK_RIGHT, wxID_FORWARD);
		entries[i++].Set(wxACCEL_NORMAL, WXK_BROWSER_FORWARD, wxID_FORWARD);
		entries[i++].Set(wxACCEL_ALT, WXK_UP, wxID_UP);

		SetAcceleratorTable({ entries.size(), entries.data() });
	}
}
