#pragma once

#include "Common/Common.h"

class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class DXBuffer;
class MeshBatch;
struct DXRenderEnvironment;
struct DXBindingResourceList;

class ViewFrustumCullingRecorder
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
		DXBuffer* m_pNumDrawsBuffer;
	};
	
	ViewFrustumCullingRecorder(InitParams* pParams);
	~ViewFrustumCullingRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
};