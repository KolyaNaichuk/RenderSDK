#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class CommandList;
class RootSignature;
class PipelineState;

class RenderSpotLightShadowMapPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_RenderCommandBufferState;
		D3D12_RESOURCE_STATES m_MeshInstanceIndexBufferState;
		D3D12_RESOURCE_STATES m_MeshInstanceWorldMatrixBufferState;
		D3D12_RESOURCE_STATES m_SpotLightViewProjMatrixBufferState;
	};

	struct InitParams
	{
		const char* m_pName = nullptr;
		RenderEnv* m_pRenderEnv = nullptr;
		ResourceStates m_InputResourceStates;
		Buffer* m_pRenderCommandBuffer = nullptr;
		Buffer* m_pMeshInstanceIndexBuffer = nullptr;
		Buffer* m_MeshInstanceWorldMatrixBuffer = nullptr;
		Buffer* m_pSpotLightViewProjMatrixBuffer = nullptr;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv = nullptr;
		CommandList* m_pCommandList = nullptr;
		Buffer* m_pRenderCommandBuffer = nullptr; 
		UINT64 m_FirstRenderCommand = 0;
		UINT m_NumRenderCommands = 0;
	};

	RenderSpotLightShadowMapPass(InitParams* pParams);
	~RenderSpotLightShadowMapPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void InitCommandSignature(InitParams* pParams);

private:
	std::string m_Name;

	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineState = nullptr;
	DescriptorHandle m_SRVHeapStart;
	ResourceStates m_OutputResourceStates;
};