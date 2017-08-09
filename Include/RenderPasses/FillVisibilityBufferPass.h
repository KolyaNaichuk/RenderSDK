#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class CommandList;
class RootSignature;
class PipelineState;

class FillVisibilityBufferPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_InstanceIndexBufferState;
		D3D12_RESOURCE_STATES m_InstanceWorldViewProjMatrixBufferState;
		D3D12_RESOURCE_STATES m_NumInstancesBufferState;
		D3D12_RESOURCE_STATES m_DepthTextureState;
		D3D12_RESOURCE_STATES m_VisibilityBufferState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		Buffer* m_pInstanceIndexBuffer;
		Buffer* m_pInstanceWorldViewProjMatrixBuffer;
		Buffer* m_pNumInstancesBuffer;
		DepthTexture* m_pDepthTexture;
		bool m_ClampVerticesBehindCameraNearPlane;
		u32 m_MaxNumInstances;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
	};

	FillVisibilityBufferPass(InitParams* pParams);
	~FillVisibilityBufferPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	Buffer* GetVisibilityBuffer() { return m_pVisibilityBuffer; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	DescriptorHandle m_SRVHeapStartVS;
	DescriptorHandle m_SRVHeapStartPS;
	DescriptorHandle m_DSVHeapStart;
	std::vector<ResourceBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;

	Buffer* m_pUnitAABBIndexBuffer;
	Buffer* m_pVisibilityBuffer;
	Buffer* m_pArgumentBuffer;
};