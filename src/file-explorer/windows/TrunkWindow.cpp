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
#include <wx/stattext.h>
#include <wx/toolbar.h>
#include <wx/wupdlock.h>

namespace noire::explorer
{
	static constexpr const char* SectionToString(TrunkWindow::Section s)
	{
		switch (s)
		{
		case TrunkWindow::Section::Graphics: return "Graphics";
		case TrunkWindow::Section::UniqueTexture: return "Unique Texture";
		default: return "Unknown";
		}
	}

	static void* SectionToClientData(TrunkWindow::Section s) { return reinterpret_cast<void*>(s); }

	static TrunkWindow::Section SectionFromClientData(void* d)
	{
		return static_cast<TrunkWindow::Section>(reinterpret_cast<uptr>(d));
	}

	// defined in App.cpp
	wxImage CreateImageFromDDS(gsl::span<const byte> ddsData);

	TrunkWindow::TrunkWindow(wxWindow* parent,
							 wxWindowID id,
							 const wxString& title,
							 std::shared_ptr<Trunk> file)
		: wxFrame(parent, id, title),
		  mFile{ file },
		  mCenter{ nullptr },
		  mCurrentSection{ Section::None }
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

			if (mFile->HasGraphics())
			{
				sectionsCombo->Append(SectionToString(Section::Graphics),
									  SectionToClientData(Section::Graphics));
			}

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
		const Section newSection = SectionFromClientData(e.GetClientData());
		if (mCurrentSection == newSection)
		{
			return;
		}

		mCenter->DestroyChildren();
		mCenter->Initialize(nullptr);

		switch (newSection)
		{
		case Section::Graphics:
		{
			ShowGraphics();
			break;
		}
		case Section::UniqueTexture:
		{
			ShowUniqueTexture();
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
			s.GetDataStream(*mFile).CopyTo(f);
		}
	}

	void TrunkWindow::ShowGraphics()
	{
		if (auto graphics = mFile->GetGraphics(); graphics)
		{
			trunk::Drawable& d = graphics->Drawable();

			wxString str{};

			str += wxString::Format("VertexBufferSize: %zu\n", graphics->VertexBufferSize);
			str += wxString::Format("IndexBufferSize:  %zu\n", graphics->IndexBufferSize);
			str += wxString::Format("Drawable:\n");
			str += wxString::Format("\tfield_8:     0x%08X\n", d.field_8);
			str += wxString::Format("\tfield_C:     0x%08X\n", d.field_C);
			str += wxString::Format("\tTextureUnk1: 0x%08X\n", d.TextureUnk1);
			str += wxString::Format("\tTextureUnk2: 0x%08X\n", d.TextureUnk2);
			str += wxString::Format("\tModel Count: %zu\n", d.ModelCount());
			for (size i = 0; i < d.ModelCount(); i++)
			{
				trunk::Model& m = *d.Models[i];

				str += wxString::Format("\t#%zu\n", i);
				str += wxString::Format("\t  Geometry Count: %zu\n", m.GeometryCount);
			}

			wxStaticText* t = new wxStaticText(mCenter, wxID_ANY, str);
			mCenter->Initialize(t);
		}
	}

	void TrunkWindow::ShowUniqueTexture()
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
			wxWindow* right =
				new wxWindow(mCenter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);

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

			ImagePanel* imgPanel =
				new ImagePanel(right, wxID_ANY, wxDefaultPosition, wxDefaultSize);

			right->GetSizer()->Add(imgPanel, 1, wxEXPAND);

			right->Layout();
			right->Refresh();

			const bool hasTextures = uniqueTexture->Textures.size() > 0;

			left->Bind(
				wxEVT_LISTBOX,
				[this, imgPanel, uniqueTexture{ std::move(uniqueTexture) }](wxCommandEvent& e) {
					size texIndex = reinterpret_cast<size>(e.GetClientData());
					const std::vector<byte> imgData = uniqueTexture->GetTextureData(texIndex);
					const wxImage img = CreateImageFromDDS(imgData);
					imgPanel->SetImage(img);

					imgPanel->Refresh();
				});

			if (hasTextures)
			{
				left->SetSelection(0, true);

				wxCommandEvent evt{ wxEVT_LISTBOX };
				evt.SetClientData(reinterpret_cast<void*>(0));
				left->Command(evt);
			}
		}
	}
}
