#include "Path.h"
#include <algorithm>
#include <doctest/doctest.h>

namespace noire
{
	const PathView PathView::Root{ std::string_view{ &PathView::DirectorySeparator, 1 } };
	const Path Path::Root{ PathView::Root };

	Path& Path::Append(const PathView path)
	{
		if (path.IsEmpty())
		{
			return *this;
		}

		Expects(path.IsRelative());

		const bool thisIsDir = IsDirectory();
		mPathStr.reserve(mPathStr.size() + (thisIsDir ? 0 : 1) + path.String().size());
		if (!thisIsDir)
		{
			mPathStr += Path::DirectorySeparator;
		}
		mPathStr += path.String();
		return *this;
	}

	Path& Path::Concat(const std::string_view str)
	{
		mPathStr.reserve(mPathStr.size() + str.size());
		mPathStr += str;
		return *this;
	}

	Path& Path::Concat(const char c)
	{
		mPathStr.reserve(mPathStr.size() + 1);
		mPathStr += c;
		return *this;
	}

	Path& Path::Normalize()
	{
		std::replace(mPathStr.begin(), mPathStr.end(), '\\', Path::DirectorySeparator);

		// remove repeated directory separators
		auto it = mPathStr.begin();
		do
		{
			it = std::adjacent_find(it, mPathStr.end(), [](char a, char b) {
				return a == Path::DirectorySeparator && b == Path::DirectorySeparator;
			});

			if (it == mPathStr.end())
			{
				break;
			}
			else
			{
				it = mPathStr.erase(it);
			}

		} while (!mPathStr.empty());

		return *this;
	}
}

TEST_SUITE("PathView")
{
	using namespace noire;

	PathView v{};

	TEST_CASE("IsDirectory/IsFile")
	{
		v = "/";
		CHECK(v.IsDirectory());
		CHECK_FALSE(v.IsFile());

		v = "file";
		CHECK_FALSE(v.IsDirectory());
		CHECK(v.IsFile());

		v = "/file";
		CHECK_FALSE(v.IsDirectory());
		CHECK(v.IsFile());

		v = "/dir/";
		CHECK(v.IsDirectory());
		CHECK_FALSE(v.IsFile());

		v = "/dir/subfile";
		CHECK_FALSE(v.IsDirectory());
		CHECK(v.IsFile());

		v = "/dir/subdir/";
		CHECK(v.IsDirectory());
		CHECK_FALSE(v.IsFile());
	}

	TEST_CASE("IsRoot")
	{
		v = "/";
		CHECK(v.IsRoot());

		v = "/file";
		CHECK_FALSE(v.IsRoot());

		v = "/dir/";
		CHECK_FALSE(v.IsRoot());

		v = "relative_file";
		CHECK_FALSE(v.IsRoot());

		v = "relative_dir/";
		CHECK_FALSE(v.IsRoot());

		CHECK(PathView::Root.IsRoot());
		CHECK(Path::Root.IsRoot());
	}

	TEST_CASE("Parent")
	{
		v = "relative_file";
		CHECK_FALSE(v.HasParent());
		CHECK(v.Parent().IsEmpty());

		v = "/";
		CHECK_FALSE(v.HasParent());
		CHECK(v.Parent().IsEmpty());

		v = "/a";
		CHECK(v.HasParent());
		CHECK_EQ(v.Parent().String(), "/");

		v = "/dir/";
		CHECK(v.HasParent());
		CHECK_EQ(v.Parent().String(), "/");

		v = "relative_dir/";
		CHECK_FALSE(v.HasParent());
		CHECK(v.Parent().IsEmpty());

		v = "relative_dir/subfile";
		CHECK(v.HasParent());
		CHECK_EQ(v.Parent().String(), "relative_dir/");

		v = "/dir/subfile";
		CHECK(v.HasParent());
		CHECK_EQ(v.Parent().String(), "/dir/");

		v = "relative_dir/subdir/";
		CHECK(v.HasParent());
		CHECK_EQ(v.Parent().String(), "relative_dir/");

		v = "/dir/subdir/";
		CHECK(v.HasParent());
		CHECK_EQ(v.Parent().String(), "/dir/");
	}

	TEST_CASE("Name")
	{
		v = "relative_file";
		CHECK_EQ(v.Name(), "relative_file");

		v = "/";
		CHECK_EQ(v.Name(), "");

		v = "/a";
		CHECK_EQ(v.Name(), "a");

		v = "/dir/";
		CHECK_EQ(v.Name(), "dir");

		v = "relative_dir/";
		CHECK_EQ(v.Name(), "relative_dir");

		v = "relative_dir/subfile";
		CHECK_EQ(v.Name(), "subfile");

		v = "/dir/subfile";
		CHECK_EQ(v.Name(), "subfile");

		v = "relative_dir/subdir/";
		CHECK_EQ(v.Name(), "subdir");

		v = "/dir/subdir/";
		CHECK_EQ(v.Name(), "subdir");
	}

	TEST_CASE("RelativeTo")
	{
		v = "/file";
		CHECK_EQ(v.RelativeTo("/").String(), "file");
		CHECK(v.RelativeTo("").IsEmpty());

		v = "/dir/";
		CHECK_EQ(v.RelativeTo("/").String(), "dir/");
		CHECK(v.RelativeTo("").IsEmpty());

		v = "/some/deep/file";
		CHECK_EQ(v.RelativeTo("/").String(), "some/deep/file");
		CHECK_EQ(v.RelativeTo("/some/").String(), "deep/file");
		CHECK_EQ(v.RelativeTo("/some/deep/").String(), "file");

		v = "/some/deep/dir/";
		CHECK_EQ(v.RelativeTo("/").String(), "some/deep/dir/");
		CHECK_EQ(v.RelativeTo("/some/").String(), "deep/dir/");
		CHECK_EQ(v.RelativeTo("/some/deep/").String(), "dir/");
		CHECK(v.RelativeTo("/some/deep/dir/").IsEmpty());

		v = "relative_dir/file";
		CHECK_EQ(v.RelativeTo("relative_dir/").String(), "file");
		CHECK_EQ(v.RelativeTo("").String(), "relative_dir/file");
	}
}

TEST_SUITE("Path")
{
	using namespace noire;

	Path p{};

	TEST_CASE("Append")
	{
		// TODO: Path::Append tests
	}

	TEST_CASE("Concat")
	{
		// TODO: Path::Concat tests
	}

	TEST_CASE("Normalize")
	{
		SUBCASE("Replace '\\'")
		{
			p = "some\\dir\\with\\files";
			p.Normalize();
			CHECK_EQ(p.String(), "some/dir/with/files");
		}

		SUBCASE("Remove repeated '/'")
		{
			p = "some/dir//with////files///////////";
			p.Normalize();
			CHECK_EQ(p.String(), "some/dir/with/files/");
		}

		SUBCASE("Replace '\\' and remove repeated '/'")
		{
			p = "some\\dir//with\\\\\\\\files///////////\\";
			p.Normalize();
			CHECK_EQ(p.String(), "some/dir/with/files/");
		}
	}
}