#include "HashLookupWindow.h"
#include <charconv>
#include <core/Hash.h>
#include <wx/arrstr.h>
#include <wx/button.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace noire::explorer
{
	HashLookupWindow::HashLookupWindow(wxWindow* parent)
		: wxFrame(parent, wxID_ANY, "noire-suite - Hash Lookup")
	{
		wxPanel* topPanel = new wxPanel(this);
		{
			wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

			wxPanel* inputPanel = new wxPanel(topPanel);
			{
				static float dest = 0.0f;
				wxStaticText* inputLabel = new wxStaticText(inputPanel, wxID_ANY, "Input:");
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
		lookupBtn->Bind(wxEVT_BUTTON, &HashLookupWindow::OnLookup, this);

		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
		mainSizer->Add(topPanel, 1, wxEXPAND);
		mainSizer->Add(lookupBtn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

		SetSizer(mainSizer);
		SetBackgroundColour(*wxWHITE);

		SetSize(700, 500);

		mInputTextCtrl->Bind(wxEVT_CHAR, &HashLookupWindow::OnInputChar, this);
	}

	void HashLookupWindow::OnLookup(wxCommandEvent&)
	{
		wxLogDebug(__FUNCTION__);

		const wxString input = mInputTextCtrl->GetValue();
		wxString output{};
		for (const wxString& hashStr : wxSplit(input, '\n', '\0'))
		{
			wxLogDebug(" > %s", hashStr);

			const wxScopedCharBuffer hashChars = hashStr.To8BitData();
			const char* charsBegin = hashChars.data();
			const char* charsEnd = hashChars.data() + hashChars.length();
			int base = 10;

			if (hashStr.size() > 2 && hashStr[0] == '0' && hashStr[1] == 'x') // has hex prefix
			{
				charsBegin += 2; // skip prefix
				base = 16;
			}

			u32 hashValue;
			if (auto [str, err] = std::from_chars(charsBegin, charsEnd, hashValue, base);
				err == std::errc{} && str == charsEnd)
			{
				if (auto s = HashLookup::Instance().GetString(hashValue); s.has_value())
				{
					output += s.value();
				}
			}

			output += '\n';
		}

		mOutputTextCtrl->SetValue(output);
	}

	void HashLookupWindow::OnInputChar(wxKeyEvent& event)
	{
		wxLogDebug(__FUNCTION__ ": key:'%c'", event.GetUnicodeKey());

		// skip by default
		event.Skip();

		wxChar k = event.GetUnicodeKey();

		if (k < WXK_SPACE || k == WXK_DELETE)
		{
			return;
		}

		// only allow numeric chars
		if ((k >= '0' && k <= '9') || (k >= 'a' && k <= 'f') || (k >= 'A' && k <= 'F') || k == 'x')
		{
			return;
		}

		// eat message
		event.Skip(false);
	}
}
