#pragma once

#include "Common/Common.h"

class D3DCommandList;
class D3DCommandAllocator;
class D3DRootSignature;
class D3DPipelineState;
struct D3DRenderEnv;
struct D3DResourceList;

class ClearVoxelGridPass
{
public:
	struct InitParams
	{
		D3DRenderEnv* m_pRenderEnv;
		u16 m_NumGridCellsX;
		u16 m_NumGridCellsY;
		u16 m_NumGridCellsZ;
	};

	struct RenderParams
	{
		D3DRenderEnv* m_pRenderEnv;
		D3DCommandList* m_pCommandList;
		D3DCommandAllocator* m_pCommandAllocator;
		D3DResourceList* m_pResources;
	};

	ClearVoxelGridPass(InitParams* pParams);
	~ClearVoxelGridPass();

	void Record(RenderParams* pParams);

private:
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
	u16 m_NumThreadGroupsZ;
};
