#include "D3DWrapper/D3DDevice.h"
#include "D3DWrapper/D3DFactory.h"
#include "D3DWrapper/D3DCommandQueue.h"
#include "D3DWrapper/D3DCommandAllocator.h"
#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DFence.h"
#include "D3DWrapper/D3DResource.h"

void GetHardwareAdapter(D3DFactory* pFactory, D3D_FEATURE_LEVEL minFeatureLevel, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->GetDXObject()->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
			break;
	}

	*ppAdapter = adapter.Detach();
}

D3DDevice::D3DDevice(D3DFactory* pFactory, D3D_FEATURE_LEVEL minFeatureLevel, bool useWarpAdapter)
{
#ifdef _DEBUG
	ComPtr<ID3D12Debug> d3dDebug;
	DXVerify(D3D12GetDebugInterface(IID_PPV_ARGS(&d3dDebug)));
	d3dDebug->EnableDebugLayer();

	ComPtr<IDXGIDebug1> dxgiDebug;
	DXVerify(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)));
	DXVerify(dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL));
#endif

	if (useWarpAdapter)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		DXVerify(pFactory->GetDXObject()->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
		
		DXVerify(D3D12CreateDevice(warpAdapter.Get(), minFeatureLevel, IID_PPV_ARGS(GetDXObjectAddress())));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(pFactory, minFeatureLevel, &hardwareAdapter);

		DXVerify(D3D12CreateDevice(hardwareAdapter.Get(), minFeatureLevel, IID_PPV_ARGS(GetDXObjectAddress())));
	}

#ifdef _DEBUG
	ComPtr<ID3D12InfoQueue> d3dInfoQueue;
	DXVerify(GetDXObject()->QueryInterface(IID_PPV_ARGS(&d3dInfoQueue)));
	DXVerify(d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE));
	DXVerify(d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
#endif
}

void D3DDevice::CheckFeatureSupport(D3D12_FEATURE feature, void* pFeatureSupportData, UINT featureSupportDataSize)
{
	DXVerify(GetDXObject()->CheckFeatureSupport(feature, pFeatureSupportData, featureSupportDataSize));
}

void D3DDevice::CopyDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType)
{
	GetDXObject()->CopyDescriptorsSimple(1, destDescriptor, srcDescriptor, descriptorHeapType);
}
