#include "File.h"
#include "streams/FileStream.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace noire
{
	File::File(std::shared_ptr<Stream> input) : mInput{ input }, mIsLoaded{ false } {}

	void File::Load()
	{
		if (!mIsLoaded)
		{
			LoadImpl();
			mIsLoaded = true;
		}
	}

	void File::LoadImpl() { return; }

	void File::Save(Stream& output)
	{
		if (mInput)
		{
			mInput->CopyTo(output);
		}
	}

	u64 File::Size() { return mInput ? mInput->Size() : 0; }

	static auto& FileTypes() // key is Type::Id
	{
		static std::unordered_map<size, const File::Type*> i{};
		return i;
	}

	static auto& SortedFileTypes()
	{
		static std::vector<const File::Type*> i{};
		return i;
	}

	static void SortFileTypes()
	{
		auto& types = SortedFileTypes();
		std::sort(types.begin(), types.end(), [](const File::Type* a, const File::Type* b) {
			return a->Priority > b->Priority;
		});
	}

	static void RegisterFileType(const File::Type* type)
	{
		Expects(type != nullptr);
		Expects(FileTypes().find(type->Id) == FileTypes().end());

		FileTypes().insert({ type->Id, type });
		SortedFileTypes().push_back(type);
		SortFileTypes();
	}

	static void UnregisterFileType(const File::Type* type)
	{
		Expects(type != nullptr);

		if (FileTypes().erase(type->Id))
		{
			auto& types = SortedFileTypes();
			types.erase(std::find(types.begin(), types.end(), type));
			SortFileTypes();
		}
	}

	File::Type::Type(size id,
					 size priority,
					 IsValidFunc isValidFunc,
					 CreateFunc createFunc,
					 CreateEmptyFunc createEmptyFunc)
		: Id{ id },
		  Priority{ priority },
		  IsValid{ isValidFunc },
		  Create{ createFunc },
		  CreateEmpty{ createEmptyFunc }
	{
		Expects(isValidFunc != nullptr);
		Expects(createFunc != nullptr);
		Expects(createEmptyFunc != nullptr);

		RegisterFileType(this);
	}

	File::Type::~Type() { UnregisterFileType(this); }

	std::shared_ptr<File> File::NewFromStream(std::shared_ptr<Stream> input,
											  bool fallbackToBaseFile)
	{
		Expects(input != nullptr);

		std::shared_ptr<File> resultFile = nullptr;
		for (const File::Type* t : SortedFileTypes())
		{
			if (t->IsValid(input))
			{
				resultFile = t->Create(input);
				Ensures(resultFile != nullptr);
				break;
			}
		}

		// TODO: fallbackToBaseFile may no longer be needed since RawFile accepts any input stream
		if (!resultFile && fallbackToBaseFile)
		{
			resultFile = std::make_shared<File>(input);
		}

		return resultFile;
	}

	std::shared_ptr<File> File::NewEmpty(size fileTypeId)
	{
		auto it = FileTypes().find(fileTypeId);
		Expects(it != FileTypes().end());

		const File::Type* t = it->second;
		return t->CreateEmpty();
	}
}
