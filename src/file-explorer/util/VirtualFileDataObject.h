#pragma once
#include <core/Path.h>
#include <cstdint>
#include <vector>
#include <wx/dataobj.h>

namespace noire
{
	class Device;

	namespace explorer
	{
		class VirtualFileDataObject : public wxDataObject
		{
		public:
			VirtualFileDataObject(Device& device, const std::vector<PathView>& paths);

			void GetAllFormats(wxDataFormat* formats, Direction dir = Get) const override;
			bool GetDataHere(const wxDataFormat& format, void* buff) const override;
			std::size_t GetDataSize(const wxDataFormat& format) const override;
			std::size_t GetFormatCount(Direction dir = Get) const override;
			wxDataFormat GetPreferredFormat(Direction dir = Get) const override;
			bool SetData(const wxDataFormat& format, std::size_t len, const void* buff) override;
		};
	}
}
