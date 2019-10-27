#pragma once
#include <wx/treectrl.h>

class CFileTreeCtrl : public wxTreeCtrl
{
public:
	CFileTreeCtrl(wxWindow* parent, const wxWindowID id, const wxPoint& pos, const wxSize& size);

private:
	enum Icon
	{
		IconBlankFile,
		IconBlueFolder,
		IconFolder,
		IconNoire,

		IconCount,
	};

	int mIconIds[IconCount];

	void LoadIcons();
	void LoadDummyData();
};