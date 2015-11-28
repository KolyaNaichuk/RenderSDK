#pragma once

#include "Common/Common.h"

class DXDevice;
class DXPipelineState;
class DXRootSignature;
class DXCommandList;
class DXCommandAllocator;
class DXResource;

class CalcTextureLuminanceRecorder
{
public:
	CalcTextureLuminanceRecorder(DXDevice* pDevice, DXGI_FORMAT rtvFormat, bool logLuminance = false);
	~CalcTextureLuminanceRecorder();

	void Record(DXCommandList* pCommandList, DXCommandAllocator* pCommandAllocator,
		DXResource* pRTVTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor,
		ID3D12DescriptorHeap* pSRVDescriptorHeap, DXResource* pSRVTexture, D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor,
		ID3D12DescriptorHeap* pSamplerDescriptorHeap, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor,
		const D3D12_RESOURCE_STATES* pRTVEndState = nullptr, const D3D12_RESOURCE_STATES* pSRVEndState = nullptr);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};