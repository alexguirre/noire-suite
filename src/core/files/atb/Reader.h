#pragma once
#include "Types.h"

namespace noire
{
	class Stream;
}

namespace noire::atb
{
	struct Reader
	{
		Reader(Stream& stream);

		Object Read();

	private:
		void ReadCollection(Object& destCollection);
		void ReadCollectionEntry(Object& destCollection);
		void ReadObject(Object& destObject);
		Property ReadPropertyValue(u32 propNameHash, ValueType propType);
		void ResolveLinks();

		Stream& mStream;
		std::vector<LinkStorage*> mLinksToResolve;
	};
}
