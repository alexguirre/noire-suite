#include "Renderer.h"
#include <array>
#include <chrono>
#include <gsl/gsl>
#include <thread>

using Microsoft::WRL::ComPtr;

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
		mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
		mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &mBackBuffer);
		mDeviceContext->OMSetRenderTargets(1, mBackBuffer.GetAddressOf(), nullptr);
	}

	{
		RECT clientRect = { 0 };
		GetClientRect(mHWND, &clientRect);

		D3D11_VIEWPORT viewport = { 0 };
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = gsl::narrow_cast<FLOAT>(clientRect.right - clientRect.left);
		viewport.Height = gsl::narrow_cast<FLOAT>(clientRect.bottom - clientRect.top);

		mDeviceContext->RSSetViewports(1, &viewport);
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

	while (mRenderingThreadRunning)
	{
		mRenderCallback(*this, frameTime);

		mSwapChain->Present(1, 0);

		const time_point currFrame = steady_clock::now();
		frameTime = duration_cast<duration<float>>(currFrame - prevFrame).count();
		prevFrame = currFrame;
	}
}
