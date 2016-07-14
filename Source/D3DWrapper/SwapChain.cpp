#include "D3DWrapper/GraphicsFactory.h"
#include "D3DWrapper/SwapChain.h"
#include "D3DWrapper/CommandQueue.h"
#include "D3DWrapper/GraphicsResource.h"

SwapChainDesc::SwapChainDesc(UINT bufferCount, HWND hOutputWindow, UINT width, UINT height, BOOL windowedMode,
	DXGI_FORMAT format, UINT sampleCount, UINT sampleQuality, DXGI_USAGE bufferUsage)
{
	BufferDesc.Width = width;
	BufferDesc.Height = height;
	BufferDesc.RefreshRate.Numerator = 0;
	BufferDesc.RefreshRate.Denominator = 0;
	BufferDesc.Format = format;
	BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	SampleDesc.Count = sampleCount;
	SampleDesc.Quality = sampleQuality;
	BufferUsage = bufferUsage;
	BufferCount = bufferCount;
	OutputWindow = hOutputWindow;
	Windowed = windowedMode;
	SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
}

SwapChain::SwapChain(GraphicsFactory* pFactory, RenderEnv* pRenderEnv, SwapChainDesc* pDesc, CommandQueue* pCommandQueue)
	: m_ppFirstBuffer(nullptr)
	, m_BufferCount(pDesc->BufferCount)
{
	ComPtr<IDXGISwapChain> dxgiSwapChain;
	VerifyD3DResult(pFactory->GetDXGIObject()->CreateSwapChain(pCommandQueue->GetD3DObject(), pDesc, &dxgiSwapChain));
	VerifyD3DResult(dxgiSwapChain.As(&m_DXGISwapChain));
	
	m_ppFirstBuffer = new ColorTexture* [m_BufferCount];
	for (UINT index = 0; index < m_BufferCount; ++index)
	{
		ComPtr<ID3D12Resource> d3dBuffer;
		VerifyD3DResult(m_DXGISwapChain->GetBuffer(index, IID_PPV_ARGS(&d3dBuffer)));
		
		ColorTexture* pBuffer = new ColorTexture(pRenderEnv, d3dBuffer, D3D12_RESOURCE_STATE_PRESENT, L"BackBuffer");
		m_ppFirstBuffer[index] = pBuffer;
	}
}

SwapChain::~SwapChain()
{
	for (UINT index = 0; index < m_BufferCount; ++index)
		SafeDelete(m_ppFirstBuffer[index]);
	SafeArrayDelete(m_ppFirstBuffer);
}

ColorTexture* SwapChain::GetBackBuffer(UINT index)
{
	assert(index < m_BufferCount);
	return m_ppFirstBuffer[index];
}

UINT SwapChain::GetCurrentBackBufferIndex()
{
	return m_DXGISwapChain->GetCurrentBackBufferIndex();
}

void SwapChain::Present(UINT syncInterval, UINT flags)
{
	VerifyD3DResult(m_DXGISwapChain->Present(syncInterval, flags));
}
