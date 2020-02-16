#include "RawFile.h"
#include "streams/Stream.h"
#include "streams/TempStream.h"
#include <optional>

namespace noire
{
	// Stream that uses a ReadOnlyStream until the user starts writing, then it switches to a
	// TempStream.
	class RawFileStream final : public Stream
	{
	public:
		RawFileStream(std::shared_ptr<ReadOnlyStream> input);

		RawFileStream(const RawFileStream&) = delete;
		RawFileStream(RawFileStream&&) = default;

		RawFileStream& operator=(const RawFileStream&) = delete;
		RawFileStream& operator=(RawFileStream&&) = default;

		u64 Read(void* dstBuffer, u64 count) override;
		u64 ReadAt(void* dstBuffer, u64 count, u64 offset) override;

		u64 Write(const void* buffer, u64 count) override;
		u64 WriteAt(const void* buffer, u64 count, u64 offset) override;

		u64 Seek(i64 offset, StreamSeekOrigin origin) override;

		u64 Tell() override;

		u64 Size() override;

	private:
		void UseOutputStream();
		Stream& Current();

		std::shared_ptr<ReadOnlyStream> mInput;
		std::optional<TempStream> mOutput;
	};

	RawFile::RawFile() : RawFile(std::make_shared<EmptyStream>()) {}

	RawFile::RawFile(std::shared_ptr<noire::Stream> input)
		: File(std::make_shared<RawFileStream>(std::make_shared<ReadOnlyStream>(input)))
	{
	}

	Stream& RawFile::Stream() { return *Input(); }

	static bool Validator(std::shared_ptr<Stream> input) { return true; }

	static std::shared_ptr<File> Creator(std::shared_ptr<Stream> input)
	{
		return std::make_shared<RawFile>(input);
	}

	static std::shared_ptr<File> CreatorEmpty() { return std::make_shared<RawFile>(); }

	const File::Type RawFile::Type{ std::hash<std::string_view>{}("RawFile"),
									0,
									&Validator,
									&Creator,
									&CreatorEmpty };

	RawFileStream::RawFileStream(std::shared_ptr<ReadOnlyStream> input)
		: mInput{ input }, mOutput{ std::nullopt }
	{
		Expects(input != nullptr);
	}

	u64 RawFileStream::Read(void* dstBuffer, u64 count) { return Current().Read(dstBuffer, count); }

	u64 RawFileStream::ReadAt(void* dstBuffer, u64 count, u64 offset)
	{
		return Current().ReadAt(dstBuffer, count, offset);
	}

	u64 RawFileStream::Write(const void* buffer, u64 count)
	{
		UseOutputStream();
		return Current().Write(buffer, count);
	}

	u64 RawFileStream::WriteAt(const void* buffer, u64 count, u64 offset)
	{
		UseOutputStream();
		return Current().WriteAt(buffer, count, offset);
	}

	u64 RawFileStream::Seek(i64 offset, StreamSeekOrigin origin)
	{
		return Current().Seek(offset, origin);
	}

	u64 RawFileStream::Tell() { return Current().Tell(); }

	u64 RawFileStream::Size() { return Current().Size(); }

	void RawFileStream::UseOutputStream()
	{
		if (!mOutput.has_value())
		{
			Stream& i = *mInput;
			Stream& o = mOutput.emplace();

			i.CopyTo(o);
			o.Seek(gsl::narrow<i64>(i.Tell()), StreamSeekOrigin::Begin);

			Ensures(i.Size() == o.Size());

			mInput.reset();
		}
	}

	Stream& RawFileStream::Current()
	{
		if (mOutput.has_value())
		{
			return *mOutput;
		}

		return *mInput;
	}
}
