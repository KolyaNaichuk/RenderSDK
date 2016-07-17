#pragma once

#include "D3DWrapper/Common.h"

class CommandList;
class PipelineState;
class RootSignature;

struct BindingResourceList;
struct RenderEnv;
struct Viewport;

class CopyTexturePass
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

	CopyTexturePass(InitParams* pParams);
	~CopyTexturePass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
};
