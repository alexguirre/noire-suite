#pragma once
#include <core/Path.h>
#include <wx/treectrl.h>

namespace noire::explorer
{
	class DirectoryItemData : public wxTreeItemData
	{
	public:
		DirectoryItemData(PathView path) : mPath{ path } {}

		PathView Path() const { return mPath; }

	private:
		noire::Path mPath;
	};

	class DirectoryTreeCtrl : public wxTreeCtrl
	{
	public:
		DirectoryTreeCtrl(wxWindow* parent,
						  const wxWindowID id,
						  const wxPoint& pos = wxDefaultPosition,
						  const wxSize& size = wxDefaultSize);

		void Refresh();
	};
}
