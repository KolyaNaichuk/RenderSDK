#pragma once

#include "Common/Common.h"

class DXDevice;
class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class DXResource;
class Mesh;

//#define HAS_TEXCOORD

struct CreateVoxelGridInitParams
{
	DXDevice* m_pDevice;
	u8 m_VertexElementFlags;
};

struct CreateVoxelGridRecordParams
{
	DXCommandList* m_pCommandList;
	DXCommandAllocator* m_pCommandAllocator;
	UINT m_NumDXDescriptorHeaps;
	ID3D12DescriptorHeap* m_pDXFirstDescriptorHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE m_ObjectTransformCBVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_CameraTransformCBVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GridConfigCBVHandle;
	DXResource* m_pGridBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GridBufferUAVHandle;
	Mesh* m_pMesh;

#ifdef HAS_TEXCOORD
	DXResource* m_pColorTexture;
	D3D12_GPU_DESCRIPTOR_HANDLE m_ColorSRVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_LinearSamplerHandle;
#endif // HAS_TEXCOORD
};

class CreateVoxelGridRecorder
{
public:
	CreateVoxelGridRecorder(CreateVoxelGridInitParams* pParams);
	~CreateVoxelGridRecorder();

	void Record(CreateVoxelGridRecordParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};