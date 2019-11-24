#pragma once
#include <memory>
#include <optional>
#include <wx/frame.h>
#include <wx/string.h>

namespace noire
{
	class CShaderProgramFile;
}

class wxStyledTextCtrl;
class wxPropertyGridEvent;

class CShaderProgramWindow : public wxFrame
{
public:
	CShaderProgramWindow(wxWindow* parent,
						 wxWindowID id,
						 const wxString& title,
						 std::unique_ptr<noire::CShaderProgramFile> file);

private:
	void OnPropertyGridSelected(wxPropertyGridEvent& event);

	std::unique_ptr<noire::CShaderProgramFile> mFile;
	wxStyledTextCtrl* mDisassemblyCtrl;
	int mCurrentDisassembly;
	std::optional<wxString> mVertexShaderDisassembly;
	std::optional<wxString> mPixelShaderDisassembly;
};