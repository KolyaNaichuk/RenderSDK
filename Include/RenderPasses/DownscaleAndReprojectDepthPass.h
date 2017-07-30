#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;

class CommandList;
class RootSignature;
class PipelineState;

class DownscaleAndReprojectDepthPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_PrevDepthTextureState;
		D3D12_RESOURCE_STATES m_ReprojectedDepthTextureState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		DepthTexture* m_pPrevDepthTexture;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pReprojectionDataBuffer;
	};

	DownscaleAndReprojectDepthPass(InitParams* pParams);
	~DownscaleAndReprojectDepthPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

private:
	void InitResources(InitParams* pParams, UINT64 reprojectedDepthTextureWidth, UINT reprojectedDepthTextureHeight);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams, UINT64 reprojectedDepthTextureWidth, UINT reprojectedDepthTextureHeight);
	void CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	DescriptorHandle m_SRVHeapStart;
	std::vector<ResourceBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
	ColorTexture* m_pReprojectedDepthTexture;
	u32 m_NumThreadGroupsX;
	u32 m_NumThreadGroupsY;
};