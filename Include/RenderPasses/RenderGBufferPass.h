#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Viewport;
class CommandList;
class CommandSignature;
class RootSignature;
class PipelineState;

class RenderGBufferPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_TexCoordTextureState;
		D3D12_RESOURCE_STATES m_NormalTextureState;
		D3D12_RESOURCE_STATES m_MaterialTextureState;
		D3D12_RESOURCE_STATES m_DepthTextureState;
		D3D12_RESOURCE_STATES m_InstanceIndexBufferState;
		D3D12_RESOURCE_STATES m_InstanceWorldMatrixBufferState;
		D3D12_RESOURCE_STATES m_NumVisibleMeshesPerTypeBufferState;
		D3D12_RESOURCE_STATES m_DrawCommandBufferState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		D3D12_INPUT_LAYOUT_DESC* m_pInputLayoutDesc;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE m_PrimitiveTopologyType;
		ColorTexture* m_pTexCoordTexture;
		ColorTexture* m_pNormalTexture;
		ColorTexture* m_pMaterialTexture;
		DepthTexture* m_pDepthTexture;
		Buffer* m_pInstanceIndexBuffer;
		Buffer* m_pInstanceWorldMatrixBuffer;
		Buffer* m_NumVisibleMeshesPerTypeBuffer;
		Buffer* m_DrawCommandBuffer;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pAppDataBuffer;
		D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
		Buffer* m_pVertexBuffer;
		Buffer* m_pIndexBuffer;
		Buffer* m_pNumVisibleMeshesPerTypeBuffer;
		UINT64 m_NumVisibleMeshesPerTypeBufferOffset;
		Buffer* m_pDrawCommandBuffer;
		UINT64 m_DrawCommandBufferOffset;
		Viewport* m_pViewport;
		u32 m_MaxNumMeshes;
	};

	RenderGBufferPass(InitParams* pParams);
	~RenderGBufferPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void InitCommandSignature(InitParams* pParams);
	void CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	CommandSignature* m_pCommandSignature;
	DescriptorHandle m_SRVHeapStartVS;
	DescriptorHandle m_RTVHeapStart;
	DescriptorHandle m_DSVHeapStart;
	std::vector<ResourceBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
};
