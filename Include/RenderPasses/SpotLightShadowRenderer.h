#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class SpotLight;
class MeshBatch;

class SpotLightShadowRenderer
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		MeshBatch* m_pStaticMeshBatch;
		SpotLight** m_ppSpotLights;
		u32 m_NumSpotLights;
		u32 m_MaxNumActiveSpotLights;
	};

	SpotLightShadowRenderer(InitParams* pParams);
	~SpotLightShadowRenderer();

	void RenderSpotLightShadowMaps(u32 numActiveSpotLights, const u32* pFirstActiveSpotLightIndex);
	ColorTexture* GetSpotLightShadowMaps() { return m_pSpotLightShadowMaps; }

private:
	enum class ShadowMapState
	{
		UpToDate,
		Outdated
	};
	struct CommandsRange
	{
		UINT64 m_CommandOffset;
		UINT m_NumCommands;
	};

	void InitResources(InitParams* pParams);
	void InitStaticMeshCommands(InitParams* pParams);
	
private:
	DepthTexture* m_pActiveShadowMaps = nullptr;
	
	Buffer* m_pStaticMeshCommandBuffer = nullptr;
	std::vector<CommandsRange> m_StaticMeshCommandsRange;
	Buffer* m_pStaticMeshInstanceIndexBuffer = nullptr;
	
	ColorTexture* m_pSpotLightShadowMaps = nullptr;
	std::vector<ShadowMapState> m_SpotLightShadowMapStates;
	std::vector<u32> m_OutdatedSpotLightShadowMapIndices;
};