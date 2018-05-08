#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Frustum;
class PointLight;
class SpotLight;
class MeshBatch;

class ShadowRenderer
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;

		u32 m_ShadowMapSize;
		u32 m_NumActiveShadowMaps;
		u32 m_NumShadowMapsInStaticCache;
		u32 m_NumShadowMapsInStaleCache;

		MeshBatch* m_pMeshBatch;

		PointLight** m_ppPointLights;
		u32 m_NumPointLights;

		SpotLight** m_ppSpotLights;
		u32 m_NumSpotLights;
	};

	ShadowRenderer(InitParams* pParams);
	~ShadowRenderer();

private:
	void InitResources(InitParams* pParams);
	void CreateSpotLightRenderStaticGeometryCommands(u32 numSpotLights, SpotLight** ppSpotLights, const MeshBatch* pMeshBatch);
	void CreatePointLightRenderStaticGeometryCommands(u32 numPointLights, PointLight** ppPointLights, const MeshBatch* pMeshBatch);
		
private:
	DepthTexture* m_pActiveShadowMaps = nullptr;
	DepthTexture* m_pStaticCache = nullptr;
	ColorTexture* m_pESMStaleCache = nullptr;
	Buffer* m_pRenderStaticGeometryCommands = nullptr;

	u32 m_SpotLightRenderStaticGeometryCommandsOffset;
	u32 m_PointLightRenderStaticGeometryCommandsOffset;
};