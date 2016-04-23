#pragma once

#include "Common/Common.h"

class DXDevice;
class DXCommandList;
class DXCommandAllocator;
class DXRootSignature;
class DXPipelineState;
class DXResource;

struct DXRenderEnvironment;

class InjectVPLsIntoVoxelGridRecorder
{
public:
	struct InitPrams
	{
		DXRenderEnvironment* m_pEnv;
		u16 m_NumGridCellsX;
		u16 m_NumGridCellsY;
		u16 m_NumGridCellsZ;
	};

	struct RenderPassParams
	{
	};

	InjectVPLsIntoVoxelGridRecorder(InitPrams* pParams);
	~InjectVPLsIntoVoxelGridRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
	u16 m_NumThreadGroupsZ;
};