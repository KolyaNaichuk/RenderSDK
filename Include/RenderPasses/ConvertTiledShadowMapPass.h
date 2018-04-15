#pragma once

#include "D3DWrapper/GraphicsResource.h"
#include "Scene/Light.h"

struct RenderEnv;
struct Viewport;
class RootSignature;
class PipelineState;
class CommandList;

struct ConvertShadowMapParams
{
	f32 m_LightViewNearPlane;
	f32 m_LightRcpViewClipRange;
	f32 m_LightProjMatrix43;
	f32 m_LightProjMatrix33;
};

class ConvertTiledShadowMapPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_TiledShadowMapState;
		D3D12_RESOURCE_STATES m_ShadowMapTileBufferState;
		D3D12_RESOURCE_STATES m_ConvertShadowMapParamsBufferState;
		D3D12_RESOURCE_STATES m_TiledVarianceShadowMapState;
	};

	struct InitParams
	{
		const char* m_pName;
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		LightType m_LightType;
		DepthTexture* m_pTiledShadowMap;
		Buffer* m_pShadowMapTileBuffer;
		Buffer* m_pConvertShadowMapParamsBuffer;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		u32 m_NumShadowMapTiles;
	};

	ConvertTiledShadowMapPass(InitParams* pParams);
	~ConvertTiledShadowMapPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	ColorTexture* GetTiledVarianceShadowMap() { return m_pTiledVarianceShadowMap; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	std::string m_Name;
	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineState = nullptr;
	DescriptorHandle m_SRVHeapStartVS;
	DescriptorHandle m_SRVHeapStartPS;
	DescriptorHandle m_RTVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
	ColorTexture* m_pTiledVarianceShadowMap = nullptr;
	Viewport* m_pViewport = nullptr;
};
