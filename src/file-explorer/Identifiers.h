#pragma once
#include <wx/defs.h>

namespace noire::explorer
{
	inline constexpr wxWindowID FileTreeExportMenuId = wxID_HIGHEST + 1;
	inline constexpr wxWindowID DirectoryContentsExportMenuId = FileTreeExportMenuId + 1;
	inline constexpr wxWindowID MenuBarOpenFolderId = DirectoryContentsExportMenuId + 1;
	inline constexpr wxWindowID MenuBarHashLookupId = MenuBarOpenFolderId + 1;
	inline constexpr wxWindowID MenuBarTestRendererId = MenuBarHashLookupId + 1;
}