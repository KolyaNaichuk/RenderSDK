#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class RootSignature;
class PipelineState;
class CommandList;

class FrustumLightCullingPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_LightWorldBoundsBufferState;
		D3D12_RESOURCE_STATES m_NumVisibleLightsBufferState;
		D3D12_RESOURCE_STATES m_VisibleLightIndexBufferState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		Buffer* m_pLightWorldBoundsBuffer;
		u32 m_NumLights;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pAppDataBuffer;
	};
	
	FrustumLightCullingPass(InitParams* pParams);
	~FrustumLightCullingPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	Buffer* GetNumVisibleLightsBuffer() { return m_pNumVisibleLightsBuffer; }
	Buffer* GetVisibleLightIndexBuffer() { return m_pVisibleLightIndexBuffer; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	Buffer* m_pNumVisibleLightsBuffer;
	Buffer* m_pVisibleLightIndexBuffer;
	DescriptorHandle m_SRVHeapStart;
	std::vector<ResourceBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
	u16 m_NumThreadGroupsX;
};