#pragma once

#include "D3DWrapper/Common.h"

struct RenderEnv;
class GraphicsDevice;
class CommandList;
class RootSignature;
class PipelineState;
class GraphicsResource;

class PropagateLightPass
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
		u16 m_NumIterations;
	};

	PropagateLightPass(InitParams* pParams);
	~PropagateLightPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pWithoutOcclusionTestState;
	PipelineState* m_pWithOcclusionTestState;
	
	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
	u16 m_NumThreadGroupsZ;
};