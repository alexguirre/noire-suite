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
	"	matrix mWorldViewProj;"
	"};"
	""
	"struct VSInput"
	"{"
	"	float3 position : POSITION;"
	"};"
	""
	"struct VSOutput"
	"{"
	"	float4 position : SV_POSITION;"
	"};"
	""
	"VSOutput VSMain(VSInput input)"
	"{"
	"	VSOutput output = (VSOutput)0;"
	"	output.position = mul(input.position, mWorldViewProj);"
	"	output.position.w = 1.0;"
	"	return output;"
	"}"
	""
	"float4 PSMain(VSOutput input) : SV_TARGET"
	"{"
	"	return float4(1.0, 0.0, 1.0, 1.0);"
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

		const std::array<D3D11_INPUT_ELEMENT_DESC, 1> inputLayoutDesc{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		Expects(SUCCEEDED(mDevice->CreateInputLayout(inputLayoutDesc.data(),
													 inputLayoutDesc.size(),
													 vertexShaderBlob->GetBufferPointer(),
													 vertexShaderBlob->GetBufferSize(),
													 &mInputLayout)));
	}

	// create vertex buffer
	{
		const std::array<float, 3 * 3> vertices{
			0.0f,  0.0f, 0.5f,  // point at top
			0.5f,  0.0f, -0.5f, // point at bottom-right
			-0.5f, 0.0f, -0.5f, // point at bottom-left
		};

		D3D11_BUFFER_DESC desc = { 0 };
		desc.ByteWidth = sizeof(vertices);
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		D3D11_SUBRESOURCE_DATA data = { 0 };
		data.pSysMem = vertices.data();
		Expects(SUCCEEDED(mDevice->CreateBuffer(&desc, &data, &mVertexBuffer)));
	}

	// create constant buffer
	{
		const XMVECTOR rotation{ XMQuaternionIdentity() };
		const XMVECTOR position{ XMVectorSet(0.5f, -5.0f, 0.0f, 0.0f) };
		constexpr float FocalDistance{ 0.0f };

		const XMMATRIX proj{ XMMatrixPerspectiveFovRH(1.24f, 16.0f / 9.0f, 0.01f, 1000.0f) };

		const XMVECTOR up{ XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation) };
		const XMVECTOR lookAt{ XMVector3Rotate(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotation) +
							   position };
		const XMVECTOR eyePos{ XMVectorScale(
								   XMVector3Rotate(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotation),
								   FocalDistance) +
							   position };
		const XMMATRIX view{ XMMatrixLookAtRH(eyePos, lookAt, up) };

		const XMMATRIX worldViewProj{ (
			XMMatrixMultiply(XMMatrixMultiply(XMMatrixIdentity(), view), proj)) };

		VSConstantBuffer initData;
		XMStoreFloat4x4(&initData.mWorldViewProj, worldViewProj);

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
	mVertexShader.Reset();
	mPixelShader.Reset();
	mInputLayout.Reset();
	mBackBuffer.Reset();
	mSwapChain.Reset();
	mDeviceContext.Reset();
	mDevice.Reset();

	mHasDeviceResources = false;
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
			}
		}

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
		const UINT vertexStride = 3 * sizeof(float);
		const UINT vertexOffset = 0;
		mDeviceContext->IASetVertexBuffers(0,
										   1,
										   mVertexBuffer.GetAddressOf(),
										   &vertexStride,
										   &vertexOffset);

		mDeviceContext->VSSetConstantBuffers(0, 1, mConstantBuffer.GetAddressOf());
		mDeviceContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
		mDeviceContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

		mDeviceContext->Draw(3, 0);

		Expects(SUCCEEDED(mSwapChain->Present(1, 0)));

		const time_point currFrame = steady_clock::now();
		frameTime = duration_cast<duration<float>>(currFrame - prevFrame).count();
		prevFrame = currFrame;
	}
}
