#pragma once

#include "Common/Common.h"

class DXDevice;
class DXCommandList;
class DXCommandAllocator;
class DXRootSignature;
class DXPipelineState;
class DXDescriptorHeap;
class DXResource;

class InjectVPLsIntoVoxelGridRecorder
{
public:
	struct InitPrams
	{
		DXDevice* m_pDevice;
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