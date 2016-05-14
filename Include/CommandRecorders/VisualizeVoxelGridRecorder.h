#pragma once

#include "Common/Common.h"

class DXCommandList;
class DXCommandAllocator;
class DXRootSignature;
class DXPipelineState;

struct DXRenderEnvironment;
struct DXBindingResourceList;
struct DXViewport;

class VisualizeVoxelGridRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pEnv;
		DXGI_FORMAT m_RTVFormat;
	};
	
	struct RenderPassParams
	{
		DXRenderEnvironment* m_pEnv;
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
		DXBindingResourceList* m_pResources;
		DXViewport* m_pViewport;
	};

	VisualizeVoxelGridRecorder(InitParams* pParams);
	~VisualizeVoxelGridRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};