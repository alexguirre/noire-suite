#pragma once
#include "Common.h"
#include "Section.h"
#include <vector>

namespace noire
{
	class Trunk;
}

namespace noire::trunk
{
	struct Geometry
	{
		u32 field_0;
		u32 field_4;
		u32 field_8;
		u32 field_C;
		u32 field_10;
		u32 field_14;
		u32 field_18;
		u32 field_1C;
		u32 field_20;
		byte* VertexData;
		u32 VertexDataSize;
		byte* IndexData;
		u32 IndexDataSize;
		u32 field_34;
		u32 TriangleCount;
		u16 field_3C;
		u16 field_3E;
		u32 field_40;
		u32 field_44;
		u32 field_48;
		u32 field_4C;
		u32 field_50;
		u32 field_54;
		u32 field_58;
		u32 field_5C;
		u32 field_60;
		u32 field_64;
		void* VertexBuffer; // used at runtime
		void* IndexBuffer;  // used at runtime
		u32 field_70;
		u32 field_74;
		u32 field_78;
		u32 field_7C;
	};
	static_assert(sizeof(Geometry) == 0x80);

	struct Model
	{
		u32 field_0;
		u32 field_4;
		u32 field_8;
		u32 field_C;
		u16 field_10;
		u16 GeometryCount;
		u32* VertDeclsHashes; // array, each item for each geometry. Next after last is 0
		u8 field_18;
		u8 field_19;
		u16 field_1A;
		u32 field_1C;
		u32 field_20;
		u32 field_24;
		u32 field_28;
		u32 field_2C;
		Geometry Geometries[1]; // works like a flexible array member
	};
	static_assert(sizeof(Model) == (0x30 + sizeof(Geometry)));

	struct Drawable
	{
		Model** Models;
		Model** ModelsLast;
		u32 field_8;
		u32 field_C;
		u32 TextureUnk1;
		u32 TextureUnk2;

		size ModelCount() const;
		void CalculateBufferSizes(u32& totalVertexDataSize, u32& totalIndexDataSize);
	};

	struct Graphics
	{
		Trunk& Owner;
		Section Main;
		Section VRAM;
		std::vector<byte> MainData;
		std::vector<byte> VRAMData;

		Graphics(Trunk& owner, Section main, Section vram);

		Drawable& Drawable();
	};
}
