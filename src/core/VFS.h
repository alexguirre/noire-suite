#pragma once
#include "Common.h"
#include "Path.h"
#include "streams/Stream.h"
#include <functional>
#include <memory>
#include <vector>

namespace noire
{
	template<class T>
	class VirtualFileSystem
	{
	public:
		using EntryData = T;

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

			const std::string& Name() const { return mName; }
			EntryType Type() const { return mType; }
			DirectoryEntry* Parent() const { return mParent; }

		private:
			std::string mName;
			EntryType mType;
			DirectoryEntry* mParent;
		};

		class DirectoryEntry : public Entry
		{
		public:
			DirectoryEntry(PathView path, DirectoryEntry* parent);

			void AddChild(Entry* e);

			const std::vector<Entry*>& Children() const { return mChildren; }

		private:
			std::vector<Entry*> mChildren;
		};

		class FileEntry : public Entry
		{
		public:
			FileEntry(PathView path, DirectoryEntry* parent, const EntryData& data);

			const EntryData& Data() const { return mData; }

		private:
			std::shared_ptr<Stream>
				mStream; // stream returned by Open()/Create(), refers to temp file (or only temp
						 // file for writing? original stream for reading?)
			EntryData mData;
		};

	public:
		VirtualFileSystem();
		VirtualFileSystem(const VirtualFileSystem&) = delete;
		VirtualFileSystem(VirtualFileSystem&&) = default;

		VirtualFileSystem& operator=(const VirtualFileSystem&) = delete;
		VirtualFileSystem& operator=(VirtualFileSystem&&) = default;

		// template<class F> // F equivalent to std::shared_ptr<Stream>(PathView, const EntryData&)
		// std::shared_ptr<Stream> Open(PathView path, F createStreamCallback);

		// template<class F>
		// std::shared_ptr<Stream> Create(PathView path, F createStreamCallback);

		// bool Exists(PathView path);

		// bool Delete(PathView path);

		void RegisterFile(PathView path, const EntryData& data = EntryData{});

		// TODO: make ForEachFile recursive
		bool ForEachFile(PathView dirPath,
						 std::function<void(PathView path, const EntryData& fileData)> callback);

	private:
		Entry* FindEntry(PathView path);
		DirectoryEntry* GetDirectory(PathView path, bool create);

		std::unordered_map<std::size_t, std::unique_ptr<Entry>> mEntries;
		DirectoryEntry* mRoot;
	};

	template<class T>
	VirtualFileSystem<T>::VirtualFileSystem() : mEntries{}, mRoot{ GetDirectory("/", true) }
	{
	}

	template<class T>
	void VirtualFileSystem<T>::RegisterFile(PathView path, const EntryData& data)
	{
		Expects(path.IsFile() && path.IsAbsolute());

		// TODO: provide override option?
		Expects(FindEntry(path) == nullptr); // path is not registerted yet

		DirectoryEntry* parent = GetDirectory(path.Parent(), true);
		Ensures(parent != nullptr);
		auto entry = std::make_unique<FileEntry>(path, parent, data);
		if (auto p = mEntries.emplace(std::hash<PathView>{}(path), std::move(entry)); p.second)
		{
			Ensures(p.first->second->Type() == EntryType::File);
		}
	}

	template<class T>
	bool VirtualFileSystem<T>::ForEachFile(
		PathView dirPath,
		std::function<void(PathView path, const EntryData& fileData)> callback)
	{
		Expects(dirPath.IsDirectory() && dirPath.IsAbsolute());

		DirectoryEntry* dir = GetDirectory(dirPath, false);
		if (dir)
		{
			for (Entry* e : dir->Children())
			{
				if (e->Type() == EntryType::File)
				{
					callback(Path{ dirPath } / e->Name(), reinterpret_cast<FileEntry*>(e)->Data());
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
	typename VirtualFileSystem<T>::Entry* VirtualFileSystem<T>::FindEntry(PathView path)
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
			return reinterpret_cast<DirectoryEntry*>(e);
		}
		else if (create)
		{
			DirectoryEntry* parent = path.IsRoot() ? nullptr : GetDirectory(path.Parent(), true);
			auto newEntry = std::make_unique<DirectoryEntry>(path, parent);
			if (auto p = mEntries.emplace(std::hash<PathView>{}(path), std::move(newEntry));
				p.second)
			{
				Ensures(p.first->second->Type() == EntryType::Directory);
				return reinterpret_cast<DirectoryEntry*>(p.first->second.get());
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
	VirtualFileSystem<T>::FileEntry::FileEntry(PathView path,
											   DirectoryEntry* parent,
											   const EntryData& data)
		: Entry(path, EntryType::File, parent), mStream{ nullptr }, mData{ data }
	{
		Expects(path.IsFile() && path.IsAbsolute());
		Expects(parent != nullptr);
	}
}