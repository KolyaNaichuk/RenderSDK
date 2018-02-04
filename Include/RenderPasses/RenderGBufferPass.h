#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Viewport;
class CommandList;
class CommandSignature;
class RootSignature;
class PipelineState;
class MeshRenderResources;

class RenderGBufferPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_TexCoordTextureState;
		D3D12_RESOURCE_STATES m_NormalTextureState;
		D3D12_RESOURCE_STATES m_MaterialIDTextureState;
		D3D12_RESOURCE_STATES m_DepthTextureState;
		D3D12_RESOURCE_STATES m_InstanceIndexBufferState;
		D3D12_RESOURCE_STATES m_InstanceWorldMatrixBufferState;
		D3D12_RESOURCE_STATES m_NumVisibleMeshesPerTypeBufferState;
		D3D12_RESOURCE_STATES m_DrawCommandBufferState;
	};

	struct InitParams
	{
		const char* m_pName;
		RenderEnv* m_pRenderEnv;
		UINT m_BufferWidth;
		UINT m_BufferHeight;
		ResourceStates m_InputResourceStates;
		MeshRenderResources* m_pMeshRenderResources;
		ColorTexture* m_pTexCoordTexture;
		ColorTexture* m_pNormalTexture;
		ColorTexture* m_pMaterialIDTexture;
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
		MeshRenderResources* m_pMeshRenderResources;
		Buffer* m_pNumVisibleMeshesPerTypeBuffer;
		Buffer* m_pDrawCommandBuffer;
		Viewport* m_pViewport;
		bool m_ClearGBufferBeforeRendering;
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
	void AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	std::string m_Name;
	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineState = nullptr;
	CommandSignature* m_pCommandSignature = nullptr;
	DescriptorHandle m_SRVHeapStartVS;
	DescriptorHandle m_RTVHeapStart;
	DescriptorHandle m_DSVHeapStart;
	ResourceStates m_OutputResourceStates;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
};
