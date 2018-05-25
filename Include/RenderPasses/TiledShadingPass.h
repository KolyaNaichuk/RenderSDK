#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Viewport;
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
		D3D12_RESOURCE_STATES m_GBuffer1State;
		D3D12_RESOURCE_STATES m_GBuffer2State;
		D3D12_RESOURCE_STATES m_GBuffer3State;
		D3D12_RESOURCE_STATES m_GBuffer4State;
		D3D12_RESOURCE_STATES m_FirstResourceIndexPerMaterialIDBufferState;
		D3D12_RESOURCE_STATES m_SpotLightPropsBufferState;
		D3D12_RESOURCE_STATES m_SpotLightIndexPerTileBufferState;
		D3D12_RESOURCE_STATES m_SpotLightRangePerTileBufferState;
		D3D12_RESOURCE_STATES m_SpotLightShadowMapsState;
		D3D12_RESOURCE_STATES m_SpotLightViewProjMatrixBufferState;
	};

	struct InitParams
	{
		const char* m_pName;
		RenderEnv* m_pRenderEnv;
		ShadingMode m_ShadingMode;
		ResourceStates m_InputResourceStates;
				
		ColorTexture* m_pAccumLightTexture;
		DepthTexture* m_pMeshTypeDepthTexture;
		Buffer* m_pShadingRectangleMinPointBuffer;
		Buffer* m_pShadingRectangleMaxPointBuffer;
		DepthTexture* m_pDepthTexture;
		ColorTexture* m_pGBuffer1;
		ColorTexture* m_pGBuffer2;
		ColorTexture* m_pGBuffer3;
		ColorTexture* m_pGBuffer4;
		Buffer* m_pFirstResourceIndexPerMaterialIDBuffer;
		u16 m_NumMaterialTextures;
		ColorTexture** m_ppMaterialTextures;

		bool m_EnableDirectionalLight;
		bool m_EnableSpotLights;
		Buffer* m_pSpotLightPropsBuffer;
		Buffer* m_pSpotLightIndexPerTileBuffer;
		Buffer* m_pSpotLightRangePerTileBuffer;
		ColorTexture* m_pSpotLightShadowMaps;
		Buffer* m_pSpotLightViewProjMatrixBuffer;
	};
	
	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pAppDataBuffer;
		Viewport* m_pViewport;
	};
	
	TiledShadingPass(InitParams* pParams);
	~TiledShadingPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	std::string m_Name;
	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineState = nullptr;
	DescriptorHandle m_SRVHeapStartVS;
	DescriptorHandle m_SRVHeapStartPS;
	DescriptorHandle m_SamplerHeapStartPS;
	DescriptorHandle m_RTVHeapStart;
	DescriptorHandle m_DSVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
};