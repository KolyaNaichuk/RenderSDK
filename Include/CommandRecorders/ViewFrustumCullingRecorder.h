#pragma once

#include "Common/Common.h"

class DXDevice;
class DXRootSignature;
class DXPipelineState;

class ViewFrustumCullingRecorder
{
public:
	struct InitParams
	{
		DXDevice* m_pDevice;
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