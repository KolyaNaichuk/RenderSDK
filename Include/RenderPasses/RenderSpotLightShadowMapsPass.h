#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class SpotLight;
class MeshBatch;

class RenderSpotLightShadowMapsPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_SpotLightShadowMapsState;
		D3D12_RESOURCE_STATES m_SpotLightViewProjMatrixBufferState;
	};
	
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		u32 m_NumSpotLights;
		SpotLight** m_ppSpotLights;
		u32 m_MaxNumActiveSpotLights;
		u32 m_NumStaticMeshTypes;
		MeshBatch** m_ppStaticMeshBatches;
		Buffer* m_pSpotLightViewProjMatrixBuffer;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		u32 m_NumActiveSpotLights;
		const u32* m_ActiveSpotLightIndices;
	};

	RenderSpotLightShadowMapsPass(InitParams* pParams);
	~RenderSpotLightShadowMapsPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

	ColorTexture* GetSpotLightShadowMaps() { return m_pSpotLightShadowMaps; }

private:
	enum class ShadowMapState
	{
		UpToDate,
		Outdated
	};
	struct CommandRange
	{
		UINT64 m_FirstCommand;
		UINT m_NumCommands;
	};

	void InitResources(InitParams* pParams);
	void InitStaticMeshCommands(InitParams* pParams);
	
private:
	DepthTexture* m_pActiveShadowMaps = nullptr;
		
	Buffer* m_pStaticMeshCommandBuffer = nullptr;
	std::vector<CommandRange> m_StaticMeshCommandRanges;
	Buffer* m_pStaticMeshInstanceIndexBuffer = nullptr;
	
	ColorTexture* m_pSpotLightShadowMaps = nullptr;
	std::vector<ShadowMapState> m_SpotLightShadowMapStates;
	std::vector<u32> m_OutdatedSpotLightShadowMapIndices;

	ResourceStates m_OutputResourceStates;
};