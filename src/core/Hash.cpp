#include "Hash.h"
#include <doctest/doctest.h>

TEST_SUITE("crc32")
{
	using namespace noire;

	TEST_CASE("case sensitive")
	{
		CHECK_EQ(0x00000000, crc32(""));
		CHECK_EQ(0x8B8838C2, crc32("abcdxyz"));
		CHECK_EQ(0xB4CDC6D8, crc32("ABCDXYZ"));
		CHECK_EQ(0xFC1BD0B1, crc32("AaBbCcDdXxYyZz"));
	}

	TEST_CASE("lowercase")
	{
		CHECK_EQ(0x00000000, crc32Lowercase(""));
		CHECK_EQ(0x8B8838C2, crc32Lowercase("abcdxyz"));
		CHECK_EQ(0x8B8838C2, crc32Lowercase("ABCDXYZ"));
		CHECK_EQ(0xAD7F9CBD, crc32Lowercase("AaBbCcDdXxYyZz"));
	}
}
