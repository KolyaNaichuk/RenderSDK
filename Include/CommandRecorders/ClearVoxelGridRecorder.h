#pragma once

#include "Common/Common.h"

class DXDevice;
class DXCommandList;
class DXCommandAllocator;
class DXRootSignature;
class DXPipelineState;
class DXResource;

struct ClearVoxelGridInitParams
{
	DXDevice* m_pDevice;
	UINT m_NumGridCellsX;
	UINT m_NumGridCellsY;
	UINT m_NumGridCellsZ;
};

struct ClearVoxelGridRecordParams
{
	DXCommandList* m_pCommandList;
	DXCommandAllocator* m_pCommandAllocator;
	UINT m_NumDXDescriptorHeaps;
	ID3D12DescriptorHeap* m_pDXFirstDescriptorHeap;
	DXResource* m_pGridBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GridBufferUAVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GridConfigCBVHandle;
};

class ClearVoxelGridRecorder
{
public:
	ClearVoxelGridRecorder(ClearVoxelGridInitParams* pParams);
	~ClearVoxelGridRecorder();

	void Record(ClearVoxelGridRecordParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
	u16 m_NumThreadGroupsZ;
};
