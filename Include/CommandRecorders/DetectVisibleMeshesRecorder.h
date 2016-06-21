#pragma once

#include "Common/Common.h"

class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class DXBuffer;
struct DXRenderEnvironment;
struct DXBindingResourceList;

class DetectVisibleMeshesRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pRenderEnv;
		u32 m_NumMeshesInBatch;
	};
	struct RenderPassParams
	{
		DXRenderEnvironment* m_pRenderEnv;
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
		DXBindingResourceList* m_pResources;
		DXBuffer* m_pNumVisibleMeshesBuffer;
	};
	
	DetectVisibleMeshesRecorder(InitParams* pParams);
	~DetectVisibleMeshesRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
};