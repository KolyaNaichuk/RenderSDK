#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Viewport;
class CommandList;
class RootSignature;
class PipelineState;

class VisualizeDepthTexturePass
{
public:
	struct CameraSettings
	{
		f32 m_ProjMatrix32;
		f32 m_ProjMatrix22;
		f32 m_ViewNearPlaneDist;
		f32 m_RcpViewClipRange;
	};

	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_InputTextureState;
		D3D12_RESOURCE_STATES m_BackBufferState;
	};

	struct InitParams
	{
		const char* m_pName;
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		GraphicsResource* m_pInputTexture;
		DescriptorHandle m_InputTextureSRV;
		ColorTexture* m_pBackBuffer;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		CameraSettings m_CameraSettings;
		Viewport* m_pViewport;
	};

	VisualizeDepthTexturePass(InitParams* pParams);
	~VisualizeDepthTexturePass();

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
	DescriptorHandle m_RTVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
};
