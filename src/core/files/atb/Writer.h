#pragma once

namespace noire
{
	class Stream;
}

namespace noire::atb
{
	struct Object;

	struct Writer
	{
		Writer(Stream& stream);

		void Write(const Object& root);

	private:
		Stream& mStream;
	};
}
