#pragma once
#include "Common.h"
#include <memory>

namespace noire
{
	class Stream;

	class File
	{
	public:
		File(std::shared_ptr<Stream> input);

		virtual ~File() = default;

		// default Load() does nothing
		virtual void Load();
		// default Save() writes the contents of the input stream to the output stream
		virtual void Save(Stream& output);
		// Returns how many bytes will be written when calling Save(). Default Size() gets the size
		// of the input stream.
		virtual u64 Size();

	protected:
		std::shared_ptr<Stream> Input() { return mInput; }

	private:
		std::shared_ptr<Stream> mInput;

	public:
		struct Type final
		{
			using IsValidFunc = bool (*)(std::shared_ptr<Stream> input);
			using CreateFunc = std::shared_ptr<File> (*)(std::shared_ptr<Stream> input);

			Type(size priority, IsValidFunc isValidFunc, CreateFunc createFunc);
			~Type();

			Type(const Type&) = delete;
			Type(Type&&) = delete;

			Type& operator=(const Type&) = delete;
			Type& operator=(Type&&) = delete;

			// Types with higher priority are checked before ones with lower priority
			size Priority;

			// Checks whether the input stream represents a valid file of this type, may modify
			// input stream position and initial stream position may not be at the beginning.
			IsValidFunc IsValid;

			// Creates a File instance. Assume IsValid was called before calling this and returned
			// true. May modify input stream position and initial stream position may not be at the
			// beginning.
			CreateFunc Create;
		};

		// fallbackToBaseFile: Whether to create File instance if the stream format does not match
		// any specific file type.
		static std::shared_ptr<File> FromStream(std::shared_ptr<Stream> input,
												bool fallbackToBaseFile = true);
	};
}