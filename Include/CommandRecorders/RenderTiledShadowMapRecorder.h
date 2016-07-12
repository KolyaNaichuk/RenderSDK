#pragma once

#include "Common/Common.h"

class D3DRootSignature;
class D3DPipelineState;
class D3DCommandSignature;
class D3DCommandList;
class D3DCommandAllocator;
class D3DBuffer;
class MeshBatch;

struct D3DViewport;
struct D3DRenderEnv;
struct D3DResourceList;

enum LightType
{
	LightType_Point = 1,
	LightType_Spot = 2
};

class RenderTiledShadowMapRecorder
{
public:
	struct InitParams
	{
		D3DRenderEnv* m_pRenderEnv;
		DXGI_FORMAT m_DSVFormat;
		MeshBatch* m_pMeshBatch;
		LightType m_LightType;
	};

	struct RenderPassParams
	{
		D3DRenderEnv* m_pRenderEnv;
		D3DCommandList* m_pCommandList;
		D3DCommandAllocator* m_pCommandAllocator;
		D3DResourceList* m_pResources;
		D3DViewport* m_pViewport;
		MeshBatch* m_pMeshBatch;
		D3DBuffer* m_pDrawShadowCasterCommandBuffer;
		D3DBuffer* m_pNumDrawShadowCastersBuffer;
	};

	RenderTiledShadowMapRecorder(InitParams* pParams);
	~RenderTiledShadowMapRecorder();

	void Record(RenderPassParams* pParams);

private:
	D3DPipelineState* m_pPipelineState;
	D3DRootSignature* m_pRootSignature;
	D3DCommandSignature* m_pCommandSignature;
};