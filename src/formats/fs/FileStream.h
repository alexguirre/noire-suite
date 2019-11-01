#pragma once
#include <cstddef>
#include <cstdint>

namespace noire::fs
{
	class IFileStream
	{
	public:
		virtual void Read(void* destBuffer, std::size_t count) = 0;
		virtual void Seek(std::size_t offset) = 0;
		virtual std::size_t Tell() = 0;
		virtual std::size_t Size() = 0;
	};

	class CSubFileStream : public IFileStream
	{
	public:
		CSubFileStream(IFileStream* baseStream, std::size_t offset, std::size_t size);

		void Read(void* destBuffer, std::size_t count) override;
		void Seek(std::size_t offset) override;
		std::size_t Tell() override;
		std::size_t Size() override;

	private:
		IFileStream* mBaseStream;
		std::size_t mOffset;
		std::size_t mSize;
		std::size_t mReadingOffset;
	};
}
