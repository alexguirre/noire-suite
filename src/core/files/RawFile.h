#pragma once
#include "Common.h"
#include "File.h"
#include <memory>

namespace noire
{
	class RawFile : public File
	{
	public:
		RawFile();
		RawFile(std::shared_ptr<Stream> input);

		Stream& Stream();

	public:
		static const Type Type;
	};
}