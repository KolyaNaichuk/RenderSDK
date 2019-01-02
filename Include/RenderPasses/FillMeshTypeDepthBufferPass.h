#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Viewport;
class CommandList;
class RootSignature;
class PipelineState;

class FillMeshTypeDepthBufferPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_GBuffer3State;
		D3D12_RESOURCE_STATES m_MeshTypePerMaterialIDBufferState;
		D3D12_RESOURCE_STATES m_MeshTypeDepthTextureState;
	};

	struct InitParams
	{
		const char* m_pName;
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		ColorTexture* m_pGBuffer3;
		Buffer* m_pMeshTypePerMaterialIDBuffer;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Viewport* m_pViewport;
	};

	FillMeshTypeDepthBufferPass(InitParams* pParams);
	~FillMeshTypeDepthBufferPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	DepthTexture* GetMeshTypeDepthTexture() { return m_pMeshTypeDepthTexture; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	std::string m_Name;
	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineState = nullptr;
	DescriptorHandle m_SRVHeapStart;
	DescriptorHandle m_DSVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
	DepthTexture* m_pMeshTypeDepthTexture = nullptr;
};
