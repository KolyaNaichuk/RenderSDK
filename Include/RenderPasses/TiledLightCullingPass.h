#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class RootSignature;
class PipelineState;
class CommandList;

class TiledLightCullingPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_DepthTextureState;
		D3D12_RESOURCE_STATES m_SpotLightWorldBoundsBufferState;
		D3D12_RESOURCE_STATES m_SpotLightIndexPerTileBufferState;
		D3D12_RESOURCE_STATES m_SpotLightRangePerTileBufferState;
	};

	struct InitParams
	{
		const char* m_pName;
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		DepthTexture* m_pDepthTexture;
				
		u32 m_MaxNumSpotLights;
		Buffer* m_pSpotLightWorldBoundsBuffer;

		u16 m_TileSize;
		u16 m_NumTilesX;
		u16 m_NumTilesY;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pAppDataBuffer;
		u32 m_NumSpotLights;
	};

	TiledLightCullingPass(InitParams* pParams);
	~TiledLightCullingPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	
	Buffer* GetSpotLightIndexPerTileBuffer() { return m_pSpotLightIndexPerTileBuffer; }
	Buffer* GetSpotLightRangePerTileBuffer() { return m_pSpotLightRangePerTileBuffer; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	std::string m_Name;

	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineState = nullptr;
	DescriptorHandle m_SRVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;

	Buffer* m_pSpotLightIndicesOffsetBuffer = nullptr;
	Buffer* m_pSpotLightIndexPerTileBuffer = nullptr;
	Buffer* m_pSpotLightRangePerTileBuffer = nullptr;

	u16 m_NumThreadGroupsX = 0;
	u16 m_NumThreadGroupsY = 0;
};