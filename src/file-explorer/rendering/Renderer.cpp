#include "Renderer.h"
#include <DirectXMath.h>
#include <array>
#include <chrono>
#include <d3dcompiler.h>
#include <gsl/gsl>
#include <string_view>
#include <thread>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct VSConstantBuffer
{
	XMFLOAT4X4 mWorldViewProj;
};

constexpr std::string_view Shader =
	"cbuffer VSConstantBuffer : register(b0)"
	"{"
	"	float4x4 mWorldViewProj;"
	"};"
	""
	"struct VSInput"
	"{"
	"	float3 position : POSITION;"
	"	float4 color : COLOR;"
	"};"
	""
	"struct VSOutput"
	"{"
	"	float4 position : SV_POSITION;"
	"	float4 color : COLOR;"
	"};"
	""
	"VSOutput VSMain(VSInput input)"
	"{"
	"	VSOutput output = (VSOutput)0;"
	"	output.position = mul(float4(input.position, 1.0), mWorldViewProj);"
	"	output.color = input.color;"
	"	return output;"
	"}"
	""
	"float4 PSMain(VSOutput input) : SV_TARGET"
	"{"
	"	return input.color;"
	"}";

CRenderer::CRenderer(HWND hwnd, const FRenderCallback& renderCB)
	: mHWND{ hwnd }, mHasDeviceResources{ false }, mRenderCallback{ renderCB }
{
	CreateDeviceResources();
}
CRenderer::~CRenderer()
{
	ReleaseDeviceResources();
}

void CRenderer::Clear(float r, float g, float b)
{
	const FLOAT color[4]{ r, g, b, 1.0f };
	mDeviceContext->ClearRenderTargetView(mBackBuffer.Get(), color);
}

static constexpr UINT VertexCount = 323;
static constexpr UINT VertexStride = (3 + 4) * sizeof(float);
static constexpr UINT VertexOffset = 0;
static constexpr UINT IndexCount = 454 * 3;

void CRenderer::CreateDeviceResources()
{
	if (mHasDeviceResources)
	{
		ReleaseDeviceResources();
	}

	constexpr UINT DeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG;
	const std::array<D3D_FEATURE_LEVEL, 2> FeatureLevels{ D3D_FEATURE_LEVEL_11_1,
														  D3D_FEATURE_LEVEL_11_0 };
	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = mHWND;
	swapChainDesc.SampleDesc.Count = 4;
	swapChainDesc.Windowed = true;
	Expects(SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr,
													D3D_DRIVER_TYPE_HARDWARE,
													nullptr,
													DeviceFlags,
													FeatureLevels.data(),
													gsl::narrow_cast<UINT>(FeatureLevels.size()),
													D3D11_SDK_VERSION,
													&swapChainDesc,
													&mSwapChain,
													&mDevice,
													nullptr,
													&mDeviceContext)));

	{
		ComPtr<ID3D11Texture2D> backBuffer;
		Expects(SUCCEEDED(mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer)));
		Expects(
			SUCCEEDED(mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &mBackBuffer)));
		mDeviceContext->OMSetRenderTargets(1, mBackBuffer.GetAddressOf(), nullptr);
	}

	// create shaders
	{
		ComPtr<ID3DBlob> vertexShaderBlob;
		ComPtr<ID3DBlob> pixelShaderBlob;
		if (ComPtr<ID3DBlob> errorBlob; FAILED(D3DCompile(Shader.data(),
														  Shader.size(),
														  "Builtin",
														  nullptr,
														  nullptr,
														  "VSMain",
														  "vs_4_0",
														  D3DCOMPILE_DEBUG,
														  0,
														  &vertexShaderBlob,
														  &errorBlob)))
		{
			OutputDebugStringA("Vertex shader compilation failed:\n");
			OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
			return;
		}

		if (ComPtr<ID3DBlob> errorBlob; FAILED(D3DCompile(Shader.data(),
														  Shader.size(),
														  "Builtin",
														  nullptr,
														  nullptr,
														  "PSMain",
														  "ps_4_0",
														  D3DCOMPILE_DEBUG,
														  0,
														  &pixelShaderBlob,
														  &errorBlob)))
		{
			OutputDebugStringA("Pixel shader compilation failed:\n");
			OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
			return;
		}

		Expects(SUCCEEDED(mDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
													  vertexShaderBlob->GetBufferSize(),
													  nullptr,
													  &mVertexShader)));

		Expects(SUCCEEDED(mDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
													 pixelShaderBlob->GetBufferSize(),
													 nullptr,
													 &mPixelShader)));

		const std::array<D3D11_INPUT_ELEMENT_DESC, 2> inputLayoutDesc{
			{ { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			  { "COLOR",
				0,
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				0,
				3 * sizeof(float),
				D3D11_INPUT_PER_VERTEX_DATA,
				0 } }
		};

		Expects(SUCCEEDED(mDevice->CreateInputLayout(inputLayoutDesc.data(),
													 inputLayoutDesc.size(),
													 vertexShaderBlob->GetBufferPointer(),
													 vertexShaderBlob->GetBufferSize(),
													 &mInputLayout)));
	}

	// create vertex buffer
	{
		// clang-format off
		const std::array<float, (3 + 4) * VertexCount> vertices{
			//0.0f,  0.5f,  0.0f, 0.5f, 0.5f, 0.5f, 1.0f, // point at top
			//0.5f,  -0.5f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, // point at bottom-right
			//-0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, // point at bottom-left

			0.206000f,  0.383618f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.188299f,  0.398114f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.184942f,  0.453261f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.222327f,  0.422620f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.170598f,  0.383618f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.147526f,  0.422620f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.163274f,  0.348582f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.132054f,  0.348582f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.170598f,  0.313578f,  -0.129765f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.147526f,  0.274575f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.188299f,  0.299081f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.184942f,  0.243904f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.206000f,  0.313578f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.222327f,  0.274575f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.213324f,  0.348582f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.237800f,  0.348582f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.206000f,  0.383618f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.222327f,  0.422620f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.188299f,  0.348582f,  -0.163884f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.209693f,  0.397626f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.184942f,  0.417920f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.184942f,  0.348582f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.160161f,  0.397626f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.149907f,  0.348582f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.160161f,  0.299570f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.184942f,  0.279244f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.209693f,  0.299570f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.219947f,  0.348582f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.209693f,  0.397626f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.222327f,  0.422620f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.184942f,  0.453261f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.147526f,  0.422620f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.132054f,  0.348582f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.147526f,  0.274575f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.184942f,  0.243904f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.222327f,  0.274575f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.237800f,  0.348582f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.222327f,  0.422620f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.235511f,  0.353099f,  0.042817f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.956999f,  0.480392f,  -0.183386f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.956511f,  0.497421f,  -0.187719f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.234687f,  0.392804f,  0.028748f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.919706f,  0.493210f,  -0.280160f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.282998f,  0.382550f,  -0.268838f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.131230f,  0.344218f,  -0.286477f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.287759f,  0.361827f,  -0.285195f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.291910f,  0.343364f,  -0.277779f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.131230f,  0.308115f,  -0.279061f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.236549f,  0.303873f,  0.028748f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.985504f,  0.487930f,  -0.192328f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.985351f,  0.499008f,  -0.196814f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.131230f,  0.308115f,  -0.279061f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.236549f,  0.303873f,  0.028748f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.001068f,  0.301798f,  0.021119f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.000183f,  0.346171f,  -0.329722f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.095981f,  0.392804f,  0.022553f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.045747f,  0.344646f,  0.035859f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.093142f,  0.390149f,  -0.295450f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.055055f,  0.422834f,  -0.320750f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.063204f,  0.413160f,  0.021119f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.057283f,  0.377697f,  -0.321512f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.982238f,  0.478591f,  -0.264016f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.982147f,  0.486618f,  -0.264016f, 0.5f, 0.5f, 0.5f, 1.0f,
			 1.000000f,  0.486007f,  -0.227363f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.999786f,  0.501328f,  -0.227363f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.981994f,  0.499008f,  -0.264016f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.981994f,  0.499008f,  -0.264016f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.985351f,  0.499008f,  -0.196814f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.999786f,  0.501328f,  -0.227363f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.985626f,  0.478591f,  -0.196814f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.982238f,  0.478591f,  -0.264016f, 0.5f, 0.5f, 0.5f, 1.0f,
			 1.000000f,  0.486007f,  -0.227363f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.985626f,  0.478591f,  -0.196814f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.919187f,  0.468551f,  -0.280160f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.919370f,  0.478133f,  -0.280160f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.131230f,  0.377850f,  -0.270119f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.919370f,  0.478133f,  -0.280160f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.919187f,  0.468551f,  -0.280160f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.957305f,  0.468551f,  -0.187719f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.131230f,  0.377850f,  -0.270119f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.131230f,  0.344218f,  -0.286477f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.957305f,  0.468551f,  -0.187719f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.919706f,  0.493210f,  -0.280160f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.206030f,  0.383618f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.222358f,  0.422620f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.184973f,  0.453261f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.188330f,  0.398114f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.147557f,  0.422620f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.170629f,  0.383618f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.132084f,  0.348582f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.163305f,  0.348582f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.147557f,  0.274575f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.170629f,  0.313578f,  -0.129765f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.184973f,  0.243904f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.188330f,  0.299081f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.222358f,  0.274575f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.206030f,  0.313578f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.237831f,  0.348582f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.213355f,  0.348582f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.222358f,  0.422620f,  0.034089f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.206030f,  0.383618f,  -0.070650f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.188330f,  0.348582f,  -0.163884f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.209723f,  0.397626f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.184973f,  0.348582f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.184973f,  0.417920f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.160192f,  0.397626f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.149937f,  0.348582f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.160192f,  0.299570f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.184973f,  0.279244f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.209723f,  0.299570f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.219977f,  0.348582f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.209723f,  0.397626f,  0.155583f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.222358f,  0.422620f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.184973f,  0.453261f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.147557f,  0.422620f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.132084f,  0.348582f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.147557f,  0.274575f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.184973f,  0.243904f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.222358f,  0.274575f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.237831f,  0.348582f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.222358f,  0.422620f,  0.138768f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.235542f,  0.353099f,  0.042817f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.234718f,  0.392804f,  0.028748f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.956542f,  0.497421f,  -0.187719f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.957030f,  0.480392f,  -0.183386f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.283029f,  0.382550f,  -0.268838f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.919736f,  0.493210f,  -0.280160f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.131260f,  0.344218f,  -0.286477f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.131260f,  0.308115f,  -0.279061f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.291940f,  0.343364f,  -0.277779f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.287790f,  0.361827f,  -0.285195f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.236579f,  0.303873f,  0.028748f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.985382f,  0.499008f,  -0.196814f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.985534f,  0.487930f,  -0.192328f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.131260f,  0.308115f,  -0.279061f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000153f,  0.346171f,  -0.329722f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.001099f,  0.301798f,  0.021119f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.236579f,  0.303873f,  0.028748f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.045778f,  0.344646f,  0.035859f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.096011f,  0.392804f,  0.022553f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.063234f,  0.413160f,  0.021119f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.055086f,  0.422834f,  -0.320750f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.093173f,  0.390149f,  -0.295450f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.057314f,  0.377697f,  -0.321512f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.982269f,  0.478591f,  -0.264016f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -1.000000f,  0.486007f,  -0.227363f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.982177f,  0.486618f,  -0.264016f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.999817f,  0.501328f,  -0.227363f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.982025f,  0.499008f,  -0.264016f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.982025f,  0.499008f,  -0.264016f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.999817f,  0.501328f,  -0.227363f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.985382f,  0.499008f,  -0.196814f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.985656f,  0.478591f,  -0.196814f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -1.000000f,  0.486007f,  -0.227363f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.982269f,  0.478591f,  -0.264016f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.985656f,  0.478591f,  -0.196814f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.919401f,  0.478133f,  -0.280160f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.919218f,  0.468551f,  -0.280160f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.131260f,  0.377850f,  -0.270119f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.919218f,  0.468551f,  -0.280160f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.919401f,  0.478133f,  -0.280160f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.957335f,  0.468551f,  -0.187719f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.131260f,  0.377850f,  -0.270119f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.131260f,  0.344218f,  -0.286477f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.957335f,  0.468551f,  -0.187719f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.919736f,  0.493210f,  -0.280160f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.036897f,  0.547746f,  -0.759728f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.095523f,  0.547746f,  -0.795312f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.095523f,  0.581317f,  -0.795312f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.036897f,  0.581317f,  -0.759728f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.059297f,  0.581317f,  -0.992065f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.017304f,  0.581317f,  -0.968200f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.059297f,  0.547746f,  -0.992065f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.017304f,  0.547746f,  -0.968200f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.095523f,  0.547746f,  -0.795312f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.036897f,  0.547746f,  -0.759728f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.275399f,  0.554735f,  -0.923307f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.261422f,  0.554735f,  -0.968413f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.261422f,  0.574328f,  -0.968413f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.275399f,  0.574328f,  -0.923307f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.255135f,  0.547746f,  -0.882290f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.255135f,  0.581317f,  -0.882290f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.229713f,  0.581317f,  -0.989471f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.229713f,  0.547746f,  -0.989471f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.255135f,  0.547746f,  -0.882290f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.229713f,  0.547746f,  -0.989471f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.229713f,  0.581317f,  -0.989471f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.036927f,  0.547746f,  -0.759728f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.036927f,  0.581317f,  -0.759728f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.095553f,  0.581317f,  -0.795312f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.095553f,  0.547746f,  -0.795312f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.017335f,  0.581317f,  -0.968200f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.059328f,  0.581317f,  -0.992065f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.017335f,  0.547746f,  -0.968200f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.059328f,  0.547746f,  -0.992065f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.036927f,  0.547746f,  -0.759728f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.095553f,  0.547746f,  -0.795312f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.275430f,  0.554735f,  -0.923307f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.275430f,  0.574328f,  -0.923307f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.261452f,  0.574328f,  -0.968413f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.261452f,  0.554735f,  -0.968413f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.255165f,  0.581317f,  -0.882290f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.255165f,  0.547746f,  -0.882290f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.229743f,  0.581317f,  -0.989471f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.229743f,  0.547746f,  -0.989471f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.255165f,  0.547746f,  -0.882290f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.229743f,  0.547746f,  -0.989471f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.229743f,  0.581317f,  -0.989471f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.025544f,  0.397595f,  0.370495f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.043428f,  0.390637f,  0.328288f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.057680f,  0.457045f,  0.329173f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.028047f,  0.452345f,  0.367809f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.010773f,  0.691214f,  -0.709922f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.044435f,  0.622974f,  -0.691855f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.025880f,  0.618366f,  -0.829005f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.007630f,  0.986175f,  -0.834346f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.055940f,  0.628956f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.082827f,  0.547838f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.083316f,  0.545183f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.057619f,  0.626698f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.050386f,  0.523026f,  0.317515f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.024873f,  0.490005f,  0.373638f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.021516f,  0.560747f,  -0.966399f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.028230f,  0.543199f,  -0.829890f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.028230f,  0.491195f,  -0.827204f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.050691f,  0.548906f,  -0.685110f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.073366f,  0.479629f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.052095f,  0.497543f,  -0.687124f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.080386f,  0.460799f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.047121f,  0.386212f,  0.235603f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.072756f,  0.461959f,  0.235603f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.063234f,  0.546495f,  0.235267f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.045076f,  0.619800f,  0.234748f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.025300f,  0.622578f,  0.281564f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.010773f,  0.691214f,  -0.709922f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.000031f,  0.691214f,  -0.699271f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.000031f,  0.672018f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.014466f,  0.577197f,  -0.962188f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.009400f,  0.961333f,  -0.938994f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.021516f,  0.560747f,  -0.966399f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.028230f,  0.491195f,  -0.827204f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.466781f,  -0.829005f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.536332f,  -1.000000f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.007630f,  0.986175f,  -0.834346f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  1.000000f,  -0.829005f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.045076f,  0.619800f,  0.234748f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.649129f,  0.234748f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.000031f,  0.629780f,  0.281564f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.047121f,  0.386212f,  0.235603f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.326121f,  0.235603f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.294473f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.061861f,  0.369274f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.012665f,  0.447249f,  0.384381f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.061861f,  0.369274f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.062014f,  0.381726f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.038362f,  0.435163f,  -0.689535f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.673055f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.055940f,  0.628956f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.000031f,  0.535966f,  0.323099f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.504349f,  0.378857f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.367901f,  0.381085f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.000031f,  0.344554f,  0.331889f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.043428f,  0.390637f,  0.328288f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.025544f,  0.397595f,  0.370495f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.310343f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.062014f,  0.381726f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.449202f,  0.402936f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.367901f,  0.381085f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.592822f,  -0.995575f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.980560f,  -0.943876f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.009400f,  0.961333f,  -0.938994f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.014466f,  0.577197f,  -0.962188f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.000000f,  0.398541f,  -0.700186f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.038362f,  0.435163f,  -0.689535f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.000031f,  0.672018f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 0.057619f,  0.626698f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.025575f,  0.397595f,  0.370495f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.028077f,  0.452345f,  0.367809f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.057711f,  0.457045f,  0.329173f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.043458f,  0.390637f,  0.328288f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.010804f,  0.691214f,  -0.709922f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.007660f,  0.986175f,  -0.834346f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.025910f,  0.618366f,  -0.829005f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.040529f,  0.634022f,  -0.691855f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.055147f,  0.632252f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.056825f,  0.629963f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.083346f,  0.545183f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.082858f,  0.547838f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.024903f,  0.490005f,  0.373638f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.050417f,  0.523026f,  0.317515f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.021546f,  0.560747f,  -0.966399f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.028260f,  0.491195f,  -0.827204f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.028260f,  0.543199f,  -0.829890f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.063417f,  0.548906f,  -0.685110f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.052126f,  0.497543f,  -0.687124f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.073397f,  0.479629f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.080416f,  0.460799f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.072787f,  0.461959f,  0.235603f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.047151f,  0.386212f,  0.235603f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.025330f,  0.622578f,  0.281564f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.041169f,  0.630848f,  0.234748f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.063265f,  0.546495f,  0.235267f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.010804f,  0.691214f,  -0.709922f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.014496f,  0.577197f,  -0.962188f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.009430f,  0.961333f,  -0.938994f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.007660f,  0.986175f,  -0.834346f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.041169f,  0.630848f,  0.234748f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.047151f,  0.386212f,  0.235603f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.061892f,  0.369274f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.012696f,  0.447249f,  0.384381f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.061892f,  0.369274f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.062044f,  0.381726f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.038392f,  0.435163f,  -0.689535f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.055147f,  0.632252f,  0.070559f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.028260f,  0.491195f,  -0.827204f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.021546f,  0.560747f,  -0.966399f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.025575f,  0.397595f,  0.370495f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.043458f,  0.390637f,  0.328288f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.062044f,  0.381726f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.014496f,  0.577197f,  -0.962188f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.009430f,  0.961333f,  -0.938994f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.038392f,  0.435163f,  -0.689535f, 0.5f, 0.5f, 0.5f, 1.0f,
			 -0.056825f,  0.629963f,  -0.287118f, 0.5f, 0.5f, 0.5f, 1.0f,
		};
		// clang-format on

		D3D11_BUFFER_DESC desc = { 0 };
		desc.ByteWidth = sizeof(vertices);
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		D3D11_SUBRESOURCE_DATA data = { 0 };
		data.pSysMem = vertices.data();
		Expects(SUCCEEDED(mDevice->CreateBuffer(&desc, &data, &mVertexBuffer)));
	}

	// create index buffer
	{
		// clang-format off
		std::array<uint16_t, IndexCount> indices{
			1, 2, 3,
			1, 3, 4,
			2, 5, 6,
			2, 6, 3,
			5, 7, 8,
			5, 8, 6,
			7, 9, 10,
			7, 10, 8,
			9, 11, 12,
			9, 12, 10,
			11, 13, 14,
			11, 14, 12,
			13, 15, 16,
			13, 16, 14,
			15, 17, 18,
			15, 18, 16,
			2, 1, 19,
			5, 2, 19,
			7, 5, 19,
			9, 7, 19,
			11, 9, 19,
			13, 11, 19,
			15, 13, 19,
			17, 15, 19,
			20, 21, 22,
			21, 23, 22,
			23, 24, 22,
			24, 25, 22,
			25, 26, 22,
			26, 27, 22,
			27, 28, 22,
			28, 29, 22,
			30, 31, 21,
			30, 21, 20,
			31, 32, 23,
			31, 23, 21,
			32, 33, 24,
			32, 24, 23,
			33, 34, 25,
			33, 25, 24,
			34, 35, 26,
			34, 26, 25,
			35, 36, 27,
			35, 27, 26,
			36, 37, 28,
			36, 28, 27,
			37, 38, 29,
			37, 29, 28,
			4, 3, 31,
			4, 31, 30,
			3, 6, 32,
			3, 32, 31,
			6, 8, 33,
			6, 33, 32,
			8, 10, 34,
			8, 34, 33,
			10, 12, 35,
			10, 35, 34,
			12, 14, 36,
			12, 36, 35,
			14, 16, 37,
			14, 37, 36,
			16, 18, 38,
			16, 38, 37,
			39, 40, 41,
			39, 41, 42,
			42, 41, 43,
			42, 43, 44,
			45, 46, 47,
			45, 47, 48,
			48, 47, 49,
			40, 50, 51,
			40, 51, 41,
			52, 53, 54,
			52, 54, 55,
			39, 42, 56,
			39, 56, 57,
			56, 58, 59,
			56, 59, 60,
			58, 61, 55,
			58, 55, 59,
			62, 63, 64,
			65, 64, 63,
			65, 63, 66,
			67, 68, 69,
			70, 71, 72,
			73, 64, 65,
			73, 65, 50,
			51, 50, 65,
			63, 62, 74,
			63, 74, 75,
			42, 44, 76,
			47, 46, 77,
			47, 77, 78,
			71, 70, 79,
			71, 79, 78,
			49, 47, 78,
			49, 78, 79,
			42, 80, 58,
			42, 58, 56,
			81, 52, 55,
			81, 55, 61,
			54, 57, 56,
			54, 56, 60,
			53, 39, 57,
			53, 57, 54,
			53, 82, 40,
			53, 40, 39,
			82, 73, 50,
			82, 50, 40,
			43, 41, 68,
			43, 68, 67,
			80, 81, 61,
			80, 61, 58,
			76, 44, 46,
			76, 46, 45,
			77, 46, 44,
			77, 44, 43,
			66, 63, 75,
			66, 75, 83,
			84, 85, 86,
			84, 86, 87,
			87, 86, 88,
			87, 88, 89,
			89, 88, 90,
			89, 90, 91,
			91, 90, 92,
			91, 92, 93,
			93, 92, 94,
			93, 94, 95,
			95, 94, 96,
			95, 96, 97,
			97, 96, 98,
			97, 98, 99,
			99, 98, 100,
			99, 100, 101,
			87, 102, 84,
			89, 102, 87,
			91, 102, 89,
			93, 102, 91,
			95, 102, 93,
			97, 102, 95,
			99, 102, 97,
			101, 102, 99,
			103, 104, 105,
			105, 104, 106,
			106, 104, 107,
			107, 104, 108,
			108, 104, 109,
			109, 104, 110,
			110, 104, 111,
			111, 104, 112,
			113, 103, 105,
			113, 105, 114,
			114, 105, 106,
			114, 106, 115,
			115, 106, 107,
			115, 107, 116,
			116, 107, 108,
			116, 108, 117,
			117, 108, 109,
			117, 109, 118,
			118, 109, 110,
			118, 110, 119,
			119, 110, 111,
			119, 111, 120,
			120, 111, 112,
			120, 112, 121,
			85, 113, 114,
			85, 114, 86,
			86, 114, 115,
			86, 115, 88,
			88, 115, 116,
			88, 116, 90,
			90, 116, 117,
			90, 117, 92,
			92, 117, 118,
			92, 118, 94,
			94, 118, 119,
			94, 119, 96,
			96, 119, 120,
			96, 120, 98,
			98, 120, 121,
			98, 121, 100,
			122, 123, 124,
			122, 124, 125,
			123, 126, 127,
			123, 127, 124,
			128, 129, 130,
			128, 130, 131,
			129, 132, 130,
			125, 124, 133,
			125, 133, 134,
			135, 136, 137,
			135, 137, 138,
			122, 139, 140,
			122, 140, 123,
			140, 141, 142,
			140, 142, 143,
			143, 142, 136,
			143, 136, 144,
			145, 146, 147,
			147, 146, 148,
			147, 148, 149,
			150, 151, 152,
			153, 154, 155,
			148, 146, 156,
			148, 156, 134,
			133, 148, 134,
			147, 157, 158,
			147, 158, 145,
			123, 159, 126,
			130, 160, 161,
			130, 161, 131,
			162, 153, 155,
			162, 155, 160,
			160, 130, 132,
			160, 132, 162,
			123, 140, 143,
			123, 143, 163,
			164, 144, 136,
			164, 136, 135,
			137, 141, 140,
			137, 140, 139,
			138, 137, 139,
			138, 139, 122,
			138, 122, 125,
			138, 125, 165,
			165, 125, 134,
			165, 134, 156,
			127, 150, 152,
			127, 152, 124,
			163, 143, 144,
			163, 144, 164,
			159, 128, 131,
			159, 131, 126,
			161, 127, 126,
			161, 126, 131,
			149, 166, 157,
			149, 157, 147,
			167, 168, 169,
			167, 169, 170,
			170, 169, 171,
			170, 171, 172,
			172, 171, 173,
			172, 173, 174,
			174, 173, 175,
			174, 175, 176,
			177, 178, 179,
			177, 179, 180,
			169, 168, 181,
			169, 181, 182,
			171, 169, 182,
			171, 182, 183,
			173, 171, 183,
			173, 183, 184,
			175, 173, 184,
			175, 184, 185,
			181, 186, 178,
			181, 178, 177,
			186, 187, 179,
			186, 179, 178,
			187, 182, 180,
			187, 180, 179,
			182, 181, 177,
			182, 177, 180,
			188, 189, 190,
			188, 190, 191,
			189, 192, 193,
			189, 193, 190,
			192, 194, 195,
			192, 195, 193,
			194, 196, 197,
			194, 197, 195,
			198, 199, 200,
			198, 200, 201,
			190, 202, 203,
			190, 203, 191,
			193, 204, 202,
			193, 202, 190,
			195, 205, 204,
			195, 204, 193,
			197, 206, 205,
			197, 205, 195,
			203, 198, 201,
			203, 201, 207,
			207, 201, 200,
			207, 200, 208,
			208, 200, 199,
			208, 199, 202,
			202, 199, 198,
			202, 198, 203,
			209, 210, 211,
			209, 211, 212,
			213, 214, 215,
			213, 215, 216,
			217, 218, 219,
			217, 219, 220,
			212, 211, 221,
			212, 221, 222,
			223, 224, 225,
			226, 219, 227,
			226, 227, 228,
			219, 218, 229,
			219, 229, 227,
			211, 210, 230,
			211, 230, 231,
			221, 232, 233,
			221, 233, 234,
			214, 235, 220,
			220, 235, 236,
			220, 236, 237,
			220, 219, 226,
			220, 226, 214,
			238, 215, 224,
			238, 224, 223,
			216, 215, 238,
			216, 238, 239,
			228, 224, 226,
			240, 241, 242,
			240, 242, 243,
			244, 245, 236,
			244, 236, 235,
			224, 215, 214,
			224, 214, 226,
			231, 229, 218,
			231, 218, 232,
			233, 232, 218,
			233, 218, 217,
			246, 247, 248,
			246, 248, 234,
			249, 250, 251,
			249, 251, 252,
			253, 212, 222,
			209, 212, 253,
			221, 211, 231,
			221, 231, 232,
			231, 230, 254,
			231, 254, 229,
			227, 229, 254,
			227, 254, 255,
			228, 227, 255,
			228, 255, 256,
			225, 224, 228,
			225, 228, 256,
			257, 247, 246,
			257, 246, 258,
			259, 260, 222,
			259, 222, 221,
			261, 262, 263,
			261, 263, 264,
			248, 259, 221,
			248, 221, 234,
			251, 265, 266,
			251, 266, 252,
			262, 250, 249,
			262, 249, 263,
			267, 268, 209,
			267, 209, 253,
			269, 270, 271,
			269, 271, 272,
			273, 242, 241,
			273, 241, 274,
			270, 245, 244,
			270, 244, 271,
			265, 273, 274,
			265, 274, 266,
			275, 257, 258,
			275, 258, 276,
			243, 269, 272,
			243, 272, 240,
			260, 267, 253,
			260, 253, 222,
			277, 278, 279,
			277, 279, 280,
			281, 282, 283,
			281, 283, 284,
			285, 286, 287,
			285, 287, 288,
			278, 289, 290,
			278, 290, 279,
			291, 292, 293,
			294, 295, 296,
			294, 296, 287,
			287, 296, 297,
			287, 297, 288,
			279, 298, 299,
			279, 299, 280,
			290, 300, 301,
			290, 301, 302,
			284, 286, 303,
			236, 303, 286,
			236, 286, 237,
			286, 284, 294,
			286, 294, 287,
			304, 291, 293,
			304, 293, 283,
			282, 305, 304,
			282, 304, 283,
			295, 294, 293,
			306, 303, 236,
			306, 236, 245,
			293, 294, 284,
			293, 284, 283,
			298, 302, 288,
			298, 288, 297,
			301, 285, 288,
			301, 288, 302,
			307, 300, 248,
			307, 248, 247,
			308, 309, 251,
			308, 251, 250,
			310, 289, 278,
			277, 310, 278,
			290, 302, 298,
			290, 298, 279,
			298, 297, 311,
			298, 311, 299,
			296, 312, 311,
			296, 311, 297,
			295, 313, 312,
			295, 312, 296,
			292, 313, 295,
			292, 295, 293,
			257, 314, 307,
			257, 307, 247,
			259, 290, 289,
			259, 289, 260,
			315, 316, 243,
			315, 243, 242,
			261, 317, 318,
			261, 318, 262,
			248, 300, 290,
			248, 290, 259,
			251, 309, 319,
			251, 319, 265,
			262, 318, 308,
			262, 308, 250,
			267, 310, 277,
			267, 277, 268,
			269, 320, 321,
			269, 321, 270,
			273, 322, 315,
			273, 315, 242,
			270, 321, 306,
			270, 306, 245,
			265, 319, 322,
			265, 322, 273,
			275, 323, 314,
			275, 314, 257,
			243, 316, 320,
			243, 320, 269,
			260, 289, 310,
			260, 310, 267,
		};
		// clang-format on

		for (uint16_t& i : indices)
		{
			i -= 1;
		}

		D3D11_BUFFER_DESC desc = { 0 };
		desc.ByteWidth = sizeof(indices);
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		D3D11_SUBRESOURCE_DATA data = { 0 };
		data.pSysMem = indices.data();
		Expects(SUCCEEDED(mDevice->CreateBuffer(&desc, &data, &mIndexBuffer)));
	}

	// create constant buffer
	{
		VSConstantBuffer initData;
		XMStoreFloat4x4(&initData.mWorldViewProj, XMMatrixIdentity());

		D3D11_BUFFER_DESC desc = { 0 };
		desc.ByteWidth = sizeof(VSConstantBuffer);
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		D3D11_SUBRESOURCE_DATA data = { 0 };
		data.pSysMem = &initData;
		Expects(SUCCEEDED(mDevice->CreateBuffer(&desc, &data, &mConstantBuffer)));
	}

	mRenderingThreadRunning = true;
	mRenderingThread = std::thread{ &CRenderer::RenderThreadStart, this };

	mHasDeviceResources = true;
}

void CRenderer::ReleaseDeviceResources()
{
	mRenderingThreadRunning = false;
	if (mRenderingThread.joinable())
	{
		mRenderingThread.join();
	}

	if (!mHasDeviceResources)
	{
		return;
	}

	mConstantBuffer.Reset();
	mVertexBuffer.Reset();
	mIndexBuffer.Reset();
	mVertexShader.Reset();
	mPixelShader.Reset();
	mInputLayout.Reset();
	mBackBuffer.Reset();
	mSwapChain.Reset();
	mDeviceContext.Reset();
	mDevice.Reset();

	mHasDeviceResources = false;
}

static bool UpdateCamera(CCamera& c, float frameTime)
{
	constexpr float Speed{ 10.0f };
	constexpr float RotSpeed{ 0.5f };

	bool changed = false;

	Vector3 offset = Vector3::Zero;
	if (GetAsyncKeyState('W') & 0x8000)
	{
		offset += Vector3::Forward * Speed * frameTime;
		changed = true;
	}

	if (GetAsyncKeyState('S') & 0x8000)
	{
		offset += Vector3::Backward * Speed * frameTime;
		changed = true;
	}

	if (GetAsyncKeyState('A') & 0x8000)
	{
		offset += Vector3::Left * Speed * frameTime;
		changed = true;
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		offset += Vector3::Right * Speed * frameTime;
		changed = true;
	}

	if (GetAsyncKeyState('Q') & 0x8000)
	{
		offset += Vector3::Down * Speed * frameTime;
		changed = true;
	}

	if (GetAsyncKeyState('E') & 0x8000)
	{
		offset += Vector3::Up * Speed * frameTime;
		changed = true;
	}

	float yaw = 0.0f, pitch = 0.0f;
	if (GetAsyncKeyState('Z') & 0x8000)
	{
		yaw += RotSpeed * frameTime;
		changed = true;
	}

	if (GetAsyncKeyState('X') & 0x8000)
	{
		yaw -= RotSpeed * frameTime;
		changed = true;
	}

	if (GetAsyncKeyState('T') & 0x8000)
	{
		pitch += RotSpeed * frameTime;
		changed = true;
	}

	if (GetAsyncKeyState('G') & 0x8000)
	{
		pitch -= RotSpeed * frameTime;
		changed = true;
	}

	const Matrix delta =
		Matrix::CreateFromYawPitchRoll(yaw, pitch, 0.0f) * Matrix::CreateTranslation(offset);
	c.Transform(delta * c.Transform());

	if (changed)
	{
		char b[512];
		std::snprintf(b,
					  std::size(b),
					  "Cam position: (%.3f, %.3f, %.3f)\n",
					  c.Transform().Translation().x,
					  c.Transform().Translation().y,
					  c.Transform().Translation().z);
		OutputDebugStringA(b);
	}

	return changed;
}

void CRenderer::WorldMtx(const Matrix& world)
{
	D3D11_MAPPED_SUBRESOURCE m = { 0 };
	Expects(
		SUCCEEDED(mDeviceContext->Map(mConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m)));

	XMStoreFloat4x4(&reinterpret_cast<VSConstantBuffer*>(m.pData)->mWorldViewProj,
					(world * mCam.ViewProjection()).Transpose());

	mDeviceContext->Unmap(mConstantBuffer.Get(), 0);
}

void CRenderer::WorldDefault()
{
	WorldMtx(Matrix::Identity);
}

void CRenderer::RenderThreadStart()
{
	using namespace std::chrono;
	using namespace std::chrono_literals;

	time_point prevFrame = steady_clock::now();
	float frameTime = 0.0f;

	RECT clientRect = { 0 };
	while (mRenderingThreadRunning)
	{
		bool camChanged = false;

		{
			RECT currClientRect = { 0 };
			GetClientRect(mHWND, &currClientRect);
			const LONG oldW = (clientRect.right - clientRect.left);
			const LONG oldH = (clientRect.bottom - clientRect.top);
			const LONG newW = (currClientRect.right - currClientRect.left);
			const LONG newH = (currClientRect.bottom - currClientRect.top);

			if (oldW != newW || oldH != newH)
			{
				clientRect = currClientRect;

				mDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
				mBackBuffer.Reset();

				Expects(SUCCEEDED(mSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0)));

				ComPtr<ID3D11Texture2D> backBuffer;
				Expects(
					SUCCEEDED(mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer)));
				Expects(SUCCEEDED(
					mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &mBackBuffer)));

				mCam.Resize(static_cast<size_t>(newW), static_cast<size_t>(newH));

				camChanged = true;
			}
		}

		camChanged |= UpdateCamera(mCam, frameTime);

		mRenderCallback(*this, frameTime);

		// update viewport
		{
			D3D11_VIEWPORT viewport = { 0 };
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			viewport.Width = gsl::narrow_cast<FLOAT>(clientRect.right - clientRect.left);
			viewport.Height = gsl::narrow_cast<FLOAT>(clientRect.bottom - clientRect.top);
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			mDeviceContext->RSSetViewports(1, &viewport);
		}

		mDeviceContext->OMSetRenderTargets(1, mBackBuffer.GetAddressOf(), nullptr);

		mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mDeviceContext->IASetInputLayout(mInputLayout.Get());
		mDeviceContext->IASetVertexBuffers(0,
										   1,
										   mVertexBuffer.GetAddressOf(),
										   &VertexStride,
										   &VertexOffset);
		mDeviceContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

		mDeviceContext->VSSetConstantBuffers(0, 1, mConstantBuffer.GetAddressOf());
		mDeviceContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
		mDeviceContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

		static Matrix objMatrix{ Matrix::Identity };
		objMatrix *= Matrix::CreateFromAxisAngle(Vector3::Up, frameTime);

		WorldMtx(objMatrix);
		mDeviceContext->DrawIndexed(IndexCount, 0, 0);

		for (float i = 1.0f; i < 100.0f; i += 1.0f)
		{
			WorldMtx(objMatrix * Matrix::CreateFromAxisAngle(Vector3::Up, -(i / 50.0f)) *
					 Matrix::CreateTranslation(0.0f, i, 0.0f));
			mDeviceContext->DrawIndexed(IndexCount, 0, 0);

			WorldMtx(objMatrix * Matrix::CreateFromAxisAngle(Vector3::Up, (i / 50.0f)) *
					 Matrix::CreateTranslation(0.0f, -i, 0.0f));
			mDeviceContext->DrawIndexed(IndexCount, 0, 0);
		}
		Expects(SUCCEEDED(mSwapChain->Present(1, 0)));

		const time_point currFrame = steady_clock::now();
		frameTime = duration_cast<duration<float>>(currFrame - prevFrame).count();
		prevFrame = currFrame;
	}
}
