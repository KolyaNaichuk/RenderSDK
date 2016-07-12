#pragma once

#include "Common/Common.h"

class D3DRootSignature;
class D3DPipelineState;
class D3DCommandList;
class D3DCommandAllocator;
class D3DBuffer;
struct D3DRenderEnv;
struct D3DResourceList;

enum ObjectBoundsType
{
	ObjectBoundsType_AABB = 1,
	ObjectBoundsType_Sphere = 2
};

class ViewFrustumCullingRecorder
{
public:
	struct InitParams
	{
		D3DRenderEnv* m_pRenderEnv;
		ObjectBoundsType m_ObjectBoundsType;
		u32 m_NumObjects;
	};
	struct RenderPassParams
	{
		D3DRenderEnv* m_pRenderEnv;
		D3DCommandList* m_pCommandList;
		D3DCommandAllocator* m_pCommandAllocator;
		D3DResourceList* m_pResources;
		D3DBuffer* m_pNumVisibleObjectsBuffer;
	};
	
	ViewFrustumCullingRecorder(InitParams* pParams);
	~ViewFrustumCullingRecorder();

	void Record(RenderPassParams* pParams);

private:
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
};