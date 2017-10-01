#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class RootSignature;
class PipelineState;
class CommandList;

class CalcShadingRectanglesPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_MeshTypeDepthTextureState;
		D3D12_RESOURCE_STATES m_ShadingRectangleBufferState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		DepthTexture* m_pMeshTypeDepthTexture;
		u32 m_NumMeshTypes;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pAppDataBuffer;
	};

	CalcShadingRectanglesPass(InitParams* pParams);
	~CalcShadingRectanglesPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	Buffer* GetScreenRectPerMeshTypeBuffer() { return m_pShadngRectangleBuffer; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	DescriptorHandle m_SRVHeapStart;
	std::vector<ResourceBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
	Buffer* m_pShadngRectangleBuffer;
	u32 m_NumThreadGroupsX;
	u32 m_NumThreadGroupsY;
};