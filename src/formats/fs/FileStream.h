#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace noire::fs
{
	using FileStreamSize = std::uintmax_t;

	class IFileStream
	{
	public:
		virtual ~IFileStream() = default;

		virtual void Read(void* destBuffer, FileStreamSize count) = 0;
		virtual void Seek(FileStreamSize offset) = 0;
		virtual FileStreamSize Tell() = 0;
		virtual FileStreamSize Size() = 0;

		template<class T>
		T Read()
		{
			static_assert(std::is_pod_v<T>, "Expected a POD type for T");

			T tmp;
			Read(&tmp, sizeof(tmp));
			return tmp;
		}
	};

	class CSubFileStream : public IFileStream
	{
	public:
		CSubFileStream(IFileStream* baseStream, FileStreamSize offset, FileStreamSize size);

		void Read(void* destBuffer, FileStreamSize count) override;
		void Seek(FileStreamSize offset) override;
		FileStreamSize Tell() override;
		FileStreamSize Size() override;

	private:
		IFileStream* mBaseStream;
		FileStreamSize mOffset;
		FileStreamSize mSize;
		FileStreamSize mReadingOffset;
	};
}
