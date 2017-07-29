#pragma once

#include "D3DWrapper/Common.h"

class RootSignature;
class PipelineState;
class CommandList;
struct RenderEnv;

class CreateRenderGBufferCommandsPass
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
	};

	CreateRenderGBufferCommandsPass(InitParams* pParams);
	~CreateRenderGBufferCommandsPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
};