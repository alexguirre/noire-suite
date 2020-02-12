#include "File.h"
#include "streams/FileStream.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace noire
{
	File::File(std::shared_ptr<Stream> input) : mInput{ input } {}

	void File::Load()
	{
		// empty
	}

	void File::Save(Stream& output)
	{
		if (mInput)
		{
			mInput->CopyTo(output);
		}
	}

	u64 File::Size() { return mInput ? mInput->Size() : 0; }

	static std::unordered_map<size, const File::Type*> gFileTypes; // key is Type::Id
	static std::vector<const File::Type*> gSortedFileTypes;

	static void SortFileTypes()
	{
		auto& types = gSortedFileTypes;
		std::sort(types.begin(), types.end(), [](const File::Type* a, const File::Type* b) {
			return a->Priority > b->Priority;
		});
	}

	static void RegisterFileType(const File::Type* type)
	{
		Expects(type != nullptr);
		Expects(gFileTypes.find(type->Id) == gFileTypes.end());

		gFileTypes.insert({ type->Id, type });
		gSortedFileTypes.push_back(type);
		SortFileTypes();
	}

	static void UnregisterFileType(const File::Type* type)
	{
		Expects(type != nullptr);

		if (gFileTypes.erase(type->Id))
		{
			auto& types = gSortedFileTypes;
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
		for (const File::Type* t : gSortedFileTypes)
		{
			if (t->IsValid(input))
			{
				resultFile = t->Create(input);
				break;
			}
		}

		if (!resultFile && fallbackToBaseFile)
		{
			resultFile = std::make_shared<File>(input);
		}

		return resultFile;
	}

	std::shared_ptr<File> File::NewEmpty(size fileTypeId)
	{
		auto it = gFileTypes.find(fileTypeId);
		Expects(it != gFileTypes.end());

		const File::Type* t = it->second;
		return t->CreateEmpty();
	}
}
