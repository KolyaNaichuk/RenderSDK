#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;
class CommandList;
class RootSignature;
class PipelineState;
class GraphicsResource;

struct RenderEnv;
struct BindingResourceList;

class PropagateLightPass
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
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		BindingResourceList** m_ppResources;
		u16 m_NumIterations;
	};

	PropagateLightPass(InitPrams* pParams);
	~PropagateLightPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
	u16 m_NumThreadGroupsZ;
};