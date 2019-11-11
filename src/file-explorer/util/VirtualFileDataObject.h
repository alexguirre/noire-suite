#pragma once
#include <cstdint>
#include <formats/fs/Path.h>
#include <wx/dataobj.h>

namespace noire::fs
{
	class CFileSystem;
}

class CVirtualFileDataObject : public wxDataObject
{
public:
	CVirtualFileDataObject(noire::fs::CFileSystem* fileSystem, noire::fs::SPathView path);

	void GetAllFormats(wxDataFormat* formats, Direction dir = Get) const override;
	bool GetDataHere(const wxDataFormat& format, void* buff) const override;
	std::size_t GetDataSize(const wxDataFormat& format) const override;
	std::size_t GetFormatCount(Direction dir = Get) const override;
	wxDataFormat GetPreferredFormat(Direction dir = Get) const override;
	bool SetData(const wxDataFormat& format, std::size_t len, const void* buff) override;
};