#pragma once
#include "FileStream.h"
#include <memory>
#include <string_view>

namespace noire::fs
{
	class IDevice
	{
	public:
		virtual bool PathExists(std::string_view path) const = 0;
		virtual std::unique_ptr<IFileStream> OpenFile(std::string_view path) = 0;
	};
}
