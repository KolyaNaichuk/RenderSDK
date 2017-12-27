#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Viewport;
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
		Buffer* m_pAppDataBuffer;
	};

	DownscaleAndReprojectDepthPass(InitParams* pParams);
	~DownscaleAndReprojectDepthPass();

	void Record(RenderParams* pParams);

	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	DepthTexture* GetReprojectedDepthTexture() { return m_pReprojectionDepthTexture; }
	
private:
	void InitReprojectResources(InitParams* pParams, UINT64 reprojectedDepthTextureWidth, UINT reprojectedDepthTextureHeight);
	void InitReprojectRootSignature(InitParams* pParams);
	void InitReprojectPipelineState(InitParams* pParams, UINT64 reprojectedDepthTextureWidth, UINT reprojectedDepthTextureHeight);
	void AddReprojectResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

	void InitCopyResources(InitParams* pParams, UINT64 reprojectedDepthTextureWidth, UINT reprojectedDepthTextureHeight);
	void InitCopyRootSignature(InitParams* pParams);
	void InitCopyPipelineState(InitParams* pParams);
	void AddCopyResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pReprojectRootSignature;
	PipelineState* m_pReprojectPipelineState;
	DescriptorHandle m_ReprojectSRVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ReprojectResourceBarriers;
	ColorTexture* m_pReprojectionColorTexture;
	u32 m_NumThreadGroupsX;
	u32 m_NumThreadGroupsY;
	
	RootSignature* m_pCopyRootSignature;
	PipelineState* m_pCopyPipelineState;
	DescriptorHandle m_CopySRVHeapStart;
	DescriptorHandle m_CopyDSVHeapStart;
	std::vector<ResourceTransitionBarrier> m_CopyResourceBarriers;
	DepthTexture* m_pReprojectionDepthTexture;
	Viewport* m_pCopyViewport;

	ResourceStates m_OutputResourceStates;
};