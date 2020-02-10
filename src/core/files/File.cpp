#include "File.h"
#include "streams/FileStream.h"
#include <algorithm>
#include <vector>

namespace noire
{
	File::File(std::shared_ptr<Stream> input) : mInput{ input } { Expects(input != nullptr); }

	void File::Load()
	{
		// empty
	}

	void File::Save(Stream& output) { mInput->CopyTo(output); }

	u64 File::Size() { return mInput->Size(); }

	static std::vector<const File::Type*>& FileTypes()
	{
		static std::vector<const File::Type*> types;
		return types;
	}

	static void SortFileTypes()
	{
		auto& types = FileTypes();
		std::sort(types.begin(), types.end(), [](const File::Type* a, const File::Type* b) {
			return a->Priority > b->Priority;
		});
	}

	static void RegisterFileType(const File::Type* type)
	{
		Expects(type != nullptr);

		auto& types = FileTypes();
		auto it = std::find(types.begin(), types.end(), type);
		if (it == types.end())
		{
			types.push_back(type);
			SortFileTypes();
		}
	}

	static void UnregisterFileType(const File::Type* type)
	{
		Expects(type != nullptr);

		auto& types = FileTypes();
		auto it = std::find(types.begin(), types.end(), type);
		if (it != types.end())
		{
			types.erase(it);
			SortFileTypes();
		}
	}

	File::Type::Type(size priority, IsValidFunc isValidFunc, CreateFunc createFunc)
		: Priority{ priority }, IsValid{ isValidFunc }, Create{ createFunc }
	{
		Expects(isValidFunc != nullptr);
		Expects(createFunc != nullptr);

		RegisterFileType(this);
	}

	File::Type::~Type() { UnregisterFileType(this); }

	std::shared_ptr<File> File::FromStream(std::shared_ptr<Stream> input, bool fallbackToBaseFile)
	{
		Expects(input != nullptr);

		std::shared_ptr<File> resultFile = nullptr;
		for (const File::Type* t : FileTypes())
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
}
