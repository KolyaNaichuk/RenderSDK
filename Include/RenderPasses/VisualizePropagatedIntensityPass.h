#pragma once

#include "D3DWrapper/Common.h"

class RootSignature;
class PipelineState;
class CommandList;
class ColorTexture;

struct RenderEnv;
struct BindingResourceList;

class VisualizePropagatedIntensityPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
	};
	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		BindingResourceList* m_pResources;
	};

	VisualizePropagatedIntensityPass(InitParams* pParams);
	~VisualizePropagatedIntensityPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
};