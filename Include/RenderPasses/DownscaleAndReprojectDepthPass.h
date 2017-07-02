#pragma once

#include "D3DWrapper/Common.h"

struct RenderEnv;
struct BindingResourceList;

class CommandList;
class RootSignature;
class PipelineState;

class DownscaleAndReprojectDepthPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		u16 m_ReprojectedDepthTextureWidth;
		u16 m_ReprojectedDepthTextureHeight;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		BindingResourceList* m_pResources;
	};

	DownscaleAndReprojectDepthPass(InitParams* pParams);
	~DownscaleAndReprojectDepthPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
};