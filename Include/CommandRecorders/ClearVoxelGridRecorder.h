#pragma once

#include "Common/Common.h"

class DXCommandList;
class DXCommandAllocator;
class DXRootSignature;
class DXPipelineState;
struct DXRenderEnvironment;
struct DXBindingResourceList;

class ClearVoxelGridRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pRenderEnv;
		u16 m_NumGridCellsX;
		u16 m_NumGridCellsY;
		u16 m_NumGridCellsZ;
	};

	struct RenderPassParams
	{
		DXRenderEnvironment* m_pRenderEnv;
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
		DXBindingResourceList* m_pResources;
	};

	ClearVoxelGridRecorder(InitParams* pParams);
	~ClearVoxelGridRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
	u16 m_NumThreadGroupsZ;
};
