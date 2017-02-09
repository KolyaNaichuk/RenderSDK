#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;
class CommandList;
class RootSignature;
class PipelineState;
class GraphicsResource;

struct Viewport;
struct RenderEnv;
struct BindingResourceList;

class CalcIndirectLightPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		DXGI_FORMAT m_RTVFormat;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		BindingResourceList* m_pResources;
		Viewport* m_pViewport;
	};

	CalcIndirectLightPass(InitParams* pParams);
	~CalcIndirectLightPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
};