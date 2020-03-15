#include "Graphics.h"
#include <unordered_map>

namespace noire::trunk
{
	static void Load(byte* mainData, byte* vramData, u32& vertexBufferSize, u32& indexBufferSize);

	size Drawable::ModelCount() const { return ModelsLast - Models; }

	Graphics::Graphics(Trunk& owner, Section main, Section vram)
		: Owner{ owner },
		  Main{ main },
		  VRAM{ vram },
		  MainData{},
		  VRAMData{},
		  VertexBufferSize{ 0 },
		  IndexBufferSize{ 0 }
	{
		SubStream m = Main.GetDataStream(owner);
		MainData.resize(gsl::narrow<size>(m.Size()));
		m.Read(MainData.data(), MainData.size());

		SubStream v = VRAM.GetDataStream(owner);
		VRAMData.resize(gsl::narrow<size>(v.Size()));
		v.Read(VRAMData.data(), VRAMData.size());

		Load(MainData.data(), VRAMData.data(), VertexBufferSize, IndexBufferSize);
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

	static void Load(byte* mainData, byte* vramData, u32& vertexBufferSize, u32& indexBufferSize)
	{
		DataStorage s{};
		StoreData(s, vramData, 1);
		Fixup(s, mainData, (mainData + ((*(u32*)(mainData + 8) + 15) & 0xFFFFFFF0)));

		byte* v5 = (mainData + ((*(u32*)(mainData + 8) + 15) & 0xFFFFFFF0));
		u32 v8 = *(u32*)(v5 + 4);
		u32 v9 = *(u32*)v5;
		u32 modelCount = (v8 - v9) >> 2;

		indexBufferSize = 0;
		vertexBufferSize = 0;
		u32 v33 = 0;
		if (modelCount)
		{
			u32 _v9 = v9;
			u32 v36 = modelCount;
			do
			{
				u32 v12 = *(u32*)v9;
				u32 v37 = v12;
				u16 v13 = *(u16*)(v12 + 18);
				u32 v14 = 0;
				u32 v15 = 0;
				u32 v16 = 0;
				u32 v17 = 0;
				u32 v18 = 0;
				u16 v38 = v13;
				if (v13 >= 2)
				{
					u32* v19 = (u32*)(v37 + 96);
					u32 v20 = ((u32)(v13 - 2) >> 1) + 1;
					v18 = 2 * v20;
					do
					{
						v16 += *(v19 - 2);
						v14 += *v19;
						v17 += v19[30];
						v15 += v19[32];
						v19 += 64;
						--v20;
					} while (v20);
					v13 = v38;
					v9 = _v9;
				}
				u32 v22 = 0;
				if (v18 >= v13)
				{
					v22 = v33;
				}
				else
				{
					auto v21 = *(u32*)v9 + (v18 << 7);
					vertexBufferSize += *(u32*)(v21 + 88);
					v22 = *(u32*)(v21 + 96) + v33;
				}
				indexBufferSize = v15 + v14 + v22;
				vertexBufferSize += v17 + v16 + *(u32*)(*(u32*)v9 + 32);
				v9 += 4;
				v33 = indexBufferSize;
				_v9 = v9;
			} while (!(v36-- == 1));
		}
	}
}
