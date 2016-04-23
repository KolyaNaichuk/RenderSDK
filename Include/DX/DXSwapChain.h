#pragma once

#include "DXObject.h"

class DXFactory;
class DXCommandQueue;
class DXRenderTarget;

struct DXRenderEnvironment;

struct DXSwapChainDesc : public DXGI_SWAP_CHAIN_DESC
{
	DXSwapChainDesc(UINT bufferCount, HWND hOutputWindow, UINT width, UINT height,
		BOOL windowedMode = TRUE, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,
		UINT sampleCount = 1, UINT sampleQuality = 0,
		DXGI_USAGE bufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT);
};

class DXSwapChain : public DXObject<IDXGISwapChain3>
{
public:
	DXSwapChain(DXFactory* pFactory, DXRenderEnvironment* pEnv, DXSwapChainDesc* pDesc, DXCommandQueue* pCommandQueue);
	~DXSwapChain();

	DXRenderTarget* GetBackBuffer(UINT index);
	UINT GetCurrentBackBufferIndex();

	void Present(UINT syncInterval = 1, UINT flags = 0);

private:
	DXRenderTarget** m_ppFirstBuffer;
	UINT m_BufferCount;
};
