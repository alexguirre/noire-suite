#pragma once
#include "Common.h"
#include <memory>

namespace noire
{
	class Stream;

	class File
	{
	public:
		File(std::shared_ptr<Stream> input);

		virtual ~File() = default;

		virtual void Load();
		// default Save() writes the contents of the input stream to the output stream
		virtual void Save(Stream& output);

	protected:
		std::shared_ptr<Stream> Input() { return mInput; }

	private:
		std::shared_ptr<Stream> mInput;
	};
}