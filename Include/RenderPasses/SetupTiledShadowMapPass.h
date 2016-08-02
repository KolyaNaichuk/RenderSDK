#pragma once

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
		u16 m_NumTilesX;
		u16 m_NumTilesY;
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
};
