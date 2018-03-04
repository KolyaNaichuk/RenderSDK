#pragma once

#include "D3DWrapper/GraphicsResource.h"
#include "Math/Vector2.h"
#include "Common/Light.h"

struct RenderEnv;
struct Viewport;
class CommandList;
class CommandSignature;
class RootSignature;
class PipelineState;
class MeshRenderResources;

class RenderTiledShadowMapPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_TiledShadowMapState;
		D3D12_RESOURCE_STATES m_MeshInstanceWorldMatrixBufferState;
		D3D12_RESOURCE_STATES m_LightWorldBoundsOrPropsBufferState;
		D3D12_RESOURCE_STATES m_LightWorldFrustumBufferState;
		D3D12_RESOURCE_STATES m_LightViewProjMatrixBufferState;
		D3D12_RESOURCE_STATES m_LightIndexForMeshInstanceBufferState;
		D3D12_RESOURCE_STATES m_MeshInstanceIndexForLightBufferState;
		D3D12_RESOURCE_STATES m_NumShadowMapCommandsBufferState;
		D3D12_RESOURCE_STATES m_ShadowMapCommandBufferState;
	};
	
	struct InitParams
	{
		const char* m_pName;
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		LightType m_LightType;
		u32 m_ShadowMapSize;
		MeshRenderResources* m_pMeshRenderResources;
		Buffer* m_pMeshInstanceWorldMatrixBuffer;
		Buffer* m_pLightWorldBoundsOrPropsBuffer;
		Buffer* m_pLightWorldFrustumBuffer;
		Buffer* m_pLightViewProjMatrixBuffer;
		Buffer* m_pLightIndexForMeshInstanceBuffer;
		Buffer* m_pMeshInstanceIndexForLightBuffer;
		Buffer* m_pNumShadowMapCommandsBuffer;
		Buffer* m_pShadowMapCommandBuffer;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		MeshRenderResources* m_pMeshRenderResources;
		Buffer* m_pNumShadowMapCommandsBuffer;
		Buffer* m_pShadowMapCommandBuffer;
	};

	RenderTiledShadowMapPass(InitParams* pParams);
	~RenderTiledShadowMapPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	DepthTexture* GetTiledShadowMap() { return m_pTiledShadowMap; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void InitCommandSignature(InitParams* pParams);
	void AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	std::string m_Name;
	PipelineState* m_pPipelineState = nullptr;
	RootSignature* m_pRootSignature = nullptr;
	CommandSignature* m_pCommandSignature = nullptr;
	ResourceStates m_OutputResourceStates;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	DescriptorHandle m_SRVHeapStartVS;
	DescriptorHandle m_SRVHeapStartGS;
	DescriptorHandle m_DSVHeapStart;
	DepthTexture* m_pTiledShadowMap = nullptr;
	Viewport* m_pViewport = nullptr;
};

struct ShadowMapTile
{
	Vector2f m_TexSpaceTopLeft;
	f32 m_TexSpaceSize = 0.0f;
};

class ShadowMapTileAllocator
{
public:
	ShadowMapTileAllocator(u32 shadowMapSize, u32 numLevels);
	~ShadowMapTileAllocator();

	bool Allocate(u32 tileSize, ShadowMapTile* pNewTile);
	void Reset();
	
private:	
	u32 CalcTreeLevel(u32 tileSize) const;
	u32 FindFreeNode(u32 currentLevel, u32 currentIndex, u32 requiredLevel);

private:
	u32 m_MaxTileSize = 0;
	f32 m_RcpMaxTileSize = 0.0f;
	f32 m_Log2MaxTileSize = 0.0f;
	u32 m_MinTileSize = 0;
	u32 m_NumNodes = 0;
	u8* m_pFreeNodes = nullptr;
	ShadowMapTile* m_pTiles = nullptr;
};