#pragma once

#include "Common/Common.h"

class D3DRootSignature;
class D3DPipelineState;
class D3DCommandList;
class D3DCommandAllocator;
class D3DBuffer;

struct D3DRenderEnv;
struct D3DResourceList;

class TiledLightCullingPass
{
public:
	struct InitParams
	{
		D3DRenderEnv* m_pRenderEnv;
		u16 m_TileSize;
		u16 m_NumTilesX;
		u16 m_NumTilesY;
		u32 m_MaxNumPointLights;
		u32 m_MaxNumSpotLights;
	};

	struct RenderParams
	{
		D3DRenderEnv* m_pRenderEnv;
		D3DCommandList* m_pCommandList;
		D3DCommandAllocator* m_pCommandAllocator;
		D3DResourceList* m_pResources;
		D3DBuffer* m_pNumPointLightsPerTileBuffer;
		D3DBuffer* m_pNumSpotLightsPerTileBuffer;
	};

	TiledLightCullingPass(InitParams* pParams);
	~TiledLightCullingPass();

	void Record(RenderParams* pParams);

private:
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
	
	bool m_CullPointLights;
	bool m_CullSpotLights;
};