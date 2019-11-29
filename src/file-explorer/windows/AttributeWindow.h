#pragma once
#include <memory>
#include <wx/frame.h>

namespace noire
{
	class CAttributeFile;
}

class wxPropertyGridEvent;

class CAttributeWindow : public wxFrame
{
public:
	CAttributeWindow(wxWindow* parent,
					 wxWindowID id,
					 const wxString& title,
					 std::unique_ptr<noire::CAttributeFile> file);

private:
	std::unique_ptr<noire::CAttributeFile> mFile;
};