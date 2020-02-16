#include "StatusBar.h"
#include "util/Format.h"
#include <App.h>
#include <array>
#include <wx/dcclient.h>

namespace noire::explorer
{
	StatusBar::StatusBar(wxWindow* parent, wxWindowID id, long style, const wxString& name)
		: wxStatusBar(parent, id, style, name)
	{
		SetFieldsCount(static_cast<int>(StatusBarField::Count));

		SetNumberOfItems(0);
		SetSelectedItems(0, 0);

		parent->Bind(EVT_DIRECTORY_CHANGED, &StatusBar::OnDirectoryChanged, this);
	}

	void StatusBar::SetNumberOfItems(size num)
	{
		SetFieldText(StatusBarField::NumberOfItems,
					 wxString::Format("%zu item%s", num, num == 1 ? "" : "s"));
	}

	// TODO: update SelectedItems status
	void StatusBar::SetSelectedItems(size num, u64 totalByteSize)
	{
		wxString str = wxString::Format("%zu item%s selected  %s",
										num,
										num == 1 ? "" : "s",
										BytesToHumanReadable(totalByteSize).c_str());
		SetFieldText(StatusBarField::SelectedItems, str);
	}

	void StatusBar::SetInfo(const wxString& newText)
	{
		SetFieldText(StatusBarField::Info, newText);
	}

	void StatusBar::SetFieldText(StatusBarField field, const wxString& newText)
	{
		SetStatusText(newText, static_cast<int>(field));
		UpdateFieldWidth(field, newText);
	}

	void StatusBar::UpdateFieldWidth(StatusBarField field, const wxString& newText)
	{
		std::array<int, static_cast<size>(StatusBarField::Count)> widths;
		for (size i = 0; i < static_cast<size>(StatusBarField::Count); i++)
		{
			widths[i] = GetStatusWidth(i);
		}

		wxClientDC dc{ this };
		widths[static_cast<size>(field)] = dc.GetTextExtent(newText).GetX();
		SetStatusWidths(widths.size(), widths.data());
	}

	void StatusBar::OnDirectoryChanged(DirectoryEvent& e)
	{
		// TODO: GetEntries may be too much overhead when a directory has too many items, maybe we
		// need a GetEntryCount function or similar in the filesystem
		const size numItems = 0;
		// TODO: wxGetApp().RootDevice()->GetEntries(e.GetDirectory()).size();

		SetNumberOfItems(numItems);

		e.Skip();
	}
}
