#pragma once

#include "Common/Common.h"

class DXDevice;
class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class MeshBatch;

struct DXViewport;
struct DXRenderEnvironment;
struct DXBindingResourceList;

//#define HAS_TEXCOORD

class CreateVoxelGridRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pEnv;
		MeshBatch* m_pMeshBatch;
	};

	struct RenderPassParams
	{
		DXRenderEnvironment* m_pEnv;
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
		DXBindingResourceList* m_pResources;
		DXViewport* m_pViewport;
		MeshBatch* m_pMeshBatch;
	};

	CreateVoxelGridRecorder(InitParams* pParams);
	~CreateVoxelGridRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};