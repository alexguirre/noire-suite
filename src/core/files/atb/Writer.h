#pragma once
#include <vector>

namespace noire
{
	class Stream;
}

namespace noire::atb
{
	struct Object;
	struct Property;
	struct LinkStorage;

	struct Writer
	{
		Writer(Stream& stream);

		void Write(const Object& root);

	private:
		void WriteCollection(const Object& collection);
		void WriteCollectionEntry(const Object& entry);
		void WriteObject(const Object& object);
		void WritePropertyValue(const Property& prop);
		void WriteLinks();

		Stream& mStream;
		std::vector<LinkStorage*> mLinks;
	};
}
