#include "Writer.h"
#include "Reader.h"
#include "Types.h"
#include "streams/FileStream.h"
#include "streams/Stream.h"
#include <doctest/doctest.h>

namespace noire::atb
{
	Writer::Writer(Stream& stream) : mStream{ stream } {}

	void Writer::Write(const Object& root)
	{
		// TODO: atb::Writer::Write
		(void)root;

		Stream& s = mStream;

		s.Write<u32>(HeaderMagic | 0x04000000); // value 04 is what the game's root.atb.pc

		s.Write<u16>(0); // tmp, root object count
	}
}

#ifndef DOCTEST_CONFIG_DISABLE
TEST_SUITE("atb::Writer")
{
	using namespace noire;
	using namespace noire::atb;

	TEST_CASE("Read/Write" * doctest::skip(false))
	{
		FileStream in{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\root.atb.pc" };
		const Object root = Reader{ in }.Read();

		FileStream out{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\root_copy.atb.pc" };
		Writer{ out }.Write(root);
	}
}
#endif
