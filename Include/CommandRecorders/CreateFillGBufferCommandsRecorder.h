#pragma once

#include "Common/Common.h"

class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
struct DXRenderEnvironment;
struct DXBindingResourceList;

class CreateFillGBufferCommandsRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pEnv;
		u32 m_NumMeshesInBatch;
	};
	struct RenderPassParams
	{
		DXRenderEnvironment* m_pEnv;
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
		DXBindingResourceList* m_pResources;
	};

	CreateFillGBufferCommandsRecorder(InitParams* pParams);
	~CreateFillGBufferCommandsRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
};