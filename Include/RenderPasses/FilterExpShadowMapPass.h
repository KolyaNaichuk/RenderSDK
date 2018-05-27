#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class CommandList;
class RootSignature;
class PipelineState;

class FilterExpShadowMapPass
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

	FilterExpShadowMapPass(InitParams* pParams);
	~FilterExpShadowMapPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignatures(InitParams* pParams);
	void InitPipelineStates(InitParams* pParams);

private:
	std::string m_Name;

	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineState = nullptr;
	DescriptorHandle m_SRVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
};