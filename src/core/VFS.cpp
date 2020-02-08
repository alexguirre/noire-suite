#include "VFS.h"
#include "Common.h"
#include <doctest/doctest.h>
#include <iostream>

TEST_SUITE("VFS")
{
	using namespace noire;

	VirtualFileSystem<i32> vfs{};

	TEST_CASE("")
	{
		vfs.RegisterFile("/test1", 1);
		vfs.RegisterFile("/test2", 2);
		vfs.RegisterFile("/test3", 3);
		vfs.RegisterFile("/test4", 4);

		vfs.ForEachFile("/", [](PathView p, const i32& d) {
			std::cout << p.String() << ": " << d << '\n';
		});

		CHECK(vfs.Exists("/"));
		CHECK(vfs.Exists("/test1"));
		CHECK(vfs.Exists("/test2"));
		CHECK(vfs.Exists("/test3"));
		CHECK(vfs.Exists("/test4"));
		CHECK_FALSE(vfs.Exists("/test5"));
	}
}