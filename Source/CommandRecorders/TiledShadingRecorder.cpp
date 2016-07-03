#include "CommandRecorders/TiledShadingRecorder.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXResource.h"
#include "DX/DXCommandList.h"
#include "DX/DXCommandAllocator.h"
#include "DX/DXRenderEnvironment.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

TiledShadingRecorder::TiledShadingRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_NumThreadGroupsX(pParams->m_NumTilesX)
	, m_NumThreadGroupsY(pParams->m_NumTilesY)
{
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;

	std::string tileSizeStr = std::to_string(pParams->m_TileSize);
	std::string numTilesXStr = std::to_string(pParams->m_NumTilesX);
	std::string shadingModeStr = std::to_string(pParams->m_ShadingMode);
	std::string enablePointLightsStr = std::to_string(pParams->m_EnablePointLights ? 1 : 0);
	std::string enableSpotLightsStr = std::to_string(pParams->m_EnableSpotLights ? 1 : 0);
	std::string enableDirectionalLightStr = std::to_string(pParams->m_EnableDirectionalLight ? 1 : 0);
	
	const DXShaderMacro shaderDefines[] =
	{
		DXShaderMacro("TILE_SIZE", tileSizeStr.c_str()),
		DXShaderMacro("NUM_TILES_X", numTilesXStr.c_str()),
		DXShaderMacro("SHADING_MODE", shadingModeStr.c_str()),
		DXShaderMacro("ENABLE_POINT_LIGHTS", enablePointLightsStr.c_str()),
		DXShaderMacro("ENABLE_SPOT_LIGHTS", enableSpotLightsStr.c_str()),
		DXShaderMacro("ENABLE_DIRECTIONAL_LIGHT", enableDirectionalLightStr.c_str()),
		DXShaderMacro()
	};
	DXShader computeShader(L"Shaders//TiledShadingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	std::vector<D3D12_DESCRIPTOR_RANGE> srvDescriptorRanges;
	srvDescriptorRanges.push_back(DXUAVRange(1, 0));
	srvDescriptorRanges.push_back(DXCBVRange(1, 0));
	srvDescriptorRanges.push_back(DXSRVRange(4, 0));
	
	if (pParams->m_EnablePointLights)
		srvDescriptorRanges.push_back(DXSRVRange(4, 4));
	if (pParams->m_EnableSpotLights)
		srvDescriptorRanges.push_back(DXSRVRange(4, 8));
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = DXRootDescriptorTableParameter(srvDescriptorRanges.size(), srvDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_ALL);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new DXRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"TiledShadingRecorder::m_pRootSignature");

	DXComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new DXPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"TiledShadingRecorder::m_pPipelineState");
}

TiledShadingRecorder::~TiledShadingRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void TiledShadingRecorder::Record(RenderPassParams* pParams)
{
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;
	DXCommandList* pCommandList = pParams->m_pCommandList;
	DXBindingResourceList* pResources = pParams->m_pResources;
	DXColorTexture* pAccumLightTexture = pParams->m_pAccumLightTexture;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetResourceTransitions(&pResources->m_ResourceTransitions);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	const FLOAT accumLightClearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	pCommandList->ClearUnorderedAccessView(pResources->m_SRVHeapStart, pAccumLightTexture->GetUAVHandle(), pAccumLightTexture, accumLightClearColor);

	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);
	pCommandList->Close();
}
