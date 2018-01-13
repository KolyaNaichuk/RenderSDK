#pragma once

#include "D3DWrapper/GraphicsResource.h"
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
		D3D12_RESOURCE_STATES m_MeshInstanceWorldMatrixBufferState;
		D3D12_RESOURCE_STATES m_LightIndexForMeshInstanceBufferState;
		D3D12_RESOURCE_STATES m_MeshInstanceIndexForLightBufferState;
		D3D12_RESOURCE_STATES m_LightWorldBoundsOrPropsBufferState;
		D3D12_RESOURCE_STATES m_LightWorldFrustumBufferState;
		D3D12_RESOURCE_STATES m_LightViewProjMatrixBufferState;
		D3D12_RESOURCE_STATES m_TiledShadowMapState;
		D3D12_RESOURCE_STATES m_NumShadowMapCommandsBufferState;
		D3D12_RESOURCE_STATES m_ShadowMapCommandBufferState;
	};
	
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		LightType m_LightType;
		u32 m_ShadowMapWidth;
		u32 m_ShadowMapHeight;
		MeshRenderResources* m_pMeshRenderResources;
		Buffer* m_pMeshInstanceWorldMatrixBuffer;
		Buffer* m_pLightIndexForMeshInstanceBuffer;
		Buffer* m_pMeshInstanceIndexForLightBuffer;
		Buffer* m_pLightWorldBoundsOrPropsBuffer;
		Buffer* m_pLightWorldFrustumBuffer;
		Buffer* m_pLightViewProjMatrixBuffer;
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
	DepthTexture* GetShadowMap() { return m_pShadowMap; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void InitCommandSignature(InitParams* pParams);
	void AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	PipelineState* m_pPipelineState;
	RootSignature* m_pRootSignature;
	CommandSignature* m_pCommandSignature;
	ResourceStates m_OutputResourceStates;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	DescriptorHandle m_SRVHeapStartVS;
	DescriptorHandle m_SRVHeapStartGS;
	DescriptorHandle m_DSVHeapStart;
	DepthTexture* m_pShadowMap;
	Viewport* m_pViewport;
};