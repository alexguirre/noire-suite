#pragma once
#include "Common.h"
#include "Path.h"
#include "streams/Stream.h"
#include "streams/TempStream.h"
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace noire
{
	class File;

	template<class T>
	class VirtualFileSystem
	{
	public:
		using FileEntryInfo = T;

	private:
		enum class EntryType
		{
			None = 0,
			Directory,
			File,
		};

		class DirectoryEntry;

		class Entry
		{
		public:
			Entry(PathView path, EntryType type, DirectoryEntry* parent);
			virtual ~Entry() = default;

			const std::string& Name() const { return mName; }
			EntryType Type() const { return mType; }
			DirectoryEntry* Parent() const { return mParent; }

		private:
			std::string mName;
			EntryType mType;
			DirectoryEntry* mParent;
		};

		class DirectoryEntry final : public Entry
		{
		public:
			DirectoryEntry(PathView path, DirectoryEntry* parent);

			void AddChild(Entry* e);
			void RemoveChild(Entry* e);

			const std::vector<Entry*>& Children() const { return mChildren; }

		private:
			std::vector<Entry*> mChildren;
		};

		class FileEntry final : public Entry
		{
		public:
			FileEntry(PathView path, DirectoryEntry* parent, FileEntryInfo info);

			const FileEntryInfo& Info() const { return mInfo; }
			std::shared_ptr<File>& File() { return mFile; };

		private:
			FileEntryInfo mInfo;
			std::shared_ptr<noire::File> mFile;
		};

	public:
		VirtualFileSystem();
		VirtualFileSystem(const VirtualFileSystem&) = delete;
		VirtualFileSystem(VirtualFileSystem&&) = default;

		VirtualFileSystem& operator=(const VirtualFileSystem&) = delete;
		VirtualFileSystem& operator=(VirtualFileSystem&&) = default;

		// template<class F> // F equivalent to std::shared_ptr<Stream>(PathView, const
		// FileEntryInfo&)
		using OpenCallback =
			std::function<std::shared_ptr<File>(PathView filePath, const FileEntryInfo& info)>;
		using CreateCallback = std::function<FileEntryInfo(PathView filePath)>;

		std::shared_ptr<File> Open(PathView path, OpenCallback cb);

		std::shared_ptr<File> Create(PathView path, size fileTypeId, CreateCallback cb);

		bool Exists(PathView path) const;

		// NOTE: only supports deleting files, returns whether the file existed and was deleted
		bool Delete(PathView filePath);

		const FileEntryInfo& GetFileInfo(PathView path) const;
		void RegisterExistingFile(PathView path, FileEntryInfo info = FileEntryInfo{});

		// TODO: make ForEachFile recursive
		bool
		ForEachFile(PathView dirPath,
					std::function<void(PathView path, const FileEntryInfo& fileInfo)> callback);

	private:
		Entry* FindEntry(PathView path) const;
		DirectoryEntry* GetDirectory(PathView path, bool create);

		std::unordered_map<std::size_t, std::unique_ptr<Entry>> mEntries;
		DirectoryEntry* mRoot;
	};

	template<class T>
	VirtualFileSystem<T>::VirtualFileSystem()
		: mEntries{}, mRoot{ GetDirectory(PathView::Root, true) }
	{
	}

	template<class T>
	std::shared_ptr<File> VirtualFileSystem<T>::Open(PathView path, OpenCallback cb)
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

	template<class T>
	std::shared_ptr<File>
	VirtualFileSystem<T>::Create(PathView path, size fileTypeId, CreateCallback cb)
	{
		Expects(path.IsFile() && path.IsAbsolute());
		Expects(FindEntry(path) == nullptr);

		FileEntryInfo info = cb(path);
		RegisterExistingFile(path, std::move(info));

		return File::NewEmpty(fileTypeId);
	}

	template<class T>
	bool VirtualFileSystem<T>::Exists(PathView path) const
	{
		Expects(path.IsAbsolute());

		return FindEntry(path) != nullptr;
	}

	template<class T>
	bool VirtualFileSystem<T>::Delete(PathView filePath)
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

	template<class T>
	typename const VirtualFileSystem<T>::FileEntryInfo&
	VirtualFileSystem<T>::GetFileInfo(PathView path) const
	{
		Expects(path.IsFile() && path.IsAbsolute());

		Entry* e = FindEntry(path);
		Expects(e != nullptr);
		Ensures(e->Type() == EntryType::File);

		return static_cast<FileEntry*>(e)->Info();
	}

	template<class T>
	void VirtualFileSystem<T>::RegisterExistingFile(PathView path, FileEntryInfo info)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		// TODO: provide override option?
		Expects(FindEntry(path) == nullptr); // path is not registerted yet

		DirectoryEntry* parent = GetDirectory(path.Parent(), true);
		Ensures(parent != nullptr);
		auto entry = std::make_unique<FileEntry>(path, parent, std::move(info));
		if (auto p = mEntries.emplace(std::hash<PathView>{}(path), std::move(entry)); p.second)
		{
			Ensures(p.first->second->Type() == EntryType::File);
		}
	}

	template<class T>
	bool VirtualFileSystem<T>::ForEachFile(
		PathView dirPath,
		std::function<void(PathView path, const FileEntryInfo& fileInfo)> callback)
	{
		Expects(dirPath.IsDirectory() && dirPath.IsAbsolute());

		DirectoryEntry* dir = GetDirectory(dirPath, false);
		if (dir)
		{
			for (Entry* e : dir->Children())
			{
				if (e->Type() == EntryType::File)
				{
					callback(Path{ dirPath } / e->Name(), static_cast<FileEntry*>(e)->Info());
				}
			}

			return true;
		}
		else
		{
			return false;
		}
	}

	template<class T>
	typename VirtualFileSystem<T>::Entry* VirtualFileSystem<T>::FindEntry(PathView path) const
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

	template<class T>
	typename VirtualFileSystem<T>::DirectoryEntry* VirtualFileSystem<T>::GetDirectory(PathView path,
																					  bool create)
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

	template<class T>
	VirtualFileSystem<T>::Entry::Entry(PathView path, EntryType type, DirectoryEntry* parent)
		: mName{ path.Name() }, mType{ type }, mParent{ parent }
	{
		if (parent)
		{
			parent->AddChild(this);
		}
	}

	template<class T>
	VirtualFileSystem<T>::DirectoryEntry::DirectoryEntry(PathView path, DirectoryEntry* parent)
		: Entry(path, EntryType::Directory, parent)
	{
		Expects(path.IsDirectory() && path.IsAbsolute());
	}

	template<class T>
	void VirtualFileSystem<T>::DirectoryEntry::AddChild(Entry* e)
	{
		Expects(e != nullptr && e->Parent() == this);

		mChildren.push_back(e);
	}

	template<class T>
	void VirtualFileSystem<T>::DirectoryEntry::RemoveChild(Entry* e)
	{
		Expects(e != nullptr && e->Parent() == this);

		mChildren.erase(std::find(mChildren.begin(), mChildren.end(), e));
	}

	template<class T>
	VirtualFileSystem<T>::FileEntry::FileEntry(PathView path,
											   DirectoryEntry* parent,
											   FileEntryInfo info)
		: Entry(path, EntryType::File, parent), mInfo{ std::move(info) }, mFile{ nullptr }
	{
		Expects(path.IsFile() && path.IsAbsolute());
		Expects(parent != nullptr);
	}
}