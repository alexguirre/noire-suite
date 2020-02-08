#include "VFS.h"
#include "Common.h"
#include <doctest/doctest.h>
#include <iostream>

TEST_SUITE("VFS")
{
	using namespace noire;

	TEST_CASE("Simple")
	{
		VirtualFileSystem<i32> vfs{};

		vfs.RegisterExistingFile("/test1", 1);
		vfs.RegisterExistingFile("/test2", 2);
		vfs.RegisterExistingFile("/test3", 3);
		vfs.RegisterExistingFile("/test4", 4);

		{
			const i32 expected[4]{ 1, 2, 3, 4 };
			size i = 0;
			vfs.ForEachFile("/", [&i, &expected](PathView p, const i32& d) {
				CHECK_EQ(expected[i], d);
				i++;

				std::cout << p.String() << ": " << d << '\n';
			});
		}

		CHECK(vfs.Exists("/"));
		CHECK(vfs.Exists("/test1"));
		CHECK(vfs.Exists("/test2"));
		CHECK(vfs.Exists("/test3"));
		CHECK(vfs.Exists("/test4"));
		CHECK_FALSE(vfs.Exists("/test5"));

		CHECK(vfs.Delete("/test4"));
		CHECK_FALSE(vfs.Exists("/test4"));

		CHECK(vfs.Delete("/test1"));
		CHECK_FALSE(vfs.Exists("/test1"));

		CHECK_FALSE(vfs.Delete("/test5"));

		std::cout << "==============\n";
		{
			const i32 expected[2]{ 2, 3 };
			size i = 0;
			vfs.ForEachFile("/", [&i, &expected](PathView p, const i32& d) {
				CHECK_EQ(expected[i], d);
				i++;

				std::cout << p.String() << ": " << d << '\n';
			});
		}
	}
}