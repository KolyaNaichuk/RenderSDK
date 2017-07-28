#pragma once

#include "D3DWrapper/Common.h"

struct RenderEnv;
struct ResourceList;
struct Viewport;

class CommandList;
class RootSignature;
class PipelineState;

class CopyReprojectedDepthPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		DXGI_FORMAT m_DSVFormat;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		ResourceList* m_pResources;
		Viewport* m_pViewport;
	};

	CopyReprojectedDepthPass(InitParams* pParams);
	~CopyReprojectedDepthPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
};
