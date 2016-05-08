#pragma once

#include "Common/Common.h"

class DXCommandList;
class DXCommandAllocator;
class DXRootSignature;
class DXPipelineState;
struct DXRenderEnvironment;
struct DXBindingResourceList;

struct ClearVoxelGridInitParams
{
	DXRenderEnvironment* m_pEnv;
	u16 m_NumGridCellsX;
	u16 m_NumGridCellsY;
	u16 m_NumGridCellsZ;
};

struct ClearVoxelGridRecordParams
{
	DXRenderEnvironment* m_pEnv;
	DXCommandList* m_pCommandList;
	DXCommandAllocator* m_pCommandAllocator;
	DXBindingResourceList* m_pResources;
};

class ClearVoxelGridRecorder
{
public:
	ClearVoxelGridRecorder(ClearVoxelGridInitParams* pParams);
	~ClearVoxelGridRecorder();

	void Record(ClearVoxelGridRecordParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
	u16 m_NumThreadGroupsZ;
};
