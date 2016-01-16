#pragma once

#include "Common/Common.h"

class DXDevice;
class DXCommandList;
class DXCommandAllocator;
class DXResource;
class DXPipelineState;
class DXRootSignature;
class DXDescriptorHeap;

class CopyTextureRecorder
{
public:
	CopyTextureRecorder(DXDevice* pDevice, DXGI_FORMAT rtvFormat);
	~CopyTextureRecorder();

	void Record(DXCommandList* pCommandList, DXCommandAllocator* pCommandAllocator,
		DXResource* pRTVTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor,
		DXDescriptorHeap* pSRVDescriptorHeap, DXResource* pSRVTexture, D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor,
		DXDescriptorHeap* pSamplerDescriptorHeap, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor,
		const D3D12_RESOURCE_STATES* pRTVEndState = nullptr,
		const D3D12_RESOURCE_STATES* pSRVEndState = nullptr);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};
