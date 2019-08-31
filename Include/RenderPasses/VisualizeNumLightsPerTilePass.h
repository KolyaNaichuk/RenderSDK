#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Viewport;
class Buffer;
class RootSignature;
class PipelineState;
class CommandList;

class VisualizeNumLightsPerTilePass
{
public:
	enum ColorMode
	{
		GrayscaleColor = 1,
		RadarColor
	};

	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_LightRangePerTileBufferState;
		D3D12_RESOURCE_STATES m_BackBufferState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		Buffer* m_pLightRangePerTileBuffer;
		ColorTexture* m_pBackBuffer;
		ColorMode m_ColorMode;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pAppDataBuffer;
		Viewport* m_pViewport;
		u32 m_MaxNumLights;
	};

	VisualizeNumLightsPerTilePass(InitParams* pParams);
	~VisualizeNumLightsPerTilePass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineState = nullptr;
	DescriptorHandle m_SRVHeapStart;
	DescriptorHandle m_RTVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
};
