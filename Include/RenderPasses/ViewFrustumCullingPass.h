#pragma once

#include "D3DWrapper/Common.h"

struct RenderEnv;
class RootSignature;
class PipelineState;
class CommandList;
class Buffer;

enum ObjectBoundsType
{
	ObjectBoundsType_AABB = 1,
	ObjectBoundsType_Sphere = 2
};

class ViewFrustumCullingPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ObjectBoundsType m_ObjectBoundsType;
		u32 m_NumObjects;
	};
	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pNumVisibleObjectsBuffer;
	};
	
	ViewFrustumCullingPass(InitParams* pParams);
	~ViewFrustumCullingPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
};