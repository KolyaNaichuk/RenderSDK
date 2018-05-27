#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class CommandList;
class RootSignature;
class PipelineState;

class CreateExpShadowMapPass
{
public:
	struct ResourceStates
	{
	};

	struct InitParams
	{
		const char* m_pName = nullptr;
		RenderEnv* m_pRenderEnv = nullptr;
		ResourceStates m_InputResourceStates;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv = nullptr;
		CommandList* m_pCommandList = nullptr;
	};

	CreateExpShadowMapPass(InitParams* pParams);
	~CreateExpShadowMapPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

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
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
};