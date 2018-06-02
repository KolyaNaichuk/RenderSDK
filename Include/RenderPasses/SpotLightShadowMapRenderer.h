#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class SpotLight;
class MeshBatch;
class MeshRenderResources;
class CommandList;

class RenderSpotLightShadowMapPass;
class CreateExpShadowMapPass;
class FilterExpShadowMapPass;

class SpotLightShadowMapRenderer
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_SpotLightShadowMapsState;
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
		MeshRenderResources* m_pStaticMeshRenderResources;
		u32 m_ShadowMapSize;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		u32 m_NumActiveSpotLights;
		const u32* m_ActiveSpotLightIndices;
		MeshRenderResources* m_pStaticMeshRenderResources;
	};

	SpotLightShadowMapRenderer(InitParams* pParams);
	~SpotLightShadowMapRenderer();

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
	void InitRenderSpotLightShadowMapPass(InitParams* pParams);
	void InitCreateExpShadowMapPass(InitParams* pParams);
	void InitFilterExpShadowMapPass(InitParams* pParams);

private:
	DepthTexture* m_pActiveShadowMaps = nullptr;
			
	Buffer* m_pStaticMeshCommandBuffer = nullptr;
	std::vector<CommandRange> m_StaticMeshCommandRanges;
	Buffer* m_pStaticMeshInstanceIndexBuffer = nullptr;
	
	ColorTexture* m_pSpotLightShadowMaps = nullptr;
	std::vector<ShadowMapState> m_SpotLightShadowMapStates;
	std::vector<u32> m_OutdatedSpotLightShadowMapIndices;

	ResourceStates m_OutputResourceStates;

	Buffer* m_pSpotLightViewProjMatrixBuffer = nullptr;
	Buffer* m_pCreateExpShadowMapParamsBuffer = nullptr;

	RenderSpotLightShadowMapPass* m_pRenderSpotLightShadowMapPass = nullptr;
	CreateExpShadowMapPass* m_pCreateExpShadowMapPass = nullptr;
	FilterExpShadowMapPass* m_pFilterExpShadowMapPass = nullptr;
};
