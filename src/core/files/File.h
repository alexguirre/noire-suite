#pragma once
#include "Common.h"
#include "Path.h"
#include <memory>

namespace noire
{
	class Device;
	class Stream;
	class ReadOnlyStream;

	// IDEA: remove RawFile and give File a RawStream property
	class File
	{
	public:
		File(Device& parent, PathView path, bool created);

		virtual ~File() = default;

		void Load();
		// default Save() writes the contents of the input stream to the output stream
		virtual void Save();
		// Returns how many bytes will be written when calling Save(). Default Size() gets the size
		// of the input stream.
		virtual u64 Size();

		virtual bool HasChanged() const;

		bool IsLoaded() const { return mIsLoaded; }
		PathView Path() const { return mPath; }
		Device& Parent() { return mParent; }
		const Device& Parent() const { return mParent; }

		Stream& Raw();

	protected:
		// default LoadImpl() does nothing
		virtual void LoadImpl();

	private:
		Device& mParent;
		noire::Path mPath;
		bool mIsLoaded;
		std::unique_ptr<Stream> mRawStream;

	public:
		struct TypeDefinition final
		{
			using IsValidFunc = bool (*)(Stream& input);
			using CreateFunc = std::shared_ptr<File> (*)(Device& parent,
														 PathView path,
														 bool created);

			TypeDefinition(size id, size priority, IsValidFunc isValidFunc, CreateFunc createFunc);
			~TypeDefinition();

			TypeDefinition(const TypeDefinition&) = delete;
			TypeDefinition(TypeDefinition&&) = delete;

			TypeDefinition& operator=(const TypeDefinition&) = delete;
			TypeDefinition& operator=(TypeDefinition&&) = delete;

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
		static const TypeDefinition Type;

		static size FindTypeOfStream(Stream& input);
		static std::shared_ptr<File>
		New(Device& parent, PathView path, bool created, size fileTypeId);
	};
}
