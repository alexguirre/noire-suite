#pragma once
#include "Common.h"
#include "Section.h"

namespace noire
{
	class Trunk;
}

namespace noire::trunk
{
	struct UniqueTexture
	{
		struct Entry
		{
			u32 Offset;
			u32 NameHash;
		};

		Trunk& Owner;
		Section Main;
		Section VRAM;
		std::vector<Entry> Textures;

		UniqueTexture(Trunk& owner, Section main, Section vram);

		std::vector<byte> GetTextureData(size textureIndex) const;
	};
}
