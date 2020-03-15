#include "Section.h"
#include "files/Trunk.h"

namespace noire::trunk
{
	SubStream Section::GetDataStream(Trunk& owner) const
	{
		return SubStream{ owner.Raw(), owner.GetDataOffset(Offset), Size };
	}
}
