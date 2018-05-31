#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class CommandList;
class RootSignature;
class PipelineState;

struct CreateExpShadowMapParams
{
	f32 m_LightProjMatrix43;
	f32 m_LightProjMatrix33;
	f32 m_LightViewNearPlane;
	f32 m_LightRcpViewClipRange;
	f32 m_ExpShadowMapConstant;
};

class CreateExpShadowMapPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_StandardShadowMapsState;
		D3D12_RESOURCE_STATES m_CreateExpShadowMapParamsBufferState;
		D3D12_RESOURCE_STATES m_ExpShadowMapsState;
	};

	struct InitParams
	{
		const char* m_pName = nullptr;
		RenderEnv* m_pRenderEnv = nullptr;
		ResourceStates m_InputResourceStates;
		DepthTexture* m_pStandardShadowMaps = nullptr;
		Buffer* m_pCreateExpShadowMapParamsBuffer = nullptr;
		ColorTexture* m_pExpShadowMaps = nullptr;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv = nullptr;
		CommandList* m_pCommandList = nullptr;
		DepthTexture* m_pStandardShadowMaps = nullptr;
		ColorTexture* m_pExpShadowMaps = nullptr;
		u32 m_StandardShadowMapIndex = -1;
		u32 m_ExpShadowMapIndex = -1;
	};

	CreateExpShadowMapPass(InitParams* pParams);
	~CreateExpShadowMapPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);

private:
	std::string m_Name;

	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineState = nullptr;
	DescriptorHandle m_SRVHeapStart;
	ResourceStates m_OutputResourceStates;

	u32 m_NumThreadGroupsX = 0;
	u32 m_NumThreadGroupsY = 0;
};