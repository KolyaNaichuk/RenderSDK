#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Viewport;
class CommandList;
class RootSignature;
class PipelineState;

class VisualizeTexturePass
{
public:
	enum TextureType
	{
		TextureType_GBufferNormal = 1,
		TextureType_GBufferTexCoord,
		TextureType_Depth,
		TextureType_Other
	};

	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_InputTextureState;
		D3D12_RESOURCE_STATES m_BackBufferState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		GraphicsResource* m_pInputTexture;
		DescriptorHandle m_InputTextureSRV;
		ColorTexture* m_pBackBuffer;
		TextureType m_TextureType;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pAppDataBuffer;
		Viewport* m_pViewport;
	};

	VisualizeTexturePass(InitParams* pParams);
	~VisualizeTexturePass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	DescriptorHandle m_SRVHeapStart;
	DescriptorHandle m_RTVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
};
