#include "StatusBar.h"
#include "util/Format.h"
#include <array>
#include <wx/dcclient.h>

using namespace noire::fs;

CStatusBar::CStatusBar(wxWindow* parent, wxWindowID id, long style, const wxString& name)
	: wxStatusBar(parent, id, style, name)
{
	SetFieldsCount(static_cast<int>(EStatusBarField::Count));

	SetNumberOfItems(0);
	SetSelectedItems(0, 0);

	parent->Bind(EVT_DIRECTORY_CHANGED, &CStatusBar::OnDirectoryChanged, this);
}

void CStatusBar::SetFileSystem(CFileSystem* fileSystem)
{
	mFileSystem = fileSystem;
}

void CStatusBar::SetNumberOfItems(std::size_t num)
{
	SetFieldText(EStatusBarField::NumberOfItems,
				 wxString::Format("%zu item%s", num, num == 1 ? "" : "s"));
}

// TODO: update SelectedItems status
void CStatusBar::SetSelectedItems(std::size_t num, std::uintmax_t totalByteSize)
{
	wxString str = wxString::Format("%zu item%s selected  %s",
									num,
									num == 1 ? "" : "s",
									BytesToHumanReadable(totalByteSize).c_str());
	SetFieldText(EStatusBarField::SelectedItems, str);
}

void CStatusBar::SetInfo(const wxString& newText)
{
	SetFieldText(EStatusBarField::Info, newText);
}

void CStatusBar::SetFieldText(EStatusBarField field, const wxString& newText)
{
	SetStatusText(newText, static_cast<int>(field));
	UpdateFieldWidth(field, newText);
}

void CStatusBar::UpdateFieldWidth(EStatusBarField field, const wxString& newText)
{
	std::array<int, static_cast<std::size_t>(EStatusBarField::Count)> widths;
	for (std::size_t i = 0; i < static_cast<std::size_t>(EStatusBarField::Count); i++)
	{
		widths[i] = GetStatusWidth(i);
	}

	wxClientDC dc{ this };
	widths[static_cast<int>(field)] = dc.GetTextExtent(newText).GetX();
	SetStatusWidths(widths.size(), widths.data());
}

void CStatusBar::OnDirectoryChanged(CDirectoryEvent& e)
{
	// TODO: GetEntries may be too much overhead when a directory has too many items, maybe we need
	// a GetEntryCount function or similar in the filesystem
	std::size_t numItems = mFileSystem->GetEntries(e.GetDirectory()).size();

	SetNumberOfItems(numItems);

	e.Skip();
}
