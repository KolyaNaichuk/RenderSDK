#include "RenderPasses/TiledShadingPass.h"
#include "D3DWrapper/D3DPipelineState.h"
#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DResource.h"
#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DCommandAllocator.h"
#include "D3DWrapper/D3DRenderEnv.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

TiledShadingPass::TiledShadingPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_NumThreadGroupsX(pParams->m_NumTilesX)
	, m_NumThreadGroupsY(pParams->m_NumTilesY)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	std::string tileSizeStr = std::to_string(pParams->m_TileSize);
	std::string numTilesXStr = std::to_string(pParams->m_NumTilesX);
	std::string shadingModeStr = std::to_string(pParams->m_ShadingMode);
	std::string enablePointLightsStr = std::to_string(pParams->m_EnablePointLights ? 1 : 0);
	std::string enableSpotLightsStr = std::to_string(pParams->m_EnableSpotLights ? 1 : 0);
	std::string enableDirectionalLightStr = std::to_string(pParams->m_EnableDirectionalLight ? 1 : 0);
	
	const D3DShaderMacro shaderDefines[] =
	{
		D3DShaderMacro("TILE_SIZE", tileSizeStr.c_str()),
		D3DShaderMacro("NUM_TILES_X", numTilesXStr.c_str()),
		D3DShaderMacro("SHADING_MODE", shadingModeStr.c_str()),
		D3DShaderMacro("ENABLE_POINT_LIGHTS", enablePointLightsStr.c_str()),
		D3DShaderMacro("ENABLE_SPOT_LIGHTS", enableSpotLightsStr.c_str()),
		D3DShaderMacro("ENABLE_DIRECTIONAL_LIGHT", enableDirectionalLightStr.c_str()),
		D3DShaderMacro()
	};
	D3DShader computeShader(L"Shaders//TiledShadingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	std::vector<D3D12_DESCRIPTOR_RANGE> srvDescriptorRanges;
	srvDescriptorRanges.push_back(D3DUAVRange(1, 0));
	srvDescriptorRanges.push_back(D3DCBVRange(1, 0));
	srvDescriptorRanges.push_back(D3DSRVRange(4, 0));
	
	if (pParams->m_EnablePointLights)
		srvDescriptorRanges.push_back(D3DSRVRange(4, 4));
	if (pParams->m_EnableSpotLights)
		srvDescriptorRanges.push_back(D3DSRVRange(4, 8));
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = D3DRootDescriptorTableParameter(srvDescriptorRanges.size(), srvDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_ALL);

	D3DRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new D3DRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"TiledShadingPass::m_pRootSignature");

	D3DComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new D3DPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"TiledShadingPass::m_pPipelineState");
}

TiledShadingPass::~TiledShadingPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void TiledShadingPass::Record(RenderParams* pParams)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	D3DCommandList* pCommandList = pParams->m_pCommandList;
	D3DResourceList* pResources = pParams->m_pResources;
	D3DColorTexture* pAccumLightTexture = pParams->m_pAccumLightTexture;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetResourceTransitions(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	const FLOAT accumLightClearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	pCommandList->ClearUnorderedAccessView(pResources->m_SRVHeapStart, pAccumLightTexture->GetUAVHandle(), pAccumLightTexture, accumLightClearColor);

	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);
	pCommandList->Close();
}
