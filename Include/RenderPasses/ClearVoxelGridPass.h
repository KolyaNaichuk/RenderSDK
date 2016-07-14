#pragma once

#include "D3DWrapper/Common.h"

class CommandList;
class CommandAllocator;
class RootSignature;
class PipelineState;
struct RenderEnv;
struct BindingResourceList;

class ClearVoxelGridPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		u16 m_NumGridCellsX;
		u16 m_NumGridCellsY;
		u16 m_NumGridCellsZ;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		CommandAllocator* m_pCommandAllocator;
		BindingResourceList* m_pResources;
	};

	ClearVoxelGridPass(InitParams* pParams);
	~ClearVoxelGridPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
	u16 m_NumThreadGroupsZ;
};
