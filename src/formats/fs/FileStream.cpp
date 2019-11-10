#include "FileStream.h"
#include <gsl/gsl>

namespace noire::fs
{
	CSubFileStream::CSubFileStream(IFileStream* baseStream,
								   FileStreamSize offset,
								   FileStreamSize size)
		: mBaseStream{ baseStream }, mOffset{ offset }, mSize{ size }, mReadingOffset{ 0 }
	{
		Expects(baseStream);
		Expects(offset + size <= baseStream->Size());
	}

	void CSubFileStream::Read(void* destBuffer, FileStreamSize count)
	{
		Expects(destBuffer);
		Expects(count <= (mSize - mReadingOffset));

		const FileStreamSize oldPos = mBaseStream->Tell();

		mBaseStream->Seek(mOffset + mReadingOffset);
		mBaseStream->Read(destBuffer, count);
		mReadingOffset += count;

		mBaseStream->Seek(oldPos);
	}

	void CSubFileStream::Seek(FileStreamSize offset)
	{
		Expects(offset < mSize);
		mReadingOffset = offset;
	}

	FileStreamSize CSubFileStream::Tell() { return mReadingOffset; }

	FileStreamSize CSubFileStream::Size() { return mSize; }
}
