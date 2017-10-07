#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class RootSignature;
class PipelineState;
class CommandList;

enum ShadingMode
{
	ShadingMode_Phong = 1,
	ShadingMode_BlinnPhong = 2
};

class TiledShadingPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_AccumLightTextureState;
		D3D12_RESOURCE_STATES m_MeshTypeDepthTextureState;
		D3D12_RESOURCE_STATES m_ShadingRectangleMinPointBufferState;
		D3D12_RESOURCE_STATES m_ShadingRectangleMaxPointBufferState;		
		D3D12_RESOURCE_STATES m_DepthTextureState;
		D3D12_RESOURCE_STATES m_TexCoordTextureState;
		D3D12_RESOURCE_STATES m_NormalTextureState;
		D3D12_RESOURCE_STATES m_MaterialIDTextureState;
		D3D12_RESOURCE_STATES m_FirstResourceIndexPerMaterialIDBufferState;
		
		D3D12_RESOURCE_STATES m_PointLightBoundsBufferState;
		D3D12_RESOURCE_STATES m_PointLightPropsBufferState;
		D3D12_RESOURCE_STATES m_PointLightIndexPerTileBufferState;
		D3D12_RESOURCE_STATES m_PointLightRangePerTileBufferState;

		D3D12_RESOURCE_STATES m_SpotLightBoundsBufferState;
		D3D12_RESOURCE_STATES m_SpotLightPropsBufferState;
		D3D12_RESOURCE_STATES m_SpotLightIndexPerTileBufferState;
		D3D12_RESOURCE_STATES m_SpotLightRangePerTileBufferState;

		D3D12_RESOURCE_STATES m_IntensityRCoeffsTextureState;
		D3D12_RESOURCE_STATES m_IntensityGCoeffsTextureState;
		D3D12_RESOURCE_STATES m_IntensityBCoeffsTextureState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		ShadingMode m_ShadingMode;
		
		ColorTexture* m_pAccumLightTexture;
		DepthTexture* m_pMeshTypeDepthTexture;
		Buffer* m_pShadingRectangleMinPointBuffer;
		Buffer* m_pShadingRectangleMaxPointBuffer;
		DepthTexture* m_pDepthTexture;
		ColorTexture* m_pTexCoordTexture;
		ColorTexture* m_pNormalTexture;
		ColorTexture* m_pMaterialIDTexture;
		Buffer* m_pFirstResourceIndexPerMaterialIDBuffer;
		bool m_EnableDirectionalLight;

		u16 m_NumMaterialTextures;
		ColorTexture** m_ppMaterialTextures;
		
		bool m_EnablePointLights;
		Buffer* m_pPointLightBoundsBuffer;
		Buffer* m_pPointLightPropsBuffer;
		Buffer* m_pPointLightIndexPerTileBuffer;
		Buffer* m_pPointLightRangePerTileBuffer;

		bool m_EnableSpotLights;
		Buffer* m_pSpotLightBoundsBuffer;
		Buffer* m_pSpotLightPropsBuffer;
		Buffer* m_pSpotLightIndexPerTileBuffer;
		Buffer* m_pSpotLightRangePerTileBuffer;

		bool m_EnableIndirectLight;
		ColorTexture* m_pIntensityRCoeffsTexture;
		ColorTexture* m_pIntensityGCoeffsTexture;
		ColorTexture* m_pIntensityBCoeffsTexture;
	};
	
	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
	};
	
	TiledShadingPass(InitParams* pParams);
	~TiledShadingPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	DescriptorHandle m_SRVHeapStartVS;
	DescriptorHandle m_SRVHeapStartPS;
	DescriptorHandle m_SamplerHeapStartPS;
	DescriptorHandle m_RTVHeapStart;
	DescriptorHandle m_DSVHeapStart;
	std::vector<ResourceBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
};