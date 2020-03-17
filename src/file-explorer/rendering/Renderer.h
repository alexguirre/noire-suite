#pragma once
#include "Camera.h"
#include <Windows.h>
#include <atomic>
#include <core/files/trunk/VertexDeclaration.h>
#include <d3d11.h>
#include <dxgi.h>
#include <functional>
#include <thread>
#include <winrt/base.h>

namespace noire::explorer
{
	class Renderer final
	{
	public:
		using RenderCallback = std::function<void(Renderer&, float frameTime)>;

		Renderer(HWND hwnd, RenderCallback renderCB);
		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) = default;
		~Renderer();

		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) = delete;

		void Clear(float r, float g, float b);

	private:
		void CreateDeviceResources();
		void ReleaseDeviceResources();
		void RenderThreadStart();
		void WorldMtx(const Matrix& world);
		void WorldDefault();

		HWND mHWND;
		bool mHasDeviceResources;
		winrt::com_ptr<IDXGISwapChain> mSwapChain;
		winrt::com_ptr<ID3D11Device> mDevice;
		winrt::com_ptr<ID3D11DeviceContext> mDeviceContext;
		winrt::com_ptr<ID3D11RenderTargetView> mBackBuffer;
		winrt::com_ptr<ID3D11VertexShader> mVertexShader;
		winrt::com_ptr<ID3D11PixelShader> mPixelShader;
		winrt::com_ptr<ID3D11Buffer> mVertexBuffer;
		winrt::com_ptr<ID3D11Buffer> mIndexBuffer;
		winrt::com_ptr<ID3D11Buffer> mConstantBuffer;
		trunk::VertexDeclarationManager mVertDecls;
		std::thread mRenderingThread;
		std::atomic_bool mRenderingThreadRunning;
		RenderCallback mRenderCallback;
		Camera mCam;
	};
}
