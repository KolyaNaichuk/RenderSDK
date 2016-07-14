#pragma once

#include "D3DWrapper/Common.h"

class GraphicsFactory;
class CommandQueue;
class ColorTexture;

struct RenderEnv;

struct SwapChainDesc : public DXGI_SWAP_CHAIN_DESC
{
	SwapChainDesc(UINT bufferCount, HWND hOutputWindow, UINT width, UINT height,
		BOOL windowedMode = TRUE, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,
		UINT sampleCount = 1, UINT sampleQuality = 0,
		DXGI_USAGE bufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT);
};

class SwapChain
{
public:
	SwapChain(GraphicsFactory* pFactory, RenderEnv* pRenderEnv, SwapChainDesc* pDesc, CommandQueue* pCommandQueue);
	~SwapChain();

	IDXGISwapChain3* GetDXGISwapChain() { return m_DXGISwapChain.Get(); }

	ColorTexture* GetBackBuffer(UINT index);
	UINT GetCurrentBackBufferIndex();

	void Present(UINT syncInterval = 1, UINT flags = 0);

private:
	ComPtr<IDXGISwapChain3> m_DXGISwapChain;
	ColorTexture** m_ppFirstBuffer;
	UINT m_BufferCount;
};
