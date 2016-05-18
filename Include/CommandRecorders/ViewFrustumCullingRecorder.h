#pragma once

#include "Common/Common.h"

class DXRootSignature;
class DXPipelineState;
class MeshBatch;
struct DXRenderEnvironment;

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
	};
	
	ViewFrustumCullingRecorder(InitParams* pParams);
	~ViewFrustumCullingRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
};