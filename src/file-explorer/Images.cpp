#include "Images.h"
#include "resources/BlankFileIcon.xpm"
#include "resources/BlueFolderIcon.xpm"
#include "resources/FolderIcon.xpm"
#include "resources/NoireIcon.xpm"
#include <wx/icon.h>
#include <wx/imaglist.h>

namespace noire::explorer
{
	wxImageList* Images::Icons()
	{
		static wxImageList Icons{ 17, 17 };

		// clang-format off
		wxIcon icons[IconCount];
		icons[IconBlankFile]  = { BlankFileIcon };
		icons[IconBlueFolder] = { BlueFolderIcon };
		icons[IconFolder]     = { FolderIcon };
		icons[IconNoire]      = { NoireIcon };
		// clang-format on

		for (const wxIcon& i : icons)
		{
			Icons.Add(i);
		}

		return &Icons;
	}
}
