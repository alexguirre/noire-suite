#include "TrunkWindow.h"
#include "controls/ImagePanel.h"
#include <algorithm>
#include <core/Hash.h>
#include <core/files/Trunk.h>
#include <core/streams/FileStream.h>
#include <filesystem>
#include <tuple>
#include <vector>
#include <wx/button.h>
#include <wx/dirdlg.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>

namespace noire::explorer
{
	wxImage CreateImageFromDDS(gsl::span<const byte> ddsData);

	TrunkWindow::TrunkWindow(wxWindow* parent,
							 wxWindowID id,
							 const wxString& title,
							 std::shared_ptr<Trunk> file)
		: wxFrame(parent, id, title), mFile{ file }
	{
		Expects(mFile != nullptr);

		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
		wxSplitterWindow* splitter = new wxSplitterWindow(this,
														  wxID_ANY,
														  wxDefaultPosition,
														  wxDefaultSize,
														  wxSP_LIVE_UPDATE);
		SetBackgroundColour(splitter->GetBackgroundColour());
		splitter->SetSashGravity(0.0);
		splitter->SetMinimumPaneSize(50);
		mainSizer->Add(splitter, 1, wxEXPAND | wxALL, 6);

		wxPanel* left = new wxPanel(splitter);

		wxPanel* leftTop = new wxPanel(left);
		{
			wxStaticText* sectionsTitle = new wxStaticText(leftTop, wxID_ANY, "Sections:");
			wxButton* rawExportBtn = new wxButton(leftTop, wxID_ANY, "Raw export");
			rawExportBtn->Bind(wxEVT_BUTTON, &TrunkWindow::OnRawExport, this);

			wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
			sizer->Add(sectionsTitle, 0, wxSHRINK | wxALIGN_BOTTOM);
			sizer->AddStretchSpacer();
			sizer->Add(rawExportBtn, 0, wxSHRINK | wxALIGN_CENTER_VERTICAL);

			leftTop->SetSizer(sizer);
		}
		wxListBox* sectionsList = new wxListBox(left, wxID_ANY);
		sectionsList->SetWindowStyle(wxLB_SINGLE | wxLB_SORT);

		wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
		leftSizer->Add(leftTop, 0, wxEXPAND);
		leftSizer->Add(sectionsList, 1, wxEXPAND);

		left->SetSizer(leftSizer);

		mRight = new wxScrolledWindow(splitter); // new wxPanel(splitter);
		mRight->SetScrollRate(5, 5);
		splitter->SplitVertically(left, mRight);

		SetSizer(mainSizer);

		SetSize(800, 500);

		if (!mFile->IsLoaded())
		{
			mFile->Load();
		}

		std::vector<std::tuple<wxString, void*>> sections{};
		for (auto& section : mFile->Sections())
		{
			void* data = reinterpret_cast<void*>(section.NameHash);
			sections.emplace_back(HashLookup::Instance(false).TryGetString(section.NameHash), data);
		}

		std::sort(sections.begin(), sections.end(), [](auto& a, auto& b) {
			return std::get<0>(a).CmpNoCase(std::get<0>(b)) < 0;
		});

		for (auto& sectionTuple : sections)
		{
			sectionsList->Append(std::get<0>(sectionTuple), std::get<1>(sectionTuple));
		}

		sectionsList->Bind(wxEVT_LISTBOX, &TrunkWindow::OnSectionSelected, this);
	}

	void TrunkWindow::OnSectionSelected(wxCommandEvent& e)
	{
		mRight->DestroyChildren();
		const u32 sectionNameHash = reinterpret_cast<u32>(e.GetClientData());
		if (sectionNameHash == crc32("uniquetexturemain") ||
			sectionNameHash == crc32("uniquetexturevram"))
		{
			if (auto uniqueTexture = mFile->GetUniqueTexture(); uniqueTexture)
			{
				Freeze();

				wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
				wxStaticText* countText = new wxStaticText(
					mRight,
					wxID_ANY,
					wxString::Format("Count: %zu", uniqueTexture->Textures.size()));
				rightSizer->Add(countText, 0, wxEXPAND | wxBOTTOM, 12);

				size i = 0;
				for (auto& t : uniqueTexture->Textures)
				{
					wxStaticText* nameText = new wxStaticText(
						mRight,
						wxID_ANY,
						wxString::Format("%s (offset:0x%08X)",
										 HashLookup::Instance(false).TryGetString(t.NameHash),
										 t.Offset));
					rightSizer->Add(nameText, 0, wxSHRINK);

					const std::vector<byte> imgData = uniqueTexture->GetTextureData(i);
					const wxImage img = CreateImageFromDDS(imgData);
					ImagePanel* imgPanel =
						new ImagePanel(mRight, wxID_ANY, wxDefaultPosition, wxDefaultSize);
					imgPanel->SetImage(img);
					imgPanel->SetBackgroundColour(wxColour{ 255, 0, 0 });
					imgPanel->SetSizeHints(64, 64, 256 + 128, 256 + 128);

					rightSizer->Add(imgPanel, 1, wxEXPAND | wxBOTTOM, 12);

					i++;
				}

				mRight->SetSizer(rightSizer);
				mRight->Layout();
				rightSizer->Fit(mRight);

				Thaw();
			}
		}
		else
		{
		}
		Layout();
		Refresh();
	}

	void TrunkWindow::OnRawExport(wxCommandEvent&)
	{
		wxDirDialog dlg(this, "Select the destination folder:", wxEmptyString, wxDD_DEFAULT_STYLE);

		if (dlg.ShowModal() != wxID_OK)
		{
			return;
		}

		std::filesystem::path path = dlg.GetPath().c_str().AsChar();
		path /= mFile->Path().Name();

		const bool isDir = std::filesystem::exists(path) && std::filesystem::is_directory(path);
		if (!isDir)
		{
			std::filesystem::create_directories(path);
		}

		for (auto& s : mFile->Sections())
		{
			const std::filesystem::path filePath =
				path / HashLookup::Instance(false).TryGetString(s.NameHash);

			FileStream f{ filePath };
			mFile->GetSectionDataStream(s.NameHash)->CopyTo(f);
		}
	}
}
