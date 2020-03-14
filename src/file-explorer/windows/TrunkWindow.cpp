#include "TrunkWindow.h"
#include "controls/ImagePanel.h"
#include <algorithm>
#include <core/Hash.h>
#include <core/files/Trunk.h>
#include <core/streams/FileStream.h>
#include <filesystem>
#include <tuple>
#include <utility>
#include <vector>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/dirdlg.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/toolbar.h>
#include <wx/wupdlock.h>

namespace noire::explorer
{
	enum class Section : uptr
	{
		UniqueTexture = 1,
	};

	static constexpr const char* SectionToString(Section s)
	{
		switch (s)
		{
		case Section::UniqueTexture: return "Unique Texture";
		default: return "Unknown";
		}
	}

	static void* SectionToClientData(Section s) { return reinterpret_cast<void*>(s); }

	static Section SectionFromClientData(void* d)
	{
		return static_cast<Section>(reinterpret_cast<uptr>(d));
	}

	// defined in App.cpp
	wxImage CreateImageFromDDS(gsl::span<const byte> ddsData);

	TrunkWindow::TrunkWindow(wxWindow* parent,
							 wxWindowID id,
							 const wxString& title,
							 std::shared_ptr<Trunk> file)
		: wxFrame(parent, id, title), mFile{ file }
	{
		Expects(mFile != nullptr);

		if (!mFile->IsLoaded())
		{
			mFile->Load();
		}

		wxToolBar* tb = CreateToolBar();
		{
			wxComboBox* sectionsCombo = new wxComboBox(tb,
													   wxID_ANY,
													   wxEmptyString,
													   wxDefaultPosition,
													   wxDefaultSize,
													   0,
													   nullptr,
													   wxCB_READONLY | wxCB_DROPDOWN);
			tb->AddControl(sectionsCombo, "Sections:");

			if (mFile->HasUniqueTexture())
			{
				sectionsCombo->Append(SectionToString(Section::UniqueTexture),
									  SectionToClientData(Section::UniqueTexture));
			}

			sectionsCombo->Bind(wxEVT_COMBOBOX, &TrunkWindow::OnSectionSelected, this);

			wxButton* rawExportBtn = new wxButton(tb, wxID_ANY, "Raw export");
			tb->AddControl(rawExportBtn);

			rawExportBtn->Bind(wxEVT_BUTTON, &TrunkWindow::OnRawExport, this);

			tb->Realize();
		}

		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

		mCenter = new wxSplitterWindow(this,
									   wxID_ANY,
									   wxDefaultPosition,
									   wxDefaultSize,
									   wxSP_LIVE_UPDATE);
		mCenter->SetSashGravity(0.0);
		mCenter->SetMinimumPaneSize(50);

		mainSizer->Add(mCenter, 1, wxEXPAND);

		SetSizer(mainSizer);

		SetSize(800, 500);

		auto e = wxCommandEvent{};
		OnSectionSelected(e);
	}

	void TrunkWindow::OnSectionSelected(wxCommandEvent& e)
	{
		mCenter->Unsplit();
		mCenter->DestroyChildren();

		switch (SectionFromClientData(e.GetClientData()))
		{
		case Section::UniqueTexture:
		{
			if (auto uniqueTexture = mFile->GetUniqueTexture(); uniqueTexture)
			{
				wxWindowUpdateLocker lock{ this };

				wxListBox* left = new wxListBox(mCenter,
												wxID_ANY,
												wxDefaultPosition,
												wxDefaultSize,
												0,
												nullptr,
												wxLB_SINGLE | wxBORDER_THEME);
				wxWindow* right = new wxWindow(mCenter,
											   wxID_ANY,
											   wxDefaultPosition,
											   wxDefaultSize,
											   wxBORDER_THEME);

				left->SetBackgroundColour(*wxWHITE);
				right->SetBackgroundColour(*wxWHITE);
				right->SetSizer(new wxBoxSizer(wxVERTICAL));

				size i = 0;
				for (auto& t : uniqueTexture->Textures)
				{
					const wxString imgName =
						wxString::Format("%s (offset:0x%08X)",
										 HashLookup::Instance(false).TryGetString(t.NameHash),
										 t.Offset);
					left->Append(imgName, reinterpret_cast<void*>(i));

					i++;
				}

				mCenter->SplitVertically(left, right);

				left->Bind(
					wxEVT_LISTBOX,
					[this, right, uniqueTexture{ std::move(uniqueTexture) }](wxCommandEvent& e) {
						right->DestroyChildren();

						size texIndex = reinterpret_cast<size>(e.GetClientData());
						const std::vector<byte> imgData = uniqueTexture->GetTextureData(texIndex);
						const wxImage img = CreateImageFromDDS(imgData);
						ImagePanel* imgPanel =
							new ImagePanel(right, wxID_ANY, wxDefaultPosition, wxDefaultSize);
						imgPanel->SetImage(img);

						right->GetSizer()->Add(imgPanel, 1, wxEXPAND);
						right->Layout();
						right->Refresh();
					});
			}
			break;
		}
		}
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
