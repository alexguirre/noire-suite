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
		const std::array<float, 7 * 3 * 2> vertices{
			0.0f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // point at top
			0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // point at bottom-right
			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, // point at bottom-left
			0.0f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // point at top
			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, // point at bottom-left
			0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // point at bottom-right
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
		const UINT vertexStride = (3 + 4) * sizeof(float);
		const UINT vertexOffset = 0;
		mDeviceContext->IASetVertexBuffers(0,
										   1,
										   mVertexBuffer.GetAddressOf(),
										   &vertexStride,
										   &vertexOffset);

		mDeviceContext->VSSetConstantBuffers(0, 1, mConstantBuffer.GetAddressOf());
		mDeviceContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
		mDeviceContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

		static Matrix objMatrix{ Matrix::Identity };
		objMatrix *= Matrix::CreateFromAxisAngle(Vector3::Up, frameTime);

		WorldMtx(objMatrix);
		mDeviceContext->Draw(6, 0);

		for (float i = 1.0f; i < 100.0f; i += 1.0f)
		{
			WorldMtx(objMatrix * Matrix::CreateTranslation(0.0f, i, 0.0f));
			mDeviceContext->Draw(6, 0);

			WorldMtx(objMatrix * Matrix::CreateTranslation(0.0f, -i, 0.0f));
			mDeviceContext->Draw(6, 0);
		}
		Expects(SUCCEEDED(mSwapChain->Present(1, 0)));

		const time_point currFrame = steady_clock::now();
		frameTime = duration_cast<duration<float>>(currFrame - prevFrame).count();
		prevFrame = currFrame;
	}
}
