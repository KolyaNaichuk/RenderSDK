#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class CommandList;
class RootSignature;
class PipelineState;
class CommandSignature;

class CreateFalseNegativeDrawCommandsPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_NumMeshesBufferState;
		D3D12_RESOURCE_STATES m_VisibilityBufferState;
		D3D12_RESOURCE_STATES m_InstanceIndexBufferState;
		D3D12_RESOURCE_STATES m_MeshInfoBufferState;
		D3D12_RESOURCE_STATES m_VisibleInstanceIndexBufferState;
		D3D12_RESOURCE_STATES m_NumVisibleMeshesPerTypeBufferState;
		D3D12_RESOURCE_STATES m_DrawCommandBufferState;
	};

	struct InitParams
	{
		const char* m_pName;
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		Buffer* m_pNumMeshesBuffer;
		Buffer* m_pVisibilityBuffer;
		Buffer* m_pInstanceIndexBuffer;
		Buffer* m_pMeshInfoBuffer;
		u32 m_NumMeshTypes;
		u32 m_MaxNumMeshes;
		u32 m_MaxNumInstances;
		u32 m_MaxNumInstancesPerMesh;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pNumMeshesBuffer;
	};

	CreateFalseNegativeDrawCommandsPass(InitParams* pParams);
	~CreateFalseNegativeDrawCommandsPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

	Buffer* GetVisibleInstanceIndexBuffer() { return m_pVisibleInstanceIndexBuffer; }
	Buffer* GetNumVisibleMeshesPerTypeBuffer() { return m_pNumVisibleMeshesPerTypeBuffer; }
	Buffer* GetDrawCommandBuffer() { return m_pDrawCommandBuffer; }
	
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

	DescriptorHandle m_SRVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;

	Buffer* m_pVisibleInstanceIndexBuffer = nullptr;
	Buffer* m_pNumVisibleMeshesPerTypeBuffer = nullptr;
	Buffer* m_pDrawCommandBuffer = nullptr;
	Buffer* m_pArgumentBuffer = nullptr;
};