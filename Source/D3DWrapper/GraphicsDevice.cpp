#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsFactory.h"

#ifdef _DEBUG
#define ENABLE_GRAPHICS_DEBUGGING
#endif

void GetHardwareAdapter(GraphicsFactory* pFactory, D3D_FEATURE_LEVEL minFeatureLevel, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> dxgiAdapter;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->GetDXGIObject()->EnumAdapters1(adapterIndex, &dxgiAdapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 adapterDesc;
		dxgiAdapter->GetDesc1(&adapterDesc);

		if ((adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
			continue;

		HRESULT result = D3D12CreateDevice(dxgiAdapter.Get(), minFeatureLevel, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(result))
			break;
	}

	*ppAdapter = dxgiAdapter.Detach();
}

GraphicsDevice::GraphicsDevice(GraphicsFactory* pFactory, D3D_FEATURE_LEVEL minFeatureLevel, bool useWarpAdapter)
{
#ifdef ENABLE_GRAPHICS_DEBUGGING
	ComPtr<ID3D12Debug> d3dDebug;
	VerifyD3DResult(D3D12GetDebugInterface(IID_PPV_ARGS(&d3dDebug)));
	d3dDebug->EnableDebugLayer();

	ComPtr<IDXGIDebug1> dxgiDebug;
	VerifyD3DResult(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)));
	VerifyD3DResult(dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL));
#endif // ENABLE_GRAPHICS_DEBUGGING

	if (useWarpAdapter)
	{
		ComPtr<IDXGIAdapter> dxgiWarpAdapter;
		VerifyD3DResult(pFactory->GetDXGIObject()->EnumWarpAdapter(IID_PPV_ARGS(&dxgiWarpAdapter)));
		VerifyD3DResult(D3D12CreateDevice(dxgiWarpAdapter.Get(), minFeatureLevel, IID_PPV_ARGS(&m_D3DDevice)));
	}
	else
	{
		ComPtr<IDXGIAdapter1> dxgiHardwareAdapter;
		GetHardwareAdapter(pFactory, minFeatureLevel, &dxgiHardwareAdapter);
		VerifyD3DResult(D3D12CreateDevice(dxgiHardwareAdapter.Get(), minFeatureLevel, IID_PPV_ARGS(&m_D3DDevice)));
	}

#ifdef ENABLE_GRAPHICS_DEBUGGING
	ComPtr<ID3D12InfoQueue> d3dInfoQueue;
	VerifyD3DResult(m_D3DDevice.As(&d3dInfoQueue));
	
	VerifyD3DResult(d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE));
	VerifyD3DResult(d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
#endif // ENABLE_GRAPHICS_DEBUGGING
}

void GraphicsDevice::CheckFeatureSupport(D3D12_FEATURE feature, void* pFeatureSupportData, UINT featureSupportDataSize)
{
	VerifyD3DResult(m_D3DDevice->CheckFeatureSupport(feature, pFeatureSupportData, featureSupportDataSize));
}

void GraphicsDevice::CopyDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType)
{
	m_D3DDevice->CopyDescriptorsSimple(1, destDescriptor, srcDescriptor, descriptorHeapType);
}
