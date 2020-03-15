#include "UniqueTexture.h"

namespace noire::trunk
{
	UniqueTexture::UniqueTexture(Trunk& owner, Section main, Section vram)
		: Owner{ owner }, Main{ main }, VRAM{ vram }, Textures{}
	{
		SubStream mainStream = Main.GetDataStream(Owner);
		Stream& m = mainStream;

		// these 4 bytes are used by the game at runtime indicate if it already loaded the texture,
		// the file should always have 0 here
		Expects(m.Read<u32>() == 0);

		const u32 textureCount = m.Read<u32>();
		Textures.reserve(textureCount);
		for (size i = 0; i < textureCount; i++)
		{
			Entry& e = Textures.emplace_back();
			e.Offset = m.Read<u32>();
			Expects(m.Read<u32>() == 0); // unknown, should be always zero
			e.NameHash = m.Read<u32>();
		}
	}

	std::vector<byte> UniqueTexture::GetTextureData(size textureIndex) const
	{
		Expects(textureIndex < Textures.size());

		SubStream vramStream = VRAM.GetDataStream(Owner);
		Stream& v = vramStream;

		const Entry& e = Textures[textureIndex];
		// this expects the entries to be sorted by offset, not sure if that is always the case
		const size dataSize = ((textureIndex < (Textures.size() - 1)) ?
								   (Textures[textureIndex + 1].Offset - e.Offset) :
								   (gsl::narrow<size>(v.Size()) - e.Offset));

		std::vector<byte> buffer{};
		buffer.resize(dataSize);
		v.Seek(e.Offset, StreamSeekOrigin::Begin);
		v.Read(buffer.data(), dataSize);

		return buffer;
	}
}
