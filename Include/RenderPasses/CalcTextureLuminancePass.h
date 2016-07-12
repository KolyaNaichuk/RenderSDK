#pragma once

#include "Common/Common.h"

class D3DDevice;
class D3DPipelineState;
class D3DRootSignature;
class D3DCommandList;
class D3DCommandAllocator;
class D3DResource;
class D3DDescriptorHeap;

class CalcTextureLuminancePass
{
public:
	CalcTextureLuminancePass(D3DDevice* pDevice, DXGI_FORMAT rtvFormat, bool logLuminance = false);
	~CalcTextureLuminancePass();

	void Record(D3DCommandList* pCommandList, D3DCommandAllocator* pCommandAllocator,
		D3DResource* pRTVTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor,
		D3DDescriptorHeap* pSRVDescriptorHeap, D3DResource* pSRVTexture, D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor,
		D3DDescriptorHeap* pSamplerDescriptorHeap, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor,
		const D3D12_RESOURCE_STATES* pRTVEndState = nullptr, const D3D12_RESOURCE_STATES* pSRVEndState = nullptr);

private:
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;
};