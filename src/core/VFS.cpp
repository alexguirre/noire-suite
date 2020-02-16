#include "VFS.h"
#include "Common.h"
#include "files/File.h"
#include "streams/Stream.h"
#include "streams/TempStream.h"
#include <doctest/doctest.h>
#include <functional>
#include <iostream>
#include <utility>

namespace noire
{
	VirtualFileSystem::VirtualFileSystem() : mEntries{}, mRoot{ GetDirectory(PathView::Root, true) }
	{
	}

	std::shared_ptr<File> VirtualFileSystem::Open(PathView path, OpenCallback cb)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		if (Entry* e = FindEntry(path); e)
		{
			Ensures(e->Type() == EntryType::File);

			FileEntry* fe = static_cast<FileEntry*>(e);
			std::shared_ptr<File>& f = fe->File();
			if (f == nullptr)
			{
				f = cb(path, fe->Info());
			}

			return f;
		}

		return nullptr;
	}

	std::shared_ptr<File>
	VirtualFileSystem::Create(PathView path, size fileTypeId, CreateCallback cb)
	{
		Expects(path.IsFile() && path.IsAbsolute());
		Expects(FindEntry(path) == nullptr);

		FileEntryInfo info = cb(path);
		RegisterExistingFile(path, std::move(info));

		return File::NewEmpty(fileTypeId);
	}

	bool VirtualFileSystem::Exists(PathView path) const
	{
		Expects(path.IsAbsolute());

		return FindEntry(path) != nullptr;
	}

	bool VirtualFileSystem::Delete(PathView filePath)
	{
		Expects(filePath.IsFile() && filePath.IsAbsolute());

		const size hash = std::hash<PathView>{}(filePath);

		auto it = mEntries.find(hash);
		if (it != mEntries.end())
		{
			DirectoryEntry* parent = GetDirectory(filePath.Parent(), false);
			Ensures(parent != nullptr);

			parent->RemoveChild(it->second.get());

			mEntries.erase(it);
			return true;
		}
		else
		{
			return false;
		}
	}

	VirtualFileSystem::FileEntryInfo VirtualFileSystem::GetFileInfo(PathView path) const
	{
		Expects(path.IsFile() && path.IsAbsolute());

		Entry* e = FindEntry(path);
		Expects(e != nullptr);
		Ensures(e->Type() == EntryType::File);

		return static_cast<FileEntry*>(e)->Info();
	}

	void VirtualFileSystem::RegisterExistingFile(PathView path, FileEntryInfo info)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		// TODO: provide override option?
		Expects(FindEntry(path) == nullptr); // path is not registered yet

		DirectoryEntry* parent = GetDirectory(path.Parent(), true);
		Ensures(parent != nullptr);
		auto entry = std::make_unique<FileEntry>(path, parent, std::move(info));
		if (auto p = mEntries.emplace(std::hash<PathView>{}(path), std::move(entry)); p.second)
		{
			Ensures(p.first->second->Type() == EntryType::File);
		}
	}

	void VirtualFileSystem::Visit(VisitCallback visitDirectory,
								  VisitCallback visitFile,
								  PathView dirPath,
								  bool recursive)
	{
		DirectoryEntry* dir = GetDirectory(dirPath, false);
		if (dir)
		{
			VisitDirectory(dir, Path{ dirPath }, visitDirectory, visitFile, recursive);
		}
	}

	void VirtualFileSystem::VisitDirectory(DirectoryEntry* dir,
										   Path dirPath,
										   const VisitCallback& visitDirectory,
										   const VisitCallback& visitFile,
										   bool recursive)
	{
		for (Entry* e : dir->Children())
		{
			switch (e->Type())
			{
			case EntryType::File: visitFile(dirPath / e->Name()); break;
			case EntryType::Directory:
			{
				Path p = dirPath / e->Name() + Path::DirectorySeparator;
				visitDirectory(p);
				if (recursive)
				{
					VisitDirectory(static_cast<DirectoryEntry*>(e),
								   std::move(p),
								   visitDirectory,
								   visitFile,
								   recursive);
				}
				// TODO: another callback after visiting directory contents may be needed
				break;
			}
			default: Ensures(false); break;
			}
		}
	}

	VirtualFileSystem::Entry* VirtualFileSystem::FindEntry(PathView path) const
	{
		Expects(path.IsAbsolute());

		const size hash = std::hash<PathView>{}(path);

		auto it = mEntries.find(hash);
		if (it != mEntries.end())
		{
			Ensures(it->second != nullptr);
			return it->second.get();
		}
		else
		{
			return nullptr;
		}
	}

	VirtualFileSystem::DirectoryEntry* VirtualFileSystem::GetDirectory(PathView path, bool create)
	{
		Expects(path.IsDirectory() && path.IsAbsolute());

		Entry* e = FindEntry(path);
		if (e)
		{
			Ensures(e->Type() == EntryType::Directory);
			return static_cast<DirectoryEntry*>(e);
		}
		else if (create)
		{
			DirectoryEntry* parent = path.IsRoot() ? nullptr : GetDirectory(path.Parent(), true);
			auto newEntry = std::make_unique<DirectoryEntry>(path, parent);
			if (auto p = mEntries.emplace(std::hash<PathView>{}(path), std::move(newEntry));
				p.second)
			{
				Ensures(p.first->second->Type() == EntryType::Directory);
				return static_cast<DirectoryEntry*>(p.first->second.get());
			}
		}

		return nullptr;
	}

	VirtualFileSystem::Entry::Entry(PathView path, EntryType type, DirectoryEntry* parent)
		: mName{ path.Name() }, mType{ type }, mParent{ parent }
	{
		if (parent)
		{
			parent->AddChild(this);
		}
	}

	VirtualFileSystem::DirectoryEntry::DirectoryEntry(PathView path, DirectoryEntry* parent)
		: Entry(path, EntryType::Directory, parent)
	{
		Expects(path.IsDirectory() && path.IsAbsolute());
	}

	void VirtualFileSystem::DirectoryEntry::AddChild(Entry* e)
	{
		Expects(e != nullptr && e->Parent() == this);

		mChildren.push_back(e);
	}

	void VirtualFileSystem::DirectoryEntry::RemoveChild(Entry* e)
	{
		Expects(e != nullptr && e->Parent() == this);

		mChildren.erase(std::find(mChildren.begin(), mChildren.end(), e));
	}

	VirtualFileSystem::FileEntry::FileEntry(PathView path,
											DirectoryEntry* parent,
											FileEntryInfo info)
		: Entry(path, EntryType::File, parent), mInfo{ std::move(info) }, mFile{ nullptr }
	{
		Expects(path.IsFile() && path.IsAbsolute());
		Expects(parent != nullptr);
	}
}

TEST_SUITE("VFS")
{
	using namespace noire;

	TEST_CASE("Simple")
	{
		VirtualFileSystem vfs{};

		vfs.RegisterExistingFile("/test1", 1);
		vfs.RegisterExistingFile("/test2", 2);
		vfs.RegisterExistingFile("/test3", 3);
		vfs.RegisterExistingFile("/test4", 4);
		vfs.RegisterExistingFile("/inside/folder/test5", 5);
		vfs.RegisterExistingFile("/inside/folder/test6", 6);
		vfs.RegisterExistingFile("/inside/folder/deep/test7", 7);
		vfs.RegisterExistingFile("/inside/folder/deep/deeper/test8", 8);

		std::cout << "Visiting '" << PathView::Root.String() << "' recursively\n";
		vfs.Visit([](PathView p) { std::cout << "Visited Directory '" << p.String() << "'\n"; },
				  [](PathView p) { std::cout << "Visited File      '" << p.String() << "'\n"; },
				  PathView::Root);
		std::cout << "Visiting '"
				  << "/inside/folder/"
				  << "\n";
		vfs.Visit([](PathView p) { std::cout << "Visited Directory '" << p.String() << "'\n"; },
				  [](PathView p) { std::cout << "Visited File      '" << p.String() << "'\n"; },
				  "/inside/folder/",
				  false);

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
	}
}