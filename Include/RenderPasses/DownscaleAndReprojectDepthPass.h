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
		const char* m_pName;
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
	std::string m_Name;

	RootSignature* m_pReprojectRootSignature = nullptr;
	PipelineState* m_pReprojectPipelineState = nullptr;
	DescriptorHandle m_ReprojectSRVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ReprojectResourceBarriers;
	ColorTexture* m_pReprojectionColorTexture = nullptr;
	u32 m_NumThreadGroupsX = 0;
	u32 m_NumThreadGroupsY = 0;
	
	RootSignature* m_pCopyRootSignature = nullptr;
	PipelineState* m_pCopyPipelineState = nullptr;
	DescriptorHandle m_CopySRVHeapStart;
	DescriptorHandle m_CopyDSVHeapStart;
	std::vector<ResourceTransitionBarrier> m_CopyResourceBarriers;
	DepthTexture* m_pReprojectionDepthTexture = nullptr;
	Viewport* m_pCopyViewport = nullptr;

	ResourceStates m_OutputResourceStates;
};