#include "Graphics.h"
#include <Windows.h>
#include <unordered_map>

namespace noire::trunk
{
	static void Load(byte* mainData, byte* vramData);

	size Drawable::ModelCount() const { return ModelsLast - Models; }

	void Drawable::CalculateBufferSizes(u32& totalVertexDataSize, u32& totalIndexDataSize)
	{
		totalVertexDataSize = 0;
		totalIndexDataSize = 0;

		const size modelCount = ModelCount();
		for (size i = 0; i < modelCount; i++)
		{
			const Model& model = *Models[i];
			for (size j = 0; j < model.GeometryCount; j++)
			{
				const Geometry& geom = model.Geometries[j];
				totalVertexDataSize += geom.VertexDataSize;
				totalIndexDataSize += geom.IndexDataSize;
			}

			totalVertexDataSize += model.field_20;
			Expects(model.field_20 == 0); // the meaning of this field is unknown, all the files I
										  // tried so far have its value equal to 0
		}
	}

	Graphics::Graphics(Trunk& owner, Section main, Section vram)
		: Owner{ owner }, Main{ main }, VRAM{ vram }, MainData{}, VRAMData{}
	{
		SubStream m = Main.GetDataStream(owner);
		MainData.resize(gsl::narrow<size>(m.Size()));
		m.Read(MainData.data(), MainData.size());

		SubStream v = VRAM.GetDataStream(owner);
		VRAMData.resize(gsl::narrow<size>(v.Size()));
		v.Read(VRAMData.data(), VRAMData.size());

		Load(MainData.data(), VRAMData.data());
	}

	Drawable& Graphics::Drawable()
	{
		return *reinterpret_cast<trunk::Drawable*>(
			MainData.data() + ((*(u32*)(MainData.data() + 8) + 15) & 0xFFFFFFF0));
	}

	using DataStorage = std::unordered_map<u32, byte*>;
	static void StoreData(DataStorage& s, byte* data, u32 id) { s.insert({ id, data }); }
	static void RemoveData(DataStorage& s, u32 id) { s.erase(id); }
	static byte* FindData(DataStorage& s, u32 id)
	{
		auto it = s.find(id);
		return it == s.end() ? nullptr : it->second;
	}

	// mostly copied from decompiler output
	// TODO: research this code more
	static void Fixup(DataStorage& s, byte* a1, byte* a2)
	{
		byte* v2;   // ebp
		u16* v3;    // esi
		i32 v4;     // ebx
		i32 count;  // ecx
		u16* v6;    // esi
		i32 v7;     // eax
		i32 _count; // edx
		i32 v9;     // ecx
		byte* v10;  // esi
		i32 v11;    // eax
		u32 v12;    // ebp
		byte* v13;  // esi
		byte* v14;  // eax
		byte* v15;  // [esp+Ch] [ebp-8h]
		u32 a2a;    // [esp+10h] [ebp-4h]

		StoreData(s, a2, 0);

		v2 = &a1[*((u32*)a1 + 2)];
		v3 = (u16*)(a1 + 12);
		v15 = &a1[*((u32*)a1 + 2)];
		v4 = (i32)a2;
		while (true)
		{
			count = *v3;
			v6 = v3 + 1;
			v7 = 0;
			if (count)
			{
				_count = count;
				do
				{
					v9 = *v6;
					if (v9 & 0x8000)
					{
						v10 = (byte*)v6 + (((u32)v6 & 0xFF) & 2);
						v11 = *(u32*)v10;
						v12 = *(u32*)v10 & 0x80000000 | ((*(u32*)v10 & 0x80000000) >> 16);
						v6 = (u16*)(v10 + 4);
						v7 = v12 ^ v11;
					}
					else
					{
						v7 += 4 * v9;
						++v6;
					}
					*(u32*)&a2[v7] += v4;
					--_count;
				} while (_count);
				v2 = v15;
			}
			v13 = (byte*)v6 + (((u32)v6 & 0xFF) & 2);
			if (v13 + 6 >= v2)
				break;
			a2a = *(u32*)v13;
			v14 = FindData(s, a2a);
			if (v14)
				v4 = (i32)v14;
			else
				v4 = 0;
			v3 = (u16*)(v13 + 4);
		}

		RemoveData(s, 0);
	}

	static void Load(byte* mainData, byte* vramData)
	{
		DataStorage s{};
		StoreData(s, vramData, 1);
		Fixup(s, mainData, (mainData + ((*(u32*)(mainData + 8) + 15) & 0xFFFFFFF0)));
	}
}
