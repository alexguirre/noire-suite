#include "HashLookupWindow.h"
#include <charconv>
#include <formats/Hash.h>
#include <wx/arrstr.h>
#include <wx/button.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

CHashLookupWindow::CHashLookupWindow(wxWindow* parent)
	: wxFrame(parent, wxID_ANY, "noire-suite - Hash Lookup")
{
	wxPanel* topPanel = new wxPanel(this);
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

		wxPanel* inputPanel = new wxPanel(topPanel);
		{
			static float dest = 0.0f;
			wxStaticText* inputLabel = new wxStaticText(inputPanel, wxID_ANY, "Input:");
			// TODO: custom validator to only allow decimal and hexadecimal inputs
			mInputTextCtrl = new wxTextCtrl(inputPanel,
											wxID_ANY,
											wxEmptyString,
											wxDefaultPosition,
											wxDefaultSize,
											wxTE_MULTILINE | wxTE_DONTWRAP);

			wxBoxSizer* inputSizer = new wxBoxSizer(wxVERTICAL);
			inputSizer->Add(inputLabel, 0, 0);
			inputSizer->Add(mInputTextCtrl, 1, wxEXPAND);

			inputPanel->SetSizer(inputSizer);
		}

		wxPanel* outputPanel = new wxPanel(topPanel);
		{
			wxStaticText* outputLabel = new wxStaticText(outputPanel, wxID_ANY, "Output:");
			mOutputTextCtrl = new wxTextCtrl(outputPanel,
											 wxID_ANY,
											 wxEmptyString,
											 wxDefaultPosition,
											 wxDefaultSize,
											 wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_READONLY);

			wxBoxSizer* outputSizer = new wxBoxSizer(wxVERTICAL);
			outputSizer->Add(outputLabel, 0, 0);
			outputSizer->Add(mOutputTextCtrl, 1, wxEXPAND);

			outputPanel->SetSizer(outputSizer);
		}

		sizer->Add(inputPanel, 1, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 4);
		sizer->AddSpacer(4);
		sizer->Add(outputPanel, 1, wxEXPAND | wxRIGHT | wxTOP | wxBOTTOM, 4);

		topPanel->SetSizer(sizer);
	}

	wxButton* lookupBtn = new wxButton(this, wxID_ANY, "Lookup");
	lookupBtn->Bind(wxEVT_BUTTON, &CHashLookupWindow::OnLookup, this);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(topPanel, 1, wxEXPAND);
	mainSizer->Add(lookupBtn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

	SetSizer(mainSizer);
	SetBackgroundColour(*wxWHITE);

	SetSize(550, 650);
}

void CHashLookupWindow::OnLookup(wxCommandEvent&)
{
	wxLogDebug(__FUNCTION__);

	const wxString input = mInputTextCtrl->GetValue();
	wxString output{};
	for (const wxString& hashStr : wxSplit(input, '\n', '\0'))
	{
		wxLogDebug(" > %s", hashStr);

		const wxScopedCharBuffer hashChars = hashStr.To8BitData();
		if (hashStr.size() > 2 && hashStr[0] == '0' && hashStr[1] == 'x') // has hex prefix
		{
			// parse as hex
			if (std::uint32_t hashValue; std::from_chars(hashChars.data() + 2, // skip prefix
														 hashChars.data() + hashChars.length(),
														 hashValue,
														 16)
											 .ec == std::errc{})
			{
				if (auto s = noire::CHashDatabase::Instance().GetString(hashValue); s.has_value())
				{
					output += s.value();
				}
			}
		}
		else
		{
			// parse as decimal
			if (std::uint32_t hashValue; std::from_chars(hashChars.data(),
														 hashChars.data() + hashChars.length(),
														 hashValue,
														 10)
											 .ec == std::errc{})
			{
				if (auto s = noire::CHashDatabase::Instance().GetString(hashValue); s.has_value())
				{
					output += s.value();
				}
			}
		}

		output += '\n';
	}

	mOutputTextCtrl->SetValue(output);
}
