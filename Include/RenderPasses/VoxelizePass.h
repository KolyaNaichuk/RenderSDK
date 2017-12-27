#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Viewport;
class CommandList;
class RootSignature;
class PipelineState;
class CommandSignature;
class MeshRenderResources;

class VoxelizePass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_NumCommandsPerMeshTypeBufferState;
		D3D12_RESOURCE_STATES m_VoxelizeCommandBufferState;
		D3D12_RESOURCE_STATES m_InstanceIndexBufferState;
		D3D12_RESOURCE_STATES m_InstanceWorldMatrixBufferState;
		D3D12_RESOURCE_STATES m_VoxelReflectanceTextureState;
		D3D12_RESOURCE_STATES m_FirstResourceIndexPerMaterialIDBufferState;
		D3D12_RESOURCE_STATES m_PointLightWorldBoundsBufferState;
		D3D12_RESOURCE_STATES m_PointLightPropsBufferState;
		D3D12_RESOURCE_STATES m_SpotLightWorldBoundsBufferState;
		D3D12_RESOURCE_STATES m_SpotLightPropsBufferState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		MeshRenderResources* m_pMeshRenderResources;

		u16 m_NumVoxelsX;
		u16 m_NumVoxelsY;
		u16 m_NumVoxelsZ;

		u16 m_NumMaterialTextures;
		ColorTexture** m_ppMaterialTextures;
		Buffer* m_pFirstResourceIndexPerMaterialIDBuffer;

		Buffer* m_pNumCommandsPerMeshTypeBuffer;
		Buffer* m_pVoxelizeCommandBuffer;
		Buffer* m_pInstanceIndexBuffer;
		Buffer* m_pInstanceWorldMatrixBuffer;
				
		bool m_EnableDirectionalLight;
		bool m_EnablePointLights;
		Buffer* m_pPointLightWorldBoundsBuffer;
		Buffer* m_pPointLightPropsBuffer;
		bool m_EnableSpotLights;
		Buffer* m_pSpotLightWorldBoundsBuffer;
		Buffer* m_pSpotLightPropsBuffer;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		MeshRenderResources* m_pMeshRenderResources;
		Buffer* m_pNumCommandsPerMeshTypeBuffer;
		Buffer* m_pVoxelizeCommandBuffer;
		Buffer* m_pAppDataBuffer;
		u32 m_NumPointLights;
		u32 m_NumSpotLights;
	};

	VoxelizePass(InitParams* pParams);
	~VoxelizePass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	ColorTexture* GetVoxelReflectanceTexture() { return m_pVoxelReflectanceTexture; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void InitCommandSignature(InitParams* pParams);
	void AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	CommandSignature* m_pCommandSignature;
	DescriptorHandle m_SRVHeapStartVS;
	DescriptorHandle m_SRVHeapStartPS;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
	ColorTexture* m_pVoxelReflectanceTexture;
	Viewport* m_pViewport;
};