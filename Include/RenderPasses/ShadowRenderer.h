#pragma once

#include "D3DWrapper/GraphicsResource.h"
#include "Math/Frustum.h"
#include "Math/Matrix4.h"

struct RenderEnv;
class PointLight;
class SpotLight;
class MeshBatch;
class Camera;

class ShadowRenderer
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;

		u32 m_ShadowMapSize;
		u32 m_MaxNumActiveShadowMaps;
		u32 m_MaxNumStaticShadowMaps;
		MeshBatch* m_pStaticMeshBatch;

		PointLight** m_ppPointLights;
		u32 m_NumPointLights;

		SpotLight** m_ppSpotLights;
		u32 m_NumSpotLights;
	};

	ShadowRenderer(InitParams* pParams);
	~ShadowRenderer();

	void RenderShadowMaps(const Camera& camera);

	ColorTexture* GetShadowMaps() { return m_pShadowMaps; }
	u32 GetPointLightShadowMapsOffset() const { return m_PointLightShadowMapsOffset; }
	u32 GetSpotLightShadowMapsOffset() const { return m_SpotLightShadowMapsOffset; }

private:
	enum class ShadowMapState
	{
		Ready,
		Invalid
	};
	struct CommandsRange
	{
		UINT64 m_CommandOffset;
		UINT m_NumCommands;
	};

	void InitResources(InitParams* pParams);
	void InitStaticMeshCommands(InitParams* pParams);

	void CreateSpotLightRenderStaticGeometryCommands(u32 numSpotLights, SpotLight** ppSpotLights, const MeshBatch* pMeshBatch);
	void CreatePointLightRenderStaticGeometryCommands(u32 numPointLights, PointLight** ppPointLights, const MeshBatch* pMeshBatch);
	
private:
	DepthTexture* m_pActiveShadowMaps = nullptr;
	std::vector<u32> m_ActiveShadowMapIndices;
	
	DepthTexture* m_pStaticMeshShadowMaps = nullptr;
	Buffer* m_pStaticMeshCommandBuffer = nullptr;
	std::vector<CommandsRange> m_StaticMeshCommandsRange;
	Buffer* m_pStaticMeshInstanceIndexBuffer = nullptr;
	std::vector<u32> m_StaticMeshShadowMapIndices;
	u32 m_NumUsedStaticMeshShadowMaps;

	ColorTexture* m_pShadowMaps = nullptr;
	std::vector<ShadowMapState> m_ShadowMapStates;
	u32 m_PointLightShadowMapsOffset;
	u32 m_NumPointLightShadowMaps;
	u32 m_SpotLightShadowMapsOffset;
	u32 m_NumSpotLightShadowMaps;

	std::vector<Frustum> m_LightWorldFrustums;
};