#pragma once
#include "DirectoryContentsListCtrl.h"
#include <formats/fs/FileSystem.h>
#include <wx/statusbr.h>

enum class EStatusBarField : int
{
	NumberOfItems = 0,
	SelectedItems,
	Info,

	Count,
};

class CStatusBar : public wxStatusBar
{
public:
	CStatusBar(wxWindow* parent,
			   wxWindowID id = wxID_ANY,
			   long style = wxSTB_DEFAULT_STYLE,
			   const wxString& name = wxStatusBarNameStr);

	void SetNumberOfItems(std::size_t num);
	void SetSelectedItems(std::size_t num, std::uintmax_t totalByteSize);
	void SetInfo(const wxString& newText);

private:
	void SetFieldText(EStatusBarField field, const wxString& newText);
	void UpdateFieldWidth(EStatusBarField field, const wxString& newText);

	void OnDirectoryChanged(CDirectoryEvent& e);
};