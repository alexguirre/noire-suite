#pragma once
#include "Common.h"
#include "File.h"
#include <memory>

namespace noire
{
	class RawFile : public File
	{
	public:
		RawFile(Device& parent, PathView path);

		std::shared_ptr<Stream> Stream();

	public:
		static const Type Type;
	};
}