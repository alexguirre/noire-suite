#include "ShaderProgramWindow.h"
#include <gsl/gsl>
#include <wx/log.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/props.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stc/stc.h>

// constants to know the currently shown disassembly
// static constexpr int DisassemblyNone = -1;
// static constexpr int DisassemblyVertexShader = 0;
// static constexpr int DisassemblyPixelShader = 1;
//
// CShaderProgramWindow::CShaderProgramWindow(wxWindow* parent,
//										   wxWindowID id,
//										   const wxString& title,
//										   std::unique_ptr<noire::CShaderProgramFile> file)
//	: wxFrame(parent, id, title),
//	  mFile{ std::move(file) },
//	  mCurrentDisassembly{ DisassemblyNone },
//	  mVertexShaderDisassembly{ std::nullopt },
//	  mPixelShaderDisassembly{ std::nullopt }
//{
//	Expects(mFile != nullptr);
//
//	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
//
//	wxSplitterWindow* splitter =
//		new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
//	splitter->SetSashGravity(0.0);
//	splitter->SetMinimumPaneSize(50);
//	mainSizer->Add(splitter, 1, wxEXPAND);
//
//	mDisassemblyCtrl = new wxStyledTextCtrl(splitter, wxID_ANY);
//	wxPropertyGrid* propGrid = new wxPropertyGrid(splitter,
//												  wxID_ANY,
//												  wxDefaultPosition,
//												  wxDefaultSize,
//												  wxPG_SPLITTER_AUTO_CENTER | wxPG_DEFAULT_STYLE);
//
//	splitter->SplitVertically(propGrid, mDisassemblyCtrl, 250);
//
//	SetSizer(mainSizer);
//
//	// TODO: set lexer
//	mDisassemblyCtrl->SetReadOnly(true);
//
//	const auto appendShaderProp = [propGrid](const wxString& name,
//											 const noire::CShaderProgramFile::Shader& shader) {
//		wxPGProperty* prop = propGrid->Append(new wxStringProperty(name, wxPG_LABEL, "<composed>"));
//
//		propGrid->AppendIn(prop, new wxStringProperty("Name", wxPG_LABEL, shader.Name));
//		propGrid->AppendIn(prop, new wxUIntProperty("Unk08", wxPG_LABEL, shader.Unk08));
//		propGrid->AppendIn(prop, new wxUIntProperty("Size", wxPG_LABEL, shader.Bytecode.size()));
//
//		propGrid->SetPropertyReadOnly(prop, true);
//	};
//
//	propGrid->Append(new wxPropertyCategory("Shader Program"));
//	appendShaderProp("Vertex Shader", mFile->VertexShader());
//	appendShaderProp("Pixel Shader", mFile->PixelShader());
//
//	propGrid->Bind(wxEVT_PG_SELECTED, &CShaderProgramWindow::OnPropertyGridSelected, this);
//
//	SetSize(800, 500);
//}
//
// void CShaderProgramWindow::OnPropertyGridSelected(wxPropertyGridEvent& event)
//{
//	wxPGProperty* topParent = event.GetMainParent();
//	wxString name = topParent->GetName();
//
//	wxLogDebug("%s: %s", __FUNCTION__, name);
//
//	const wxString* disassembly = nullptr;
//	if (name.StartsWith("Vertex"))
//	{
//		if (mCurrentDisassembly == DisassemblyVertexShader)
//		{
//			// already showing the vertex shader disassembly
//			return;
//		}
//
//		if (!mVertexShaderDisassembly.has_value())
//		{
//			mVertexShaderDisassembly = mFile->VertexShader().Disassemble();
//		}
//
//		disassembly = &mVertexShaderDisassembly.value();
//		mCurrentDisassembly = DisassemblyVertexShader;
//	}
//	else if (name.StartsWith("Pixel"))
//	{
//		if (mCurrentDisassembly == DisassemblyPixelShader)
//		{
//			// already showing the pixel shader disassembly
//			return;
//		}
//
//		if (!mPixelShaderDisassembly.has_value())
//		{
//			mPixelShaderDisassembly = mFile->PixelShader().Disassemble();
//		}
//
//		disassembly = &mPixelShaderDisassembly.value();
//		mCurrentDisassembly = DisassemblyPixelShader;
//	}
//	else
//	{
//		// not selected a shader property, probably the top category
//		return;
//	}
//
//	Expects(disassembly != nullptr);
//	mDisassemblyCtrl->SetReadOnly(false);
//	mDisassemblyCtrl->SetText(*disassembly);
//	mDisassemblyCtrl->SetReadOnly(true);
//}
