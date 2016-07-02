#pragma once

#include "Common/Common.h"

class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class DXBuffer;

struct DXRenderEnvironment;
struct DXBindingResourceList;

class TiledLightCullingRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pRenderEnv;
		u16 m_TileSize;
		u16 m_NumTilesX;
		u16 m_NumTilesY;
		u32 m_MaxNumPointLights;
		u32 m_MaxNumSpotLights;
	};

	struct RenderPassParams
	{
		DXRenderEnvironment* m_pRenderEnv;
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
		DXBindingResourceList* m_pResources;
		DXBuffer* m_pNumPointLightsPerTileBuffer;
		DXBuffer* m_pNumSpotLightsPerTileBuffer;
	};

	TiledLightCullingRecorder(InitParams* pParams);
	~TiledLightCullingRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
	
	bool m_CullPointLights;
	bool m_CullSpotLights;
};