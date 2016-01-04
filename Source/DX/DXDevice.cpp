#include "DX/DXDevice.h"
#include "DX/DXFactory.h"
#include "DX/DXCommandQueue.h"
#include "DX/DXCommandAllocator.h"
#include "DX/DXCommandList.h"
#include "DX/DXFence.h"
#include "DX/DXResource.h"

void GetHardwareAdapter(DXFactory* pFactory, D3D_FEATURE_LEVEL minFeatureLevel, IDXGIAdapter1** ppAdapter)
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

DXDevice::DXDevice(DXFactory* pFactory, D3D_FEATURE_LEVEL minFeatureLevel, bool useWarpAdapter)
{
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
}

void DXDevice::CreateRenderTargetView(DXResource* pResource, const D3D12_RENDER_TARGET_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
	GetDXObject()->CreateRenderTargetView(pResource->GetDXObject(), pDesc, rtvHandle);
}

void DXDevice::CreateDepthStencilView(DXResource* pResource, const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
	GetDXObject()->CreateDepthStencilView(pResource->GetDXObject(), pDesc, dsvHandle);
}

void DXDevice::CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle)
{
	GetDXObject()->CreateConstantBufferView(pDesc, cbvHandle);
}

void DXDevice::CreateShaderResourceView(DXResource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle)
{
	GetDXObject()->CreateShaderResourceView(pResource->GetDXObject(), pDesc, srvHandle);
}

void DXDevice::CreateUnorderedAccessView(DXResource* pResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE uavHandle, DXResource* pCounterResource)
{
	GetDXObject()->CreateUnorderedAccessView(pResource->GetDXObject(), (pCounterResource != nullptr) ? pCounterResource->GetDXObject() : nullptr, pDesc, uavHandle);
}

void DXDevice::CreateSampler(const D3D12_SAMPLER_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle)
{
	GetDXObject()->CreateSampler(pDesc, samplerHandle);
}
