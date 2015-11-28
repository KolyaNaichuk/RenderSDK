#pragma once

#include "Common/Common.h"

class DXDevice;
class DXCommandList;
class DXCommandAllocator;
class DXRootSignature;
class DXPipelineState;
class DXResource;

class ClearVoxelGridRecorder
{
public:
	ClearVoxelGridRecorder(DXDevice* pDevice, UINT numGridCellsX, UINT numGridCellsY, UINT numGridCellsZ);
	~ClearVoxelGridRecorder();

	void Record(DXCommandList* pCommandList, DXCommandAllocator* pCommandAllocator,
		UINT numDXDescriptorHeaps, ID3D12DescriptorHeap* pDXFirstDescriptorHeap,
		DXResource* pGridBuffer, D3D12_GPU_DESCRIPTOR_HANDLE gridUAVDescriptor,
		D3D12_GPU_DESCRIPTOR_HANDLE gridConfigCBVDescriptor,
		const D3D12_RESOURCE_STATES* pGridBufferEndState = nullptr);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;

	UINT m_NumThreadGroupsX;
	UINT m_NumThreadGroupsY;
	UINT m_NumThreadGroupsZ;
};
