#pragma once

#include "Common/Common.h"

class D3DCommandList;
class D3DCommandAllocator;
class D3DPipelineState;
class D3DRootSignature;

struct D3DResourceList;
struct D3DRenderEnv;
struct D3DViewport;

class CopyTextureRecorder
{
public:
	struct InitParams
	{
		D3DRenderEnv* m_pRenderEnv;
		DXGI_FORMAT m_RTVFormat;
	};

	struct RenderPassParams
	{
		D3DRenderEnv* m_pRenderEnv;
		D3DCommandList* m_pCommandList;
		D3DCommandAllocator* m_pCommandAllocator;
		D3DResourceList* m_pResources;
		D3DViewport* m_pViewport;
	};

	CopyTextureRecorder(InitParams* pParams);
	~CopyTextureRecorder();

	void Record(RenderPassParams* pParams);

private:
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;
};
