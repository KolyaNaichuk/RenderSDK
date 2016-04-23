#pragma once

#include "DXObject.h"

class DXResource;
class DXFactory;

class DXDevice : public DXObject<ID3D12Device>
{
public:
	DXDevice(DXFactory* pFactory, D3D_FEATURE_LEVEL minFeatureLevel, bool useWarpAdapter = false);
	
	// Kolya: should be removed. Deprecated
	void CreateRenderTargetView(DXResource* pResource,
		const D3D12_RENDER_TARGET_VIEW_DESC* pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
	
	// Kolya: should be removed. Deprecated
	void CreateDepthStencilView(DXResource* pResource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);

	void CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle);
	
	// Kolya: should be removed. Deprecated
	void CreateShaderResourceView(DXResource* pResource,
		const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle);
	
	// Kolya: should be removed. Deprecated
	void CreateUnorderedAccessView(DXResource* pResource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE uavHandle,
		DXResource* pCounterResource = nullptr);

	// Kolya: should be removed. Deprecated
	void CreateSampler(const D3D12_SAMPLER_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle);

	void CheckFeatureSupport(D3D12_FEATURE feature, void* pFeatureSupportData, UINT featureSupportDataSize);
};
