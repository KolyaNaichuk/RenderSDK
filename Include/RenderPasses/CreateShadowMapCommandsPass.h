#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class CommandList;
class CommandSignature;
class RootSignature;
class PipelineState;

class CreateShadowMapCommandsPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_MeshInfoBufferState;
		D3D12_RESOURCE_STATES m_MeshInstanceWorldAABBBufferState;
		D3D12_RESOURCE_STATES m_MeshInstanceIndexBufferState;
		D3D12_RESOURCE_STATES m_NumMeshesBufferState;

		D3D12_RESOURCE_STATES m_PointLightWorldBoundsBufferState;
		D3D12_RESOURCE_STATES m_NumPointLightMeshInstancesBufferState;
		D3D12_RESOURCE_STATES m_PointLightIndexForMeshInstanceBufferState;
		D3D12_RESOURCE_STATES m_MeshInstanceIndexForPointLightBufferState;
		D3D12_RESOURCE_STATES m_NumPointLightCommandsBufferState;
		D3D12_RESOURCE_STATES m_PointLightCommandBufferState;

		D3D12_RESOURCE_STATES m_SpotLightWorldBoundsBufferState;
		D3D12_RESOURCE_STATES m_NumSpotLightMeshInstancesBufferState;
		D3D12_RESOURCE_STATES m_SpotLightIndexForMeshInstanceBufferState;
		D3D12_RESOURCE_STATES m_MeshInstanceIndexForSpotLightBufferState;
		D3D12_RESOURCE_STATES m_NumSpotLightCommandsBufferState;
		D3D12_RESOURCE_STATES m_SpotLightCommandBufferState;
	};
	
	struct InitParams
	{
		const char* m_pName;
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;

		u32 m_MaxNumMeshes;
		u32 m_MaxNumInstances;
		u32 m_MaxNumInstancesPerMesh;
		Buffer* m_pMeshInfoBuffer;
		Buffer* m_pMeshInstanceWorldAABBBuffer;
		Buffer* m_pMeshInstanceIndexBuffer;
		Buffer* m_pNumMeshesBuffer;

		u32 m_MaxNumPointLights;
		Buffer* m_pPointLightWorldBoundsBuffer;
		
		u32 m_MaxNumSpotLights;
		Buffer* m_pSpotLightWorldBoundsBuffer;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pNumMeshesBuffer;
		u32 m_NumPointLights;
		u32 m_NumSpotLights;
	};

	CreateShadowMapCommandsPass(InitParams* pParams);
	~CreateShadowMapCommandsPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

	Buffer* GetPointLightIndexForMeshInstanceBuffer() { return m_pPointLightIndexForMeshInstanceBuffer; }
	Buffer* GetMeshInstanceIndexForPointLightBuffer() { return m_pMeshInstanceIndexForPointLightBuffer; }
	Buffer* GetNumPointLightCommandsBuffer() { return m_pNumPointLightCommandsBuffer; }
	Buffer* GetPointLightCommandBuffer() { return m_pPointLightCommandBuffer; }

	Buffer* GetSpotLightIndexForMeshInstanceBuffer() { return m_pSpotLightIndexForMeshInstanceBuffer; }
	Buffer* GetMeshInstanceIndexForSpotLightBuffer() { return m_pMeshInstanceIndexForSpotLightBuffer; }
	Buffer* GetNumSpotLightCommandsBuffer() { return m_pNumSpotLightCommandsBuffer; }
	Buffer* GetSpotLightCommandBuffer() { return m_pSpotLightCommandBuffer; }

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

	Buffer* m_pNumPointLightMeshInstancesBuffer = nullptr;
	DescriptorHandle m_NumPointLightMeshInstancesBufferUAV;
	Buffer* m_pPointLightIndexForMeshInstanceBuffer = nullptr;
	Buffer* m_pMeshInstanceIndexForPointLightBuffer = nullptr;
	Buffer* m_pNumPointLightCommandsBuffer = nullptr;
	DescriptorHandle m_NumPointLightCommandsBufferUAV;
	Buffer* m_pPointLightCommandBuffer = nullptr;

	Buffer* m_pNumSpotLightMeshInstancesBuffer = nullptr;
	DescriptorHandle m_NumSpotLightMeshInstancesBufferUAV;
	Buffer* m_pSpotLightIndexForMeshInstanceBuffer = nullptr;
	Buffer* m_pMeshInstanceIndexForSpotLightBuffer = nullptr;
	Buffer* m_pNumSpotLightCommandsBuffer = nullptr;
	DescriptorHandle m_NumSpotLightCommandsBufferUAV;
	Buffer* m_pSpotLightCommandBuffer = nullptr;

	Buffer* m_pArgumentBuffer = nullptr;
};
