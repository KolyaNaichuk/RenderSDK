#pragma once

#include "Common/Common.h"

class D3DCommandList;
class D3DCommandAllocator;
class D3DRootSignature;
class D3DPipelineState;

struct D3DRenderEnv;
struct D3DResourceList;
struct D3DViewport;

class VisualizeVoxelGridPass
{
public:
	struct InitParams
	{
		D3DRenderEnv* m_pRenderEnv;
		DXGI_FORMAT m_RTVFormat;
	};
	
	struct RenderParams
	{
		D3DRenderEnv* m_pRenderEnv;
		D3DCommandList* m_pCommandList;
		D3DCommandAllocator* m_pCommandAllocator;
		D3DResourceList* m_pResources;
		D3DViewport* m_pViewport;
	};

	VisualizeVoxelGridPass(InitParams* pParams);
	~VisualizeVoxelGridPass();

	void Record(RenderParams* pParams);

private:
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;
};