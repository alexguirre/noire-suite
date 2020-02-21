#pragma once
#include "Common.h"
#include "Path.h"
#include <memory>

namespace noire
{
	class Device;
	class Stream;
	class ReadOnlyStream;

	class File
	{
	public:
		File(Device& parent, PathView path);

		virtual ~File() = default;

		void Load();
		// default Save() writes the contents of the input stream to the output stream
		virtual void Save(Stream& output);
		// Returns how many bytes will be written when calling Save(). Default Size() gets the size
		// of the input stream.
		virtual u64 Size();

		bool IsLoaded() const { return mIsLoaded; }
		// TODO: File's input stream shouldn't be public, required for now by CComFileStream from
		// file-explorer
		std::shared_ptr<ReadOnlyStream> Input();

	protected:
		// default LoadImpl() does nothing
		virtual void LoadImpl();

	private:
		Device& mParent;
		Path mPath;
		bool mIsLoaded;

	public:
		struct Type final
		{
			using IsValidFunc = bool (*)(Stream& input);
			using CreateFunc = std::shared_ptr<File> (*)(Device& parent, PathView path);

			Type(size id, size priority, IsValidFunc isValidFunc, CreateFunc createFunc);
			~Type();

			Type(const Type&) = delete;
			Type(Type&&) = delete;

			Type& operator=(const Type&) = delete;
			Type& operator=(Type&&) = delete;

			size Id;

			// Types with higher priority are checked before ones with lower priority
			size Priority;

			// Checks whether the input stream represents a valid file of this type, may modify
			// input stream position and initial stream position may not be at the beginning.
			IsValidFunc IsValid;

			// Creates a File instance. Assume IsValid was called before calling this and returned
			// true.
			CreateFunc Create;
		};

		static constexpr size InvalidTypeId{ static_cast<size>(-1) };

		static size FindTypeOfStream(Stream& input);
		static std::shared_ptr<File> New(Device& parent, PathView path, size fileTypeId);
	};
}