#pragma once

#include "Common/Common.h"

class DXDevice;
class DXCommandList;
class DXCommandAllocator;
class DXRootSignature;
class DXPipelineState;
class DXResource;
class DXDescriptorHeap;

struct VisualizeVoxelGridInitParams
{
	DXDevice* m_pDevice;
	DXGI_FORMAT m_RTVFormat;
};

struct VisualizeVoxelGridRecordParams
{
	DXCommandList* m_pCommandList;
	DXCommandAllocator* m_pCommandAllocator;
	DXResource* m_pRenderTarget;
	D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
	const D3D12_RESOURCE_STATES* m_pRenderTargetEndState;
	DXResource* m_pDepthTexture;
	D3D12_GPU_DESCRIPTOR_HANDLE m_DepthSRVHandle;
	DXResource* m_pGridBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GridBufferSRVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GridConfigCBVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_TransformCBVHandle;
	DXDescriptorHeap* m_pCBVSRVUAVDescriptorHeap;
};

class VisualizeVoxelGridRecorder
{
public:
	VisualizeVoxelGridRecorder(VisualizeVoxelGridInitParams* pParams);
	~VisualizeVoxelGridRecorder();

	void Record(VisualizeVoxelGridRecordParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};