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

		void Save(noire::Stream& output) override;
		u64 Size() override;

	private:
		std::shared_ptr<noire::Stream> mStream;

	public:
		static const Type Type;
	};
}