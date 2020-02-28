#include "LocalDevice.h"
#include "files/File.h"
#include "files/RawFile.h"
#include "files/WAD.h"
#include "streams/FileStream.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <functional>
#include <iostream>
#include <string>

namespace noire
{
	namespace fs = std::filesystem;

	LocalDevice::LocalDevice(const fs::path& rootPath) : mRootPath{ fs::absolute(rootPath) }
	{
		Expects(fs::is_directory(mRootPath));
	}

	bool LocalDevice::Exists(PathView path) const
	{
		Expects(path.IsAbsolute());

		const fs::path fullPath = FullPath(path);
		return path.IsDirectory() ? fs::is_directory(fullPath) : fs::is_regular_file(fullPath);
	}

	std::shared_ptr<File> LocalDevice::Open(PathView path)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		if (Exists(path))
		{
			const size h = std::hash<PathView>{}(path);
			std::shared_ptr<File> file{ nullptr };
			if (auto it = mCachedFiles.find(h); it != mCachedFiles.end())
			{
				file = it->second;
			}
			else
			{
				FileStream f{ FullPath(path) };
				file = File::New(*this, path, false, File::FindTypeOfStream(f));
				mCachedFiles.try_emplace(h, file);
			}

			return file;
		}
		else
		{
			return nullptr;
		}
	}

	std::shared_ptr<File> LocalDevice::Create(PathView path, size fileTypeId)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		Expects(!Exists(path));

		std::shared_ptr file = File::New(*this, path, true, fileTypeId);
		mCachedFiles.try_emplace(std::hash<PathView>{}(path), file);
		return file;
	}

	bool LocalDevice::Delete(PathView path)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		return fs::remove(FullPath(path));
	}

	void LocalDevice::Visit(DeviceVisitCallback visitDirectory,
							DeviceVisitCallback visitFile,
							PathView path,
							bool recursive)
	{
		Expects(path.IsDirectory() && path.IsAbsolute());

		const fs::path fullPath = FullPath(path);
		Path tmpPath{}; // keep out of the lambda to reuse the string memory already allocated
		const auto cb = [&](const fs::directory_entry& e) {
			tmpPath = Path::Root;
			tmpPath += e.path().lexically_relative(mRootPath).string();
			tmpPath.Normalize();
			if (e.is_regular_file())
			{
				visitFile(tmpPath);
			}
			else // is directory
			{
				tmpPath += Path::DirectorySeparator;
				visitDirectory(tmpPath);
			}
		};

		recursive ? std::for_each(fs::recursive_directory_iterator{ fullPath }, {}, cb) :
					std::for_each(fs::directory_iterator{ fullPath }, {}, cb);
	}

	ReadOnlyStream LocalDevice::OpenStream(PathView path)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		return ReadOnlyStream{ std::make_unique<FileStream>(FullPath(path)) };
	}

	void LocalDevice::Commit()
	{
		for (auto& e : mCachedFiles)
		{
			if (auto f = e.second; f->HasChanged())
			{
				f->Save();
				FileStream fs{ FullPath(f->Path()) };
				f->Raw().CopyTo(fs);
			}
		}
	}

	fs::path LocalDevice::FullPath(PathView path) const
	{
		// remove root '/' from path view before concatenating
		return mRootPath / path.String().substr(1);
	}
}

TEST_SUITE("LocalDevice")
{
	using namespace noire;

	TEST_CASE("Visit" * doctest::skip(true))
	{
		LocalDevice d{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\" };
		std::cout << "Visiting '/' recursively\n";
		d.Visit([](PathView p) { std::cout << "\tVisit Directory: '" << p.String() << "'\n"; },
				[](PathView p) { std::cout << "\tVisit File:      '" << p.String() << "'\n"; },
				"/",
				true);

		std::cout << "Visiting '/final/pc/'\n";
		d.Visit([](PathView p) { std::cout << "\tVisit Directory: '" << p.String() << "'\n"; },
				[](PathView p) { std::cout << "\tVisit File:      '" << p.String() << "'\n"; },
				"/final/pc/",
				true);
	}

	TEST_CASE("Open" * doctest::skip(true))
	{
		LocalDevice d{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\" };
		std::shared_ptr f = d.Open("/test/out.wad.pc");
		CHECK(f != nullptr);
		std::shared_ptr w = std::dynamic_pointer_cast<WAD>(f);
		CHECK(w != nullptr);

		w->Load();

		std::cout << "Visiting WAD's '/' recursively\n";
		w->Visit([](PathView p) { std::cout << "\tVisit Directory: '" << p.String() << "'\n"; },
				 [](PathView p) { std::cout << "\tVisit File:      '" << p.String() << "'\n"; },
				 "/",
				 true);
	}

	TEST_CASE("Create" * doctest::skip(true))
	{
		LocalDevice d{ "E:\\Rockstar Games\\L.A. Noire Complete Edition\\" };
		std::shared_ptr f = d.Create("/test/my_custom_file.txt", File::Type.Id);
		CHECK(f != nullptr);
		std::shared_ptr r = std::dynamic_pointer_cast<File>(f);
		CHECK(r != nullptr);

		r->Load();

		const std::string str{ "hello world" };
		r->Raw().Write(str.data(), str.size());

		r->Save();

		d.Commit();
	}
}
