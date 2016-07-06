#pragma once

#include "Common/Common.h"

class DXRootSignature;
class DXPipelineState;
class DXCommandSignature;
class DXCommandList;
class DXCommandAllocator;
class DXBuffer;
class MeshBatch;

struct DXViewport;
struct DXRenderEnvironment;
struct DXBindingResourceList;

enum LightType
{
	LightType_Point,
	LightType_Spot
};

class RenderTiledShadowMapRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pRenderEnv;
		DXGI_FORMAT m_DSVFormat;
		MeshBatch* m_pMeshBatch;
		LightType m_LightType;
	};

	struct RenderPassParams
	{
		DXRenderEnvironment* m_pRenderEnv;
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
		DXBindingResourceList* m_pResources;
		DXViewport* m_pViewport;
		MeshBatch* m_pMeshBatch;
	};

	RenderTiledShadowMapRecorder(InitParams* pParams);
	~RenderTiledShadowMapRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXPipelineState* m_pPipelineState;
	DXRootSignature* m_pRootSignature;
	DXCommandSignature* m_pCommandSignature;
};