#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Viewport;
class CommandList;
class RootSignature;
class PipelineState;
class CommandSignature;

class CreateVoxelGridPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_VoxelColorAndOcclusionMaskTextureState;
		D3D12_RESOURCE_STATES m_VoxelNormalMasksTextureState;
	};
	
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Viewport* m_pViewport;
	};

	CreateVoxelGridPass(InitParams* pParams);
	~CreateVoxelGridPass();

	void Record(RenderParams* pParams);

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void InitCommandSignature(InitParams* pParams);
	void CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	CommandSignature* m_pCommandSignature;
	DescriptorHandle m_SRVHeapStart;
	std::vector<ResourceBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;

	ColorTexture* m_pVoxelColorAndOcclusionMaskTexture;
	ColorTexture* m_pVoxelNormalMasksTexture;
};