#include "D3DWrapper/D3DFactory.h"
#include "D3DWrapper/D3DSwapChain.h"
#include "D3DWrapper/D3DCommandQueue.h"
#include "D3DWrapper/D3DResource.h"

D3DSwapChainDesc::D3DSwapChainDesc(UINT bufferCount, HWND hOutputWindow, UINT width, UINT height, BOOL windowedMode,
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

D3DSwapChain::D3DSwapChain(D3DFactory* pFactory, D3DRenderEnv* pRenderEnv, D3DSwapChainDesc* pDesc, D3DCommandQueue* pCommandQueue)
	: m_ppFirstBuffer(nullptr)
	, m_BufferCount(pDesc->BufferCount)
{
	IDXGISwapChain* pDXObject = nullptr;
	DXVerify(pFactory->GetDXObject()->CreateSwapChain(pCommandQueue->GetDXObject(), pDesc, &pDXObject));
	DXVerify(pDXObject->QueryInterface(IID_PPV_ARGS(GetDXObjectAddress())));
	SafeRelease(pDXObject);

	m_ppFirstBuffer = new D3DColorTexture* [m_BufferCount];
	for (UINT index = 0; index < m_BufferCount; ++index)
	{
		ID3D12Resource* pDXBuffer = nullptr;
		DXVerify(GetDXObject()->GetBuffer(index, IID_PPV_ARGS(&pDXBuffer)));
		
		D3DColorTexture* pBuffer = new D3DColorTexture(pRenderEnv, pDXBuffer, D3D12_RESOURCE_STATE_PRESENT, L"BackBuffer");
		m_ppFirstBuffer[index] = pBuffer;
	}
}

D3DSwapChain::~D3DSwapChain()
{
	for (UINT index = 0; index < m_BufferCount; ++index)
		SafeDelete(m_ppFirstBuffer[index]);
	SafeArrayDelete(m_ppFirstBuffer);
}

D3DColorTexture* D3DSwapChain::GetBackBuffer(UINT index)
{
	assert(index < m_BufferCount);
	return m_ppFirstBuffer[index];
}

UINT D3DSwapChain::GetCurrentBackBufferIndex()
{
	return GetDXObject()->GetCurrentBackBufferIndex();
}

void D3DSwapChain::Present(UINT syncInterval, UINT flags)
{
	DXVerify(GetDXObject()->Present(syncInterval, flags));
}
