#pragma once

namespace noire
{
	namespace fs
	{
		class IFileStream;
	}

	template<class T>
	struct TFileTraits
	{
		static_assert(sizeof(T) != sizeof(T), "Needs TFileTraits specialization");

		// Gets whether the file type acts like a directory.
		static constexpr bool IsCollection{ false };

		// Gets whether the file stream contains this file type. Before returning, the stream's
		// reading offset is set to 0, a.k.a. IFileStream::Seek(0).
		static bool IsValid(fs::IFileStream& stream) { return false; }
	};
}
