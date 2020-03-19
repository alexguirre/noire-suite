#include "VertexDeclaration.h"
#include <algorithm>
#include <d3dcompiler.h>
#include <string>
#include <string_view>

namespace noire::trunk
{
	static const char* SemanticToName(VertexElementSemantic s)
	{
		switch (s)
		{
		case VertexElementSemantic::POSITION: return "POSITION";
		case VertexElementSemantic::BLENDWEIGHT: return "BLENDWEIGHT";
		case VertexElementSemantic::BLENDINDICES: return "BLENDINDICES";
		case VertexElementSemantic::NORMAL: return "NORMAL";
		case VertexElementSemantic::PSIZE: return "PSIZE";
		case VertexElementSemantic::TEXCOORD0:
		case VertexElementSemantic::TEXCOORD1:
		case VertexElementSemantic::TEXCOORD2:
		case VertexElementSemantic::TEXCOORD3: return "TEXCOORD";
		case VertexElementSemantic::TANGENT: return "TANGENT";
		case VertexElementSemantic::BINORMAL: return "BINORMAL";
		case VertexElementSemantic::COLOR: return "COLOR";
		default: return "UNKNOWN";
		}
	}

	static u32 SemanticToIndex(VertexElementSemantic s)
	{
		switch (s)
		{
		case VertexElementSemantic::TEXCOORD1: return 1;
		case VertexElementSemantic::TEXCOORD2: return 2;
		case VertexElementSemantic::TEXCOORD3: return 3;
		default: return 0;
		}
	}

	static void RegisterDeclarations(std::unordered_map<u32, VertexDeclaration>& decls)
	{
		decls.clear();

		using S = VertexElementSemantic;
		using Decl = VertexDeclaration;

		// clang-format off
		decls.emplace(0x0000C090, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R16G16B16A16_SNORM,	S::POSITION },
		} });
		decls.emplace(0x0000C0D8, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R32G32B32A32_FLOAT,	S::POSITION },
		} });
		decls.emplace(0x00C01852, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R32G32B32A32_FLOAT,	S::POSITION },
			{ 1, 0x00, DXGI_FORMAT_R32G32_FLOAT,		S::TEXCOORD0 },
		} });
		decls.emplace(0x00C01856, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R32G32B32A32_FLOAT,	S::POSITION },
			{ 1, 0x00, DXGI_FORMAT_R32G32B32_FLOAT,		S::NORMAL },
		} });
		decls.emplace(0x00C01AD2, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R32G32B32A32_FLOAT,	S::POSITION },
			{ 0, 0x10, DXGI_FORMAT_R32G32_FLOAT,		S::TEXCOORD0 },
		} });
		decls.emplace(0x00C01AD6, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R32G32B32A32_FLOAT,	S::POSITION },
			{ 0, 0x10, DXGI_FORMAT_R32G32B32_FLOAT,		S::NORMAL },
		} });
		// ...
		decls.emplace(0x02543050, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R16G16B16A16_SNORM,	S::POSITION },
			{ 0, 0x08, DXGI_FORMAT_R16G16B16A16_FLOAT,	S::NORMAL },
			{ 0, 0x10, DXGI_FORMAT_R16G16B16A16_FLOAT,	S::TANGENT },
			{ 0, 0x18, DXGI_FORMAT_R8G8B8A8_UNORM,		S::BLENDWEIGHT },
			{ 0, 0x1C, DXGI_FORMAT_R16G16_FLOAT,		S::TEXCOORD0 },
			{ 0, 0x20, DXGI_FORMAT_R16G16_FLOAT,		S::TEXCOORD1 },
			{ 0, 0x24, DXGI_FORMAT_R16G16_FLOAT,		S::TEXCOORD2 },
		} });
		// ...
		decls.emplace(0x456FD77B, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R16G16B16A16_SNORM,	S::POSITION },
			{ 0, 0x08, DXGI_FORMAT_R16G16B16A16_FLOAT,	S::NORMAL },
			{ 0, 0x10, DXGI_FORMAT_R8G8B8A8_UNORM,		S::BLENDWEIGHT },
			{ 0, 0x14, DXGI_FORMAT_R8G8B8A8_UINT,		S::BLENDINDICES },
			{ 0, 0x18, DXGI_FORMAT_R16G16_FLOAT,		S::TEXCOORD0 },
			{ 0, 0x1C, DXGI_FORMAT_R16G16_FLOAT,		S::TEXCOORD1 },
		} });
		// ...
		decls.emplace(0x51B84502, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R16G16B16A16_SNORM,	S::POSITION },
			{ 0, 0x08, DXGI_FORMAT_R16G16B16A16_FLOAT,	S::NORMAL },
			{ 0, 0x10, DXGI_FORMAT_R8G8B8A8_UNORM,		S::BLENDWEIGHT },
			{ 0, 0x14, DXGI_FORMAT_R16G16_FLOAT,		S::TEXCOORD0 },
		} });
		// ...
		decls.emplace(0x51B88C82, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R16G16B16A16_SNORM,	S::POSITION },
			{ 0, 0x08, DXGI_FORMAT_R16G16B16A16_FLOAT,	S::NORMAL },
			{ 0, 0x10, DXGI_FORMAT_R16G16B16A16_FLOAT,	S::TANGENT },
			{ 0, 0x18, DXGI_FORMAT_R16G16_FLOAT,		S::TEXCOORD0 },
		} });
		// ...
		decls.emplace(0x8C05FEFA, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R16G16B16A16_SNORM,	S::POSITION },
			{ 0, 0x08, DXGI_FORMAT_R16G16B16A16_FLOAT,	S::NORMAL },
			{ 0, 0x10, DXGI_FORMAT_R16G16B16A16_FLOAT,	S::TANGENT },
			{ 0, 0x18, DXGI_FORMAT_R8G8B8A8_UNORM,		S::BLENDWEIGHT },
			{ 0, 0x1C, DXGI_FORMAT_R8G8B8A8_UINT,		S::BLENDINDICES },
			{ 0, 0x20, DXGI_FORMAT_R16G16_FLOAT,		S::TEXCOORD0 },
		} });
		// ...
		decls.emplace(0xB8456F13, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R16G16B16A16_SNORM,	S::POSITION },
			{ 0, 0x08, DXGI_FORMAT_R16G16B16A16_FLOAT,	S::NORMAL },
			{ 0, 0x10, DXGI_FORMAT_R8G8B8A8_UNORM,		S::BLENDWEIGHT },
			{ 0, 0x14, DXGI_FORMAT_R8G8B8A8_UINT,		S::BLENDINDICES },
			{ 0, 0x18, DXGI_FORMAT_R16G16_FLOAT,		S::TEXCOORD0 },
		} });
		// ...
		decls.emplace(0xB88C0293, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R16G16B16A16_SNORM,	S::POSITION },
			{ 0, 0x08, DXGI_FORMAT_R16G16B16A16_FLOAT,	S::NORMAL },
			{ 0, 0x10, DXGI_FORMAT_R16G16B16A16_FLOAT,	S::TANGENT },
			{ 0, 0x18, DXGI_FORMAT_R8G8B8A8_UNORM,		S::BLENDWEIGHT },
			{ 0, 0x1C, DXGI_FORMAT_R16G16_FLOAT,		S::TEXCOORD0 },
		} });
		// ...
		decls.emplace(0xC051BB42, Decl{ {
			{ 0, 0x00, DXGI_FORMAT_R16G16B16A16_SNORM,	S::POSITION },
			{ 0, 0x08, DXGI_FORMAT_R16G16B16A16_FLOAT,	S::NORMAL },
			{ 0, 0x10, DXGI_FORMAT_R16G16_FLOAT,		S::TEXCOORD0 },
		} });
		// ...
		// clang-format on

		Expects(std::all_of(decls.begin(), decls.end(), [](auto& d) {
			return std::is_sorted(d.second.Elements.begin(),
								  d.second.Elements.end(),
								  [](auto& a, auto& b) { return a.Offset < b.Offset; });
		}));
	}

	static int GetFormatDimension(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return 4;

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		case DXGI_FORMAT_B5G6R5_UNORM: return 3;

		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM: return 2;

		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_R1_UNORM:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM: return 1;

		default: Expects(false);
		}
	}

	static u32 GetFormatByteSize(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT: return 16;

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT: return 12;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT: return 8;

		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM: return 4;

		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM: return 2;

		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM: return 1;

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM: return 16;

		case DXGI_FORMAT_R1_UNORM:
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM: return 8;

		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP: return 4;

		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM: return 4;

		case DXGI_FORMAT_UNKNOWN:
		default: Expects(false);
		}
	}

	static const char* GetFormatDataType(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R32_FLOAT: return "float";

		default: return "int";
		}
	}

	static std::string GenerateInputLayoutShader(const std::vector<VertexElement>& elements)
	{
		std::string s{};
		s += "struct VS_INPUT\n"
			 "{\n";
		int i = 0;
		for (const VertexElement& e : elements)
		{
			char buffer[128];

			std::snprintf(buffer,
						  std::size(buffer),
						  "\t%s%d v%d : %s%d;\n",
						  GetFormatDataType(e.Format),
						  GetFormatDimension(e.Format),
						  i++,
						  SemanticToName(e.Semantic),
						  SemanticToIndex(e.Semantic));

			s += buffer;
		}
		s += "};\n";

		s += "struct VS_OUTPUT\n"
			 "{\n"
			 "	float4 v0 : POSITION;\n"
			 "};\n"
			 ""
			 "VS_OUTPUT VSMain(in VS_INPUT input)\n"
			 "{\n"
			 "	VS_OUTPUT output = (VS_OUTPUT)0;\n"
			 "	return output;\n"
			 "}\n";

		return s;
	}

	static void CreateInputLayout(ID3D11Device* device,
								  const std::vector<VertexElement>& elements,
								  ID3D11InputLayout** dest)
	{
		Expects(device != nullptr);
		Expects(dest != nullptr);

		std::vector<D3D11_INPUT_ELEMENT_DESC> descs{};
		descs.reserve(elements.size());
		std::transform(elements.begin(),
					   elements.end(),
					   std::back_inserter(descs),
					   std::mem_fn(&VertexElement::ToDesc));

		const std::string shader = GenerateInputLayoutShader(elements);
		OutputDebugStringA(shader.c_str());

		winrt::com_ptr<ID3DBlob> shaderBlob;
		if (winrt::com_ptr<ID3DBlob> errorBlob; FAILED(D3DCompile(shader.data(),
																  shader.size(),
																  "InputLayoutShader",
																  nullptr,
																  nullptr,
																  "VSMain",
																  "vs_4_0",
																  0,
																  0,
																  shaderBlob.put(),
																  errorBlob.put())))
		{
			OutputDebugStringA("Input layout shader compilation failed:\n");
			OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
			dest = nullptr;
			return;
		}

		const void* const bytecode = shaderBlob->GetBufferPointer();
		const size bytecodeSize = shaderBlob->GetBufferSize();

		device->CreateInputLayout(descs.data(), descs.size(), bytecode, bytecodeSize, dest);
	}

	u32 VertexDeclaration::VertexSize() const
	{
		return Elements.back().Offset + GetFormatByteSize(Elements.back().Format);
	}

	D3D11_INPUT_ELEMENT_DESC VertexElement::ToDesc() const
	{
		D3D11_INPUT_ELEMENT_DESC d;
		d.SemanticName = SemanticToName(Semantic);
		d.SemanticIndex = SemanticToIndex(Semantic);
		d.Format = Format;
		d.InputSlot = InputSlot;
		d.AlignedByteOffset = Offset;
		d.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		d.InstanceDataStepRate = 0;
		return d;
	}

	VertexDeclarationManager::VertexDeclarationManager() : mDevice{ nullptr }, mDeclarations{} {}

	void VertexDeclarationManager::Init(const winrt::com_ptr<ID3D11Device>& device)
	{
		mDevice = device;
		if (device)
		{
			RegisterDeclarations(mDeclarations);
		}
		else
		{
			mDeclarations.clear();
		}
	}

	void VertexDeclarationManager::Shutdown() { Init(nullptr); }

	const VertexDeclaration& VertexDeclarationManager::Get(u32 id)
	{
		auto it = mDeclarations.find(id);
		Expects(it != mDeclarations.end());

		VertexDeclaration& d = it->second;
		if (!d.InputLayout)
		{
			CreateInputLayout(mDevice.get(), d.Elements, d.InputLayout.put());
		}

		return d;
	}
}
