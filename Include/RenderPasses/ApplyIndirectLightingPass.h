#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;
class CommandList;
class RootSignature;
class PipelineState;
class GraphicsResource;

struct RenderEnv;

class ApplyIndirectLightingPass
{
public:
	struct InitPrams
	{
		RenderEnv* m_pRenderEnv;
	};

	struct RenderParams
	{
	};

	ApplyIndirectLightingPass(InitPrams* pParams);
	~ApplyIndirectLightingPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
};