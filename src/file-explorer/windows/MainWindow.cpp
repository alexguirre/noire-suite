#include "MainWindow.h"
#include "controls/DirectoryContentsListCtrl.h"
#include "controls/DirectoryTreeCtrl.h"
#include "controls/ImagePanel.h"
#include <gsl/gsl>
#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/toolbar.h>

CMainWindow::CMainWindow()
	: wxFrame(nullptr, wxID_ANY, "noire-suite - File Explorer"), mMenuBar{ new wxMenuBar() }
{
	// construct menu bar
	{
		wxMenu* fileMenu = new wxMenu();
		fileMenu->Append(wxID_ANY, "Open folder...");
		fileMenu->AppendSeparator();
		fileMenu->Append(wxID_ANY, "Exit");

		mMenuBar->Append(fileMenu, "File");
		SetMenuBar(mMenuBar);
	}

	// create tool bar
	{
		// TODO: make tool bar functional
		wxToolBar* toolBar = CreateToolBar(wxTB_HORIZONTAL | wxTB_HORZ_LAYOUT, wxID_ANY);
		mDirPathText = new wxTextCtrl(toolBar, wxID_ANY);
		mDirPathText->SetEditable(false);

		toolBar->AddTool(wxID_BACKWARD,
						 wxEmptyString,
						 wxArtProvider::GetBitmap(wxART_GO_BACK),
						 wxEmptyString,
						 wxITEM_NORMAL);
		toolBar->AddTool(wxID_FORWARD,
						 wxEmptyString,
						 wxArtProvider::GetBitmap(wxART_GO_FORWARD),
						 wxEmptyString,
						 wxITEM_NORMAL);
		toolBar->AddTool(wxID_UP,
						 wxEmptyString,
						 wxArtProvider::GetBitmap(wxART_GO_UP),
						 wxEmptyString,
						 wxITEM_NORMAL);
		toolBar->AddControl(mDirPathText);
		toolBar->AddSeparator();

		toolBar->Realize();

		// handle resizing of mDirPathText, wxToolBar doesn't seem to support any kind of sizer so
		// we do it manually
		wxTextCtrl* dirPathText = mDirPathText; // var to allow capture in lambda
		Bind(wxEVT_SIZE, [dirPathText, toolBar](wxSizeEvent& e) {
			static bool queuedRealize = false;

			// how much space is required after the path text control
			constexpr std::size_t RightSideRequiredSpace{ 100 };

			const int newSize =
				e.GetSize().GetWidth() - dirPathText->GetPosition().x - RightSideRequiredSpace;
			dirPathText->SetSize(newSize, -1);
			if (!queuedRealize)
			{
				toolBar->CallAfter([toolBar]() {
					toolBar->Realize();
					queuedRealize = false;
				});
				queuedRealize = true;
			}
			e.Skip();
		});
	}

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

		mDirTreeCtrl = { new CDirectoryTreeCtrl(splitter, wxID_ANY) };
		mDirContentsListCtrl = { new CDirectoryContentsListCtrl(splitter, wxID_ANY) };

		splitter->SplitVertically(mDirTreeCtrl, mDirContentsListCtrl);

		SetSizer(mainSizer);
	}

	SetMinSize({ 650, 350 });

	mDirContentsListCtrl->SetDirectory(mDirTreeCtrl->File().Root());

	// just some dummy path for now
	mDirPathText->AppendText(mDirTreeCtrl->File().Path().string() + '\\' +
							 mDirTreeCtrl->File().Root().Name());

	mDirTreeCtrl->Bind(wxEVT_TREE_SEL_CHANGED, &CMainWindow::OnDirectoryTreeSelectionChanged, this);
}

void CMainWindow::OnDirectoryTreeSelectionChanged(wxTreeEvent& event)
{
	wxTreeItemId itemId = event.GetItem();
	Expects(itemId.IsOk());

	const CDirectoryItemData* data =
		reinterpret_cast<CDirectoryItemData*>(mDirTreeCtrl->GetItemData(itemId));

	if (data)
	{
		mDirContentsListCtrl->SetDirectory(data->Directory());

		event.Skip();
	}
}