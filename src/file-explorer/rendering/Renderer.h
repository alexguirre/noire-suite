#pragma once
#include "Camera.h"
#include <Windows.h>
#include <atomic>
#include <d3d11.h>
#include <dxgi.h>
#include <functional>
#include <thread>
#include <wrl.h>

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
		Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
		Microsoft::WRL::ComPtr<ID3D11Device> mDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> mDeviceContext;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mBackBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mConstantBuffer;
		std::thread mRenderingThread;
		std::atomic_bool mRenderingThreadRunning;
		RenderCallback mRenderCallback;
		Camera mCam;
	};
}
