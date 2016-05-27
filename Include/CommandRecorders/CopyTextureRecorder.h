#pragma once

#include "Common/Common.h"

class DXCommandList;
class DXCommandAllocator;
class DXPipelineState;
class DXRootSignature;

struct DXBindingResourceList;
struct DXRenderEnvironment;
struct DXViewport;

class CopyTextureRecorder
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

	CopyTextureRecorder(InitParams* pParams);
	~CopyTextureRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};
