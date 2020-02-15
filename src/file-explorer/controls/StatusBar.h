#pragma once
#include "DirectoryContentsListCtrl.h"
#include <core/Common.h>
#include <wx/statusbr.h>

namespace noire::explorer
{
	enum class StatusBarField : int
	{
		NumberOfItems = 0,
		SelectedItems,
		Info,

		Count,
	};

	class StatusBar : public wxStatusBar
	{
	public:
		StatusBar(wxWindow* parent,
				  wxWindowID id = wxID_ANY,
				  long style = wxSTB_DEFAULT_STYLE,
				  const wxString& name = wxStatusBarNameStr);

		void SetNumberOfItems(size num);
		void SetSelectedItems(size num, u64 totalByteSize);
		void SetInfo(const wxString& newText);

	private:
		void SetFieldText(StatusBarField field, const wxString& newText);
		void UpdateFieldWidth(StatusBarField field, const wxString& newText);

		void OnDirectoryChanged(DirectoryEvent& e);
	};
}
