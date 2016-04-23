#pragma once

#include "Common/Common.h"

class DXDevice;
class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class DXBuffer;
class Mesh;

struct DXRenderEnvironment;

//#define HAS_TEXCOORD

struct CreateVoxelGridInitParams
{
	DXRenderEnvironment* m_pEnv;
	u8 m_VertexElementFlags;
};

struct CreateVoxelGridRecordParams
{
	DXRenderEnvironment* m_pEnv;
	DXCommandList* m_pCommandList;
	DXCommandAllocator* m_pCommandAllocator;
	DXBuffer* m_pObjectTransformBuffer;
	DXBuffer* m_pCameraTransformBuffer;
	DXBuffer* m_pGridConfigBuffer;
	DXBuffer* m_pGridBuffer;
		
#ifdef HAS_TEXCOORD
	DXResource* m_pColorTexture;
	D3D12_GPU_DESCRIPTOR_HANDLE m_ColorSRVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_LinearSamplerHandle;
#endif // HAS_TEXCOORD

	Mesh* m_pMesh;
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