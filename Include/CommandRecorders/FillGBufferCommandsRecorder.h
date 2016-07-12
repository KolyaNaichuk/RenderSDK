#pragma once

#include "Common/Common.h"

class D3DRootSignature;
class D3DPipelineState;
class D3DCommandList;
class D3DCommandAllocator;
struct D3DRenderEnv;
struct D3DResourceList;

class FillGBufferCommandsRecorder
{
public:
	struct InitParams
	{
		D3DRenderEnv* m_pRenderEnv;
		u32 m_NumMeshesInBatch;
	};
	struct RenderPassParams
	{
		D3DRenderEnv* m_pRenderEnv;
		D3DCommandList* m_pCommandList;
		D3DCommandAllocator* m_pCommandAllocator;
		D3DResourceList* m_pResources;
	};

	FillGBufferCommandsRecorder(InitParams* pParams);
	~FillGBufferCommandsRecorder();

	void Record(RenderPassParams* pParams);

private:
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
};