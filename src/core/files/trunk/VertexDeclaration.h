#pragma once
#include "Common.h"
#include <d3d11.h>
#include <unordered_map>
#include <vector>
#include <winrt/base.h>

namespace noire::trunk
{
	enum class VertexElementSemantic
	{
		Invalid = 0,
		POSITION,
		BLENDWEIGHT,
		BLENDINDICES,
		NORMAL,
		PSIZE,
		TEXCOORD0,
		TEXCOORD1,
		TEXCOORD2,
		TEXCOORD3,
		TANGENT,
		BINORMAL,
		COLOR,
	};

	struct VertexElement
	{
		u32 InputSlot{ 0 };
		u32 Offset{ 0 };
		DXGI_FORMAT Format{ DXGI_FORMAT_UNKNOWN };
		VertexElementSemantic Semantic{ VertexElementSemantic::Invalid };

		D3D11_INPUT_ELEMENT_DESC ToDesc() const;
	};

	struct VertexDeclaration
	{
		std::vector<VertexElement> Elements{};
		winrt::com_ptr<ID3D11InputLayout> InputLayout{ nullptr };

		u32 VertexSize() const;
	};

	class VertexDeclarationManager
	{
	public:
		VertexDeclarationManager();

		VertexDeclarationManager(const VertexDeclarationManager&) = delete;
		VertexDeclarationManager(VertexDeclarationManager&&) = default;

		VertexDeclarationManager& operator=(const VertexDeclarationManager&) = delete;
		VertexDeclarationManager& operator=(VertexDeclarationManager&&) = default;

		void Init(const winrt::com_ptr<ID3D11Device>& device);
		void Shutdown();
		const VertexDeclaration& Get(u32 id);

	private:
		winrt::com_ptr<ID3D11Device> mDevice;
		std::unordered_map<u32, VertexDeclaration> mDeclarations;
	};
}
