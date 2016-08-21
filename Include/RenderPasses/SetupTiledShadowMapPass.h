#pragma once

#include "Common/Light.h"
#include "D3DWrapper/Common.h"

class RootSignature;
class PipelineState;
class CommandList;
struct RenderEnv;
struct BindingResourceList;

class SetupTiledShadowMapPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		LightType m_LightType;
		u32 m_MaxNumLights;
	};
	
	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		BindingResourceList* m_pResources;
	};

	SetupTiledShadowMapPass(InitParams* pParams);
	~SetupTiledShadowMapPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
};
