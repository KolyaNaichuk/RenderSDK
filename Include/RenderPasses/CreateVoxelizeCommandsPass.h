#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Viewport;
class CommandList;
class CommandSignature;
class RootSignature;
class PipelineState;

class CreateVoxelizeCommandsPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_NumMeshesBufferState;
		D3D12_RESOURCE_STATES m_MeshInfoBufferState;
		D3D12_RESOURCE_STATES m_NumCommandsPerMeshTypeBufferState;
		D3D12_RESOURCE_STATES m_VoxelizeCommandBufferState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		Buffer* m_pNumMeshesBuffer;
		Buffer* m_pMeshInfoBuffer;
		u32 m_NumMeshTypes;
		u32 m_MaxNumMeshes;
	};
	
	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
	};

	CreateVoxelizeCommandsPass(InitParams* pParams);
	~CreateVoxelizeCommandsPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

	Buffer* GetNumCommandsPerMeshTypeBuffer() { return m_pNumCommandsPerMeshTypeBuffer; }
	Buffer* GetVoxelizeCommandBuffer() { return m_pVoxelizeCommandBuffer; }

private:
	void InitSetArgumentsResources(InitParams* pParams);
	void InitSetArgumentsRootSignature(InitParams* pParams);
	void InitSetArgumentsPipelineState(InitParams* pParams);
	void AddSetArgumentsResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

	void InitCreateCommandsResources(InitParams* pParams);
	void InitCreateCommandsRootSignature(InitParams* pParams);
	void InitCreateCommandsPipelineState(InitParams* pParams);
	void InitCreateCommandsCommandSignature(InitParams* pParams);
	void AddCreateCommandsResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);
			
private:
	RootSignature* m_pSetArgumentsRootSignature;
	PipelineState* m_pSetArgumentsPipelineState;
	Buffer* m_pArgumentBuffer;
	std::vector<ResourceBarrier> m_SetArgumentsResourceBarriers;
	DescriptorHandle m_SetArgumentsSRVHeapStart;

	RootSignature* m_pCreateCommandsRootSignature;
	PipelineState* m_pCreateCommandsPipelineState;
	CommandSignature* m_pCreateCommandsCommandSignature;
	Buffer* m_pNumCommandsPerMeshTypeBuffer;
	Buffer* m_pVoxelizeCommandBuffer;
	std::vector<ResourceBarrier> m_CreateCommandsResourceBarriers;
	DescriptorHandle m_CreateCommandsSRVHeapStart;

	ResourceStates m_OutputResourceStates;
};