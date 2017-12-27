#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;

class CommandList;
class RootSignature;
class PipelineState;

class FrustumMeshCullingPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_MeshInfoBufferState;
		D3D12_RESOURCE_STATES m_InstanceWorldAABBBufferState;
		D3D12_RESOURCE_STATES m_NumVisibleMeshesBufferState;
		D3D12_RESOURCE_STATES m_VisibleMeshInfoBufferState;
		D3D12_RESOURCE_STATES m_NumVisibleInstancesBufferState;
		D3D12_RESOURCE_STATES m_VisibleInstanceIndexBufferState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		Buffer* m_pInstanceWorldAABBBuffer;
		Buffer* m_pMeshInfoBuffer;
		u32 m_MaxNumMeshes;
		u32 m_MaxNumInstances;
		u32 m_MaxNumInstancesPerMesh;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pAppDataBuffer;
	};

	FrustumMeshCullingPass(InitParams* pParams);
	~FrustumMeshCullingPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

	Buffer* GetNumVisibleMeshesBuffer() { return m_pNumVisibleMeshesBuffer; }
	Buffer* GetVisibleMeshInfoBuffer() { return m_pVisibleMeshInfoBuffer; }
	Buffer* GetNumVisibleInstancesBuffer() { return m_pNumVisibleInstancesBuffer; }
	Buffer* GetVisibleInstanceIndexBuffer() { return m_pVisibleInstanceIndexBuffer; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	DescriptorHandle m_SRVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;

	Buffer* m_pNumVisibleMeshesBuffer;
	Buffer* m_pNumVisibleInstancesBuffer;
	Buffer* m_pVisibleMeshInfoBuffer;
	Buffer* m_pVisibleInstanceIndexBuffer;
	u32 m_MaxNumMeshes;
};