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
		void ReadCollection(Stream& stream, atb::Object& destCollection);
		void ReadCollectionEntry(Stream& stream, atb::Object& destCollection);
		void ReadObject(Stream& stream, atb::Object& destObject);
		atb::Property ReadPropertyValue(Stream& stream, u32 propNameHash, atb::ValueType propType);
		void SkipProperty(Stream& stream, atb::ValueType propType);
		void ResolveLinks(Stream& stream);

		atb::Object mRoot;
		std::vector<atb::LinkStorage*> mLinksToResolve;

	public:
		static constexpr u32 HeaderMagic{ 0x00425441 }; // 'ATB'
		static const TypeDefinition Type;
	};
}
