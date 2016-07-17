#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;
class CommandList;
class RootSignature;
class PipelineState;
class GraphicsResource;

struct RenderEnv;

class InjectVPLsIntoVoxelGridPass
{
public:
	struct InitPrams
	{
		RenderEnv* m_pRenderEnv;
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
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
	u16 m_NumThreadGroupsZ;
};