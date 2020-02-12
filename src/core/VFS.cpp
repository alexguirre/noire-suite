#include "VFS.h"
#include "Common.h"
#include <doctest/doctest.h>
#include <iostream>

namespace noire
{
	//VFSFileEntryStream::VFSFileEntryStream(std::shared_ptr<ReadOnlyStream> input)
	//	: mInput{ input }, mOutput{ std::nullopt }
	//{
	//	Expects(input != nullptr);
	//}

	//u64 VFSFileEntryStream::Read(void* dstBuffer, u64 count)
	//{
	//	return Current().Read(dstBuffer, count);
	//}

	//u64 VFSFileEntryStream::ReadAt(void* dstBuffer, u64 count, u64 offset)
	//{
	//	return Current().ReadAt(dstBuffer, count, offset);
	//}

	//u64 VFSFileEntryStream::Write(const void* buffer, u64 count)
	//{
	//	UseOutputStream();
	//	return Current().Write(buffer, count);
	//}

	//u64 VFSFileEntryStream::WriteAt(const void* buffer, u64 count, u64 offset)
	//{
	//	UseOutputStream();
	//	return Current().WriteAt(buffer, count, offset);
	//}

	//u64 VFSFileEntryStream::Seek(i64 offset, StreamSeekOrigin origin)
	//{
	//	return Current().Seek(offset, origin);
	//}

	//u64 VFSFileEntryStream::Tell() { return Current().Tell(); }

	//u64 VFSFileEntryStream::Size() { return Current().Size(); }

	//void VFSFileEntryStream::UseOutputStream()
	//{
	//	if (!mOutput.has_value())
	//	{
	//		Stream& i = *mInput;
	//		Stream& o = mOutput.emplace();

	//		i.CopyTo(o);
	//		o.Seek(gsl::narrow<i64>(i.Tell()), StreamSeekOrigin::Begin);

	//		Ensures(i.Size() == o.Size());

	//		mInput.reset();
	//	}
	//}

	//Stream& VFSFileEntryStream::Current()
	//{
	//	if (mOutput.has_value())
	//	{
	//		return *mOutput;
	//	}

	//	return *mInput;
	//}
}

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