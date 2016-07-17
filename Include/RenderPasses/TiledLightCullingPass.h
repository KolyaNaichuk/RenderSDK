#pragma once

#include "D3DWrapper/Common.h"

class RootSignature;
class PipelineState;
class CommandList;
class Buffer;

struct RenderEnv;
struct BindingResourceList;

class TiledLightCullingPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		u16 m_TileSize;
		u16 m_NumTilesX;
		u16 m_NumTilesY;
		u32 m_MaxNumPointLights;
		u32 m_MaxNumSpotLights;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		BindingResourceList* m_pResources;
		Buffer* m_pNumPointLightsPerTileBuffer;
		Buffer* m_pNumSpotLightsPerTileBuffer;
	};

	TiledLightCullingPass(InitParams* pParams);
	~TiledLightCullingPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
	
	bool m_CullPointLights;
	bool m_CullSpotLights;
};