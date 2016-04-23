#pragma once

#include "Common/Common.h"

class DXRootSignature;
class DXPipelineState;
struct DXRenderEnvironment;

class ViewFrustumCullingRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pEnv;
		u16 m_NumMeshes;
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