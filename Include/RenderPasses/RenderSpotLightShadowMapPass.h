#pragma once

#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/CommandSignature.h"

struct RenderEnv;
class CommandList;
class RootSignature;
class PipelineState;
class CommandSignature;
class MeshRenderResources;

struct ShadowMapCommand
{
	UINT m_InstanceOffset;
	DrawIndexedArguments m_Args;
};

class RenderSpotLightShadowMapPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_RenderCommandBufferState;
		D3D12_RESOURCE_STATES m_MeshInstanceIndexBufferState;
		D3D12_RESOURCE_STATES m_MeshInstanceWorldMatrixBufferState;
		D3D12_RESOURCE_STATES m_SpotLightViewProjMatrixBufferState;
		D3D12_RESOURCE_STATES m_SpotLightShadowMapsState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv = nullptr;
		ResourceStates m_InputResourceStates;
		MeshRenderResources* m_pMeshRenderResources = nullptr;
		Buffer* m_pRenderCommandBuffer = nullptr;
		Buffer* m_pMeshInstanceIndexBuffer = nullptr;
		Buffer* m_pMeshInstanceWorldMatrixBuffer = nullptr;
		Buffer* m_pSpotLightViewProjMatrixBuffer = nullptr;
		DepthTexture* m_pSpotLightShadowMaps = nullptr;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv = nullptr;
		CommandList* m_pCommandList = nullptr;
		MeshRenderResources* m_pMeshRenderResources = nullptr;
		DepthTexture* m_pSpotLightShadowMaps = nullptr;
		Buffer* m_pRenderCommandBuffer = nullptr;
		UINT64 m_FirstRenderCommand = 0;
		UINT m_NumRenderCommands = 0;
		UINT m_SpotLightIndex = -1;
		UINT m_ShadowMapIndex = -1;
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
	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineState = nullptr;
	CommandSignature* m_pCommandSignature = nullptr;
	DescriptorHandle m_SRVHeapStartVS;
	ResourceStates m_OutputResourceStates;
};