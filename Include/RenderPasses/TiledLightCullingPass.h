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

		D3D12_RESOURCE_STATES m_NumPointLightsBufferState;
		D3D12_RESOURCE_STATES m_PointLightIndexBufferState;
		D3D12_RESOURCE_STATES m_PointLightBoundsBufferState;
		D3D12_RESOURCE_STATES m_PointLightIndexPerTileBufferState;
		D3D12_RESOURCE_STATES m_PointLightRangePerTileBufferState;

		D3D12_RESOURCE_STATES m_NumSpotLightsBufferState;
		D3D12_RESOURCE_STATES m_SpotLightIndexBufferState;
		D3D12_RESOURCE_STATES m_SpotLightBoundsBufferState;
		D3D12_RESOURCE_STATES m_SpotLightIndexPerTileBufferState;
		D3D12_RESOURCE_STATES m_SpotLightRangePerTileBufferState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		
		DepthTexture* m_pDepthTexture;
		
		u32 m_MaxNumPointLights;
		Buffer* m_pNumPointLightsBuffer;
		Buffer* m_pPointLightIndexBuffer;
		Buffer* m_pPointLightWorldBoundsBuffer;
		
		u32 m_MaxNumSpotLights;
		Buffer* m_pNumSpotLightsBuffer;
		Buffer* m_pSpotLightIndexBuffer;
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
	};

	TiledLightCullingPass(InitParams* pParams);
	~TiledLightCullingPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
		
	Buffer* GetPointLightIndexPerTileBuffer() { return m_pPointLightIndexPerTileBuffer; }
	Buffer* GetPointLightRangePerTileBuffer() {	return m_pPointLightRangePerTileBuffer; }

	Buffer* GetSpotLightIndexPerTileBuffer() { return m_pSpotLightIndexPerTileBuffer; }
	Buffer* GetSpotLightRangePerTileBuffer() { return m_pSpotLightRangePerTileBuffer; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	DescriptorHandle m_SRVHeapStart;
	std::vector<ResourceBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;

	DescriptorHandle m_PointLightIndicesOffsetBufferUAV;
	Buffer* m_pPointLightIndicesOffsetBuffer;
	Buffer* m_pPointLightIndexPerTileBuffer;
	Buffer* m_pPointLightRangePerTileBuffer;

	DescriptorHandle m_SpotLightIndicesOffsetBufferUAV;
	Buffer* m_pSpotLightIndicesOffsetBuffer;
	Buffer* m_pSpotLightIndexPerTileBuffer;
	Buffer* m_pSpotLightRangePerTileBuffer;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
};