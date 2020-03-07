#pragma once
#include "Types.h"
#include <pugixml.hpp>

namespace noire::atb
{
	struct XmlReader
	{
		XmlReader();

		Object ReadObject(pugi::xml_node node);
	};
}
