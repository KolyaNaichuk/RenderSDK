#pragma once

#include "DXObject.h"

class D3DFactory;
class D3DCommandQueue;
class D3DColorTexture;

struct D3DRenderEnv;

struct D3DSwapChainDesc : public DXGI_SWAP_CHAIN_DESC
{
	D3DSwapChainDesc(UINT bufferCount, HWND hOutputWindow, UINT width, UINT height,
		BOOL windowedMode = TRUE, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,
		UINT sampleCount = 1, UINT sampleQuality = 0,
		DXGI_USAGE bufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT);
};

class D3DSwapChain : public DXObject<IDXGISwapChain3>
{
public:
	D3DSwapChain(D3DFactory* pFactory, D3DRenderEnv* pRenderEnv, D3DSwapChainDesc* pDesc, D3DCommandQueue* pCommandQueue);
	~D3DSwapChain();

	D3DColorTexture* GetBackBuffer(UINT index);
	UINT GetCurrentBackBufferIndex();

	void Present(UINT syncInterval = 1, UINT flags = 0);

private:
	D3DColorTexture** m_ppFirstBuffer;
	UINT m_BufferCount;
};
