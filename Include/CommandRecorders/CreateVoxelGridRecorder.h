#pragma once

#include "Common/Common.h"

class DXDevice;
class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class DXResource;

class CreateVoxelGridRecorder
{
public:
	CreateVoxelGridRecorder(DXDevice* pDevice);
	~CreateVoxelGridRecorder();

	void Record(DXCommandList* pCommandList, DXCommandAllocator* pCommandAllocator,
		UINT numDXDescriptorHeaps, ID3D12DescriptorHeap* pDXFirstDescriptorHeap,
		D3D12_GPU_DESCRIPTOR_HANDLE objectTransformCBVDescriptor,
		D3D12_GPU_DESCRIPTOR_HANDLE cameraTransformCBVDescriptor,
		D3D12_GPU_DESCRIPTOR_HANDLE gridConfigCBVDescriptor,
		DXResource* pGridBuffer, D3D12_GPU_DESCRIPTOR_HANDLE gridBufferUAVDescriptor,
		DXResource* pColorTexture, D3D12_GPU_DESCRIPTOR_HANDLE colorSRVDescriptor,
		D3D12_GPU_DESCRIPTOR_HANDLE colorSamplerDescriptor,
		const D3D12_RESOURCE_STATES* pGridBufferEndState = nullptr,
		const D3D12_RESOURCE_STATES* pColorTextureEndState = nullptr);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};