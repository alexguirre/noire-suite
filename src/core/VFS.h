#pragma once
#include "Common.h"
#include "Path.h"
#include <memory>
#include <string>
#include <vector>

namespace noire
{
	class File;

	class VirtualFileSystem
	{
	public:
		using FileEntryInfo = size;

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

			FileEntryInfo Info() const { return mInfo; }
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

		using OpenCallback =
			std::function<std::shared_ptr<File>(PathView filePath, FileEntryInfo info)>;
		using CreateCallback = std::function<FileEntryInfo(PathView filePath)>;

		std::shared_ptr<File> Open(PathView path, OpenCallback cb);

		std::shared_ptr<File> Create(PathView path, size fileTypeId, CreateCallback cb);

		bool Exists(PathView path) const;

		// NOTE: only supports deleting files (not directories), returns whether the file existed
		// and was deleted
		bool Delete(PathView filePath);

		FileEntryInfo GetFileInfo(PathView path) const;
		void RegisterExistingFile(PathView path, FileEntryInfo info = FileEntryInfo{});

		bool ForEachFile(PathView dirPath,
						 std::function<void(PathView path, FileEntryInfo fileInfo)> callback,
						 bool recursive = true);

	private:
		Entry* FindEntry(PathView path) const;
		DirectoryEntry* GetDirectory(PathView path, bool create);

		std::unordered_map<std::size_t, std::unique_ptr<Entry>> mEntries;
		DirectoryEntry* mRoot;
	};
}