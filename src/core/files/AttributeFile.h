#pragma once
#include "Common.h"
#include "File.h"
#include "atb/Types.h"
#include <vector>

namespace noire
{
	class AttributeFile final : public File
	{
	public:
		AttributeFile(Device& parent, PathView path, bool created);

	protected:
		void LoadImpl() override;

	public:
		void Save() override;
		u64 Size() override;
		bool HasChanged() const override;

		const atb::Object& Root() const { return mRoot; }

	private:
		atb::Object mRoot;

	public:
		static const TypeDefinition Type;
	};
}
