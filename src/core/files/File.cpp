#include "File.h"
#include "devices/Device.h"
#include "streams/FileStream.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace noire
{
	File::File(Device& parent, PathView path)
		: mParent{ parent }, mPath{ path }, mIsLoaded{ false }, mInput{}
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

	void File::Save(Stream& output)
	{
		if (std::shared_ptr i = Input())
		{
			i->CopyTo(output);
		}
	}

	u64 File::Size() { return Input()->Size(); }

	std::shared_ptr<ReadOnlyStream> File::Input()
	{
		if (std::shared_ptr p = mInput.lock())
		{
			return p;
		}
		else
		{
			std::shared_ptr s = mParent.OpenStream(mPath);
			mInput = s;
			return s;
		}
	}

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

	File::Type::Type(size id, size priority, IsValidFunc isValidFunc, CreateFunc createFunc)
		: Id{ id }, Priority{ priority }, IsValid{ isValidFunc }, Create{ createFunc }
	{
		Expects(id != InvalidTypeId);
		Expects(isValidFunc != nullptr);
		Expects(createFunc != nullptr);

		RegisterFileType(this);
	}

	File::Type::~Type() { UnregisterFileType(this); }

	size File::FindTypeOfStream(Stream& input)
	{
		for (const File::Type* t : SortedFileTypes())
		{
			if (t->IsValid(input))
			{
				return t->Id;
			}
		}

		return InvalidTypeId;
	}

	std::shared_ptr<File> File::New(Device& parent, PathView path, size fileTypeId)
	{
		Expects(fileTypeId != InvalidTypeId);

		auto it = FileTypes().find(fileTypeId);
		Expects(it != FileTypes().end());

		const File::Type& t = *it->second;
		return t.Create(parent, path);
	}
}
