#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Viewport;
class CommandList;
class RootSignature;
class PipelineState;

class VisualizeVoxelReflectancePass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_BackBufferState;
		D3D12_RESOURCE_STATES m_DepthTextureState;
		D3D12_RESOURCE_STATES m_VoxelReflectanceTextureState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		ColorTexture* m_pBackBuffer;
		DepthTexture* m_pDepthTexture;
		ColorTexture* m_pVoxelReflectanceTexture;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pAppDataBuffer;
		Viewport* m_pViewport;
	};

	VisualizeVoxelReflectancePass(InitParams* pParams);
	~VisualizeVoxelReflectancePass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineState = nullptr;
	DescriptorHandle m_SRVHeapStartPS;
	DescriptorHandle m_RTVHeapStart;
	ResourceStates m_OutputResourceStates;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
};