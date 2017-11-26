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
		D3D12_RESOURCE_STATES m_InstanceIndexBufferState;
		D3D12_RESOURCE_STATES m_InstanceWorldMatrixBufferState;
		D3D12_RESOURCE_STATES m_VoxelReflectanceTextureState;
		D3D12_RESOURCE_STATES m_FirstResourceIndexPerMaterialIDBufferState;
		D3D12_RESOURCE_STATES m_PointLightBoundsBufferState;
		D3D12_RESOURCE_STATES m_PointLightPropsBufferState;
		D3D12_RESOURCE_STATES m_NumPointLightsBufferState;
		D3D12_RESOURCE_STATES m_PointLightIndexBufferState;
		D3D12_RESOURCE_STATES m_SpotLightWorldBoundsBufferState;
		D3D12_RESOURCE_STATES m_SpotLightPropsBufferState;
		D3D12_RESOURCE_STATES m_NumSpotLightsBufferState;
		D3D12_RESOURCE_STATES m_SpotLightIndexBufferState;
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

		Buffer* m_pInstanceIndexBuffer;
		Buffer* m_pInstanceWorldMatrixBuffer;
		Buffer* m_pFirstResourceIndexPerMaterialIDBuffer;
		
		bool m_EnableDirectionalLight;
		
		bool m_EnablePointLights;
		Buffer* m_pPointLightBoundsBuffer;
		Buffer* m_pPointLightPropsBuffer;
		Buffer* m_pNumPointLightsBuffer;
		Buffer* m_pPointLightIndexBuffer;

		bool m_EnableSpotLights;
		Buffer* m_pSpotLightWorldBoundsBuffer;
		Buffer* m_pSpotLightPropsBuffer;
		Buffer* m_pNumSpotLightsBuffer;
		Buffer* m_pSpotLightIndexBuffer;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
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
	std::vector<ResourceBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
	ColorTexture* m_pVoxelReflectanceTexture;
};