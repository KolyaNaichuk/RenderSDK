#pragma once

#include "D3DWrapper/Common.h"

class RootSignature;
class PipelineState;
class CommandSignature;
class CommandList;
class Buffer;
class MeshBatch;

struct Viewport;
struct RenderEnv;
struct BindingResourceList;

enum LightType
{
	LightType_Point = 1,
	LightType_Spot = 2
};

class RenderTiledShadowMapPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		DXGI_FORMAT m_DSVFormat;
		MeshBatch* m_pMeshBatch;
		LightType m_LightType;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		BindingResourceList* m_pResources;
		Viewport* m_pViewport;
		MeshBatch* m_pMeshBatch;
		Buffer* m_pDrawShadowCasterCommandBuffer;
		Buffer* m_pNumDrawShadowCastersBuffer;
	};

	RenderTiledShadowMapPass(InitParams* pParams);
	~RenderTiledShadowMapPass();

	void Record(RenderParams* pParams);

private:
	PipelineState* m_pPipelineState;
	RootSignature* m_pRootSignature;
	CommandSignature* m_pCommandSignature;
};