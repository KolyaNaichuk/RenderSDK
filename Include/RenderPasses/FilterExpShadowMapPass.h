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
		D3D12_RESOURCE_STATES m_ExpShadowMapsState;
	};

	struct InitParams
	{
		const char* m_pName = nullptr;
		RenderEnv* m_pRenderEnv = nullptr;
		ResourceStates m_InputResourceStates;
		u32 m_MaxNumActiveExpShadowMaps = 0;
		ColorTexture* m_pExpShadowMaps = nullptr;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv = nullptr;
		CommandList* m_pCommandList = nullptr;
		ColorTexture* m_pExpShadowMaps = nullptr;
		u32 m_ExpShadowMapIndex = -1;
		u32 m_IntermediateResultIndex = -1;
	};

	FilterExpShadowMapPass(InitParams* pParams);
	~FilterExpShadowMapPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);

private:
	std::string m_Name;

	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineStateX = nullptr;
	DescriptorHandle m_SRVHeapStartX;
	PipelineState* m_pPipelineStateY = nullptr;
	DescriptorHandle m_SRVHeapStartY;
	u32 m_NumThreadGroupsX = 0;
	u32 m_NumThreadGroupsY = 0;

	ColorTexture* m_pIntermediateResults = nullptr;
	ResourceStates m_OutputResourceStates;
};