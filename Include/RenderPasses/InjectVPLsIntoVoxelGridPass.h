#pragma once

#include "Common/Common.h"

class D3DDevice;
class D3DCommandList;
class D3DCommandAllocator;
class D3DRootSignature;
class D3DPipelineState;
class D3DResource;

struct D3DRenderEnv;

class InjectVPLsIntoVoxelGridPass
{
public:
	struct InitPrams
	{
		D3DRenderEnv* m_pRenderEnv;
		u16 m_NumGridCellsX;
		u16 m_NumGridCellsY;
		u16 m_NumGridCellsZ;
	};

	struct RenderParams
	{
	};

	InjectVPLsIntoVoxelGridPass(InitPrams* pParams);
	~InjectVPLsIntoVoxelGridPass();

	void Record(RenderParams* pParams);

private:
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
	u16 m_NumThreadGroupsZ;
};