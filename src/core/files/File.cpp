#include "File.h"
#include "devices/Device.h"
#include "streams/FileStream.h"
#include "streams/TempStream.h"
#include <algorithm>
#include <optional>
#include <unordered_map>
#include <vector>

namespace noire
{
	// Stream that uses a ReadOnlyStream until the user starts writing, then it switches to a
	// TempStream.
	class RawFileStream final : public Stream
	{
	public:
		RawFileStream(File& file);

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

		bool IsUsingOutputStream() const;

	private:
		void UseOutputStream();
		Stream& Current();

		File& mFile;
		std::optional<ReadOnlyStream> mInput;
		std::optional<TempStream> mOutput;
	};

	File::File(Device& parent, PathView path, bool created)
		: mParent{ parent }, mPath{ path }, mIsLoaded{ created }, mRawStream{}
	{
	}

	void File::Load()
	{
		if (!mIsLoaded)
		{
			LoadImpl();
			mIsLoaded = true;
		}
	}

	void File::LoadImpl() {}

	void File::Save() {}

	u64 File::Size() { return Raw().Size(); }

	Stream& File::Raw()
	{
		return mRawStream ? *mRawStream : *(mRawStream = std::make_unique<RawFileStream>(*this));
	}

	bool File::HasChanged() const
	{
		return mRawStream ? static_cast<RawFileStream*>(mRawStream.get())->IsUsingOutputStream() :
							false;
	}

	static auto& FileTypes() // key is TypeDefinition::Id
	{
		static std::unordered_map<size, const File::TypeDefinition*> i{};
		return i;
	}

	static auto& SortedFileTypes()
	{
		static std::vector<const File::TypeDefinition*> i{};
		return i;
	}

	static void SortFileTypes()
	{
		auto& types = SortedFileTypes();
		std::sort(types.begin(),
				  types.end(),
				  [](const File::TypeDefinition* a, const File::TypeDefinition* b) {
					  return a->Priority > b->Priority;
				  });
	}

	static void RegisterFileType(const File::TypeDefinition* type)
	{
		Expects(type != nullptr);
		Expects(FileTypes().find(type->Id) == FileTypes().end());

		FileTypes().insert({ type->Id, type });
		SortedFileTypes().push_back(type);
		SortFileTypes();
	}

	static void UnregisterFileType(const File::TypeDefinition* type)
	{
		Expects(type != nullptr);

		if (FileTypes().erase(type->Id))
		{
			auto& types = SortedFileTypes();
			types.erase(std::find(types.begin(), types.end(), type));
			SortFileTypes();
		}
	}

	File::TypeDefinition::TypeDefinition(size id,
										 size priority,
										 IsValidFunc isValidFunc,
										 CreateFunc createFunc)
		: Id{ id }, Priority{ priority }, IsValid{ isValidFunc }, Create{ createFunc }
	{
		Expects(id != InvalidTypeId);
		Expects(isValidFunc != nullptr);
		Expects(createFunc != nullptr);

		RegisterFileType(this);
	}

	File::TypeDefinition::~TypeDefinition() { UnregisterFileType(this); }

	size File::FindTypeOfStream(Stream& input)
	{
		for (const File::TypeDefinition* t : SortedFileTypes())
		{
			if (t->IsValid(input))
			{
				return t->Id;
			}
		}

		return InvalidTypeId;
	}

	std::shared_ptr<File> File::New(Device& parent, PathView path, bool created, size fileTypeId)
	{
		if (fileTypeId == InvalidTypeId)
			Expects(fileTypeId != InvalidTypeId);

		auto it = FileTypes().find(fileTypeId);
		Expects(it != FileTypes().end());

		const File::TypeDefinition& t = *it->second;
		return t.Create(parent, path, created);
	}

	const File::TypeDefinition File::Type{ std::hash<std::string_view>{}("File"),
										   0,
										   [](Stream&) { return true; },
										   [](Device& parent, PathView path, bool created) {
											   return std::make_shared<File>(parent, path, created);
										   } };

	RawFileStream::RawFileStream(File& file)
		: mFile{ file }, mInput{ std::nullopt }, mOutput{ std::nullopt }
	{
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

	bool RawFileStream::IsUsingOutputStream() const { return mOutput.has_value(); }

	void RawFileStream::UseOutputStream()
	{
		if (!IsUsingOutputStream())
		{
			Stream& i = Current();
			Stream& o = mOutput.emplace();

			i.CopyTo(o);
			o.Seek(gsl::narrow<i64>(i.Tell()), StreamSeekOrigin::Begin);

			Ensures(i.Size() == o.Size());

			mInput.reset();
		}
	}

	Stream& RawFileStream::Current()
	{
		if (IsUsingOutputStream())
		{
			return *mOutput;
		}

		return mInput.has_value() ? *mInput : *(mInput = mFile.Parent().OpenStream(mFile.Path()));
	}
}
