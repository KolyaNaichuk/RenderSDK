#pragma once

#include "D3DWrapper/Common.h"

class RootSignature;
class PipelineState;
class CommandList;
class CommandAllocator;
struct RenderEnv;
struct BindingResourceList;

class RenderGBufferCommandsPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		u32 m_NumMeshesInBatch;
	};
	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		CommandAllocator* m_pCommandAllocator;
		BindingResourceList* m_pResources;
	};

	RenderGBufferCommandsPass(InitParams* pParams);
	~RenderGBufferCommandsPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
};