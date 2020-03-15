#pragma once
#include "Common.h"
#include "streams/Stream.h"

namespace noire
{
	class Trunk;
}

namespace noire::trunk
{
	struct Section
	{
		u32 NameHash{ 0 };
		u32 Size{ 0 };
		u32 Offset{ 0 };

		SubStream GetDataStream(Trunk& owner) const;
	};
}
