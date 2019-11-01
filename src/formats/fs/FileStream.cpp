#include "FileStream.h"
#include <gsl/gsl>

namespace noire::fs
{
	CSubFileStream::CSubFileStream(IFileStream* baseStream, std::size_t offset, std::size_t size)
		: mBaseStream{ baseStream }, mOffset{ offset }, mSize{ size }, mReadingOffset{ 0 }
	{
		Expects(baseStream);
		Expects(offset + size < baseStream->Size());
	}

	void CSubFileStream::Read(void* destBuffer, std::size_t count)
	{
		Expects(destBuffer);
		Expects(count <= (mSize - mReadingOffset));

		const std::size_t oldPos = mBaseStream->Tell();

		mBaseStream->Seek(mOffset + mReadingOffset);
		mBaseStream->Read(destBuffer, count);
		mReadingOffset += count;

		mBaseStream->Seek(oldPos);
	}

	void CSubFileStream::Seek(std::size_t offset)
	{
		Expects(offset < mSize);
		mReadingOffset = offset;
	}

	std::size_t CSubFileStream::Tell() { return mReadingOffset; }

	std::size_t CSubFileStream::Size() { return mSize; }
}
