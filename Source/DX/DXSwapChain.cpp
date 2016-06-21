#include "DX/DXFactory.h"
#include "DX/DXSwapChain.h"
#include "DX/DXCommandQueue.h"
#include "DX/DXResource.h"

DXSwapChainDesc::DXSwapChainDesc(UINT bufferCount, HWND hOutputWindow, UINT width, UINT height, BOOL windowedMode,
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

DXSwapChain::DXSwapChain(DXFactory* pFactory, DXRenderEnvironment* pRenderEnv, DXSwapChainDesc* pDesc, DXCommandQueue* pCommandQueue)
	: m_ppFirstBuffer(nullptr)
	, m_BufferCount(pDesc->BufferCount)
{
	IDXGISwapChain* pDXObject = nullptr;
	DXVerify(pFactory->GetDXObject()->CreateSwapChain(pCommandQueue->GetDXObject(), pDesc, &pDXObject));
	DXVerify(pDXObject->QueryInterface(IID_PPV_ARGS(GetDXObjectAddress())));
	SafeRelease(pDXObject);

	m_ppFirstBuffer = new DXColorTexture* [m_BufferCount];
	for (UINT index = 0; index < m_BufferCount; ++index)
	{
		ID3D12Resource* pDXBuffer = nullptr;
		DXVerify(GetDXObject()->GetBuffer(index, IID_PPV_ARGS(&pDXBuffer)));
		
		DXColorTexture* pBuffer = new DXColorTexture(pRenderEnv, pDXBuffer, D3D12_RESOURCE_STATE_PRESENT, L"BackBuffer");
		m_ppFirstBuffer[index] = pBuffer;
	}
}

DXSwapChain::~DXSwapChain()
{
	for (UINT index = 0; index < m_BufferCount; ++index)
		SafeDelete(m_ppFirstBuffer[index]);
	SafeArrayDelete(m_ppFirstBuffer);
}

DXColorTexture* DXSwapChain::GetBackBuffer(UINT index)
{
	assert(index < m_BufferCount);
	return m_ppFirstBuffer[index];
}

UINT DXSwapChain::GetCurrentBackBufferIndex()
{
	return GetDXObject()->GetCurrentBackBufferIndex();
}

void DXSwapChain::Present(UINT syncInterval, UINT flags)
{
	DXVerify(GetDXObject()->Present(syncInterval, flags));
}
