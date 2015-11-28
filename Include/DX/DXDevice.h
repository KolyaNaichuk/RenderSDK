#pragma once

#include "DXObject.h"

class DXResource;
class DXFactory;

class DXDevice : public DXObject<ID3D12Device>
{
public:
	DXDevice(DXFactory* pFactory, D3D_FEATURE_LEVEL minFeatureLevel, bool useWarpAdapter = false);
			
	void CreateRenderTargetView(DXResource* pResource,
		const D3D12_RENDER_TARGET_VIEW_DESC* pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
	
	void CreateDepthStencilView(DXResource* pResource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);
	
	void CreateShaderResourceView(DXResource* pResource,
		const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle);
	
	void CreateUnorderedAccessView(DXResource* pResource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE uavHandle,
		DXResource* pCounterResource = nullptr);

	void CreateSampler(const D3D12_SAMPLER_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle);
};
