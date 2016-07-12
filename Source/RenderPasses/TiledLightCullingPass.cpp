#include "RenderPasses/TiledLightCullingPass.h"
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

TiledLightCullingPass::TiledLightCullingPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_NumThreadGroupsX(pParams->m_NumTilesX)
	, m_NumThreadGroupsY(pParams->m_NumTilesY)
	, m_CullPointLights(pParams->m_MaxNumPointLights > 0)
	, m_CullSpotLights(pParams->m_MaxNumSpotLights > 0)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	std::string tileSizeStr = std::to_string(pParams->m_TileSize);
	std::string numTilesXStr = std::to_string(pParams->m_NumTilesX);
	std::string maxNumPointLightsStr = std::to_string(pParams->m_MaxNumPointLights);
	std::string maxNumSpotLightsStr = std::to_string(pParams->m_MaxNumSpotLights);

	const D3DShaderMacro shaderDefines[] =
	{
		D3DShaderMacro("TILE_SIZE", tileSizeStr.c_str()),
		D3DShaderMacro("NUM_TILES_X", numTilesXStr.c_str()),
		D3DShaderMacro("MAX_NUM_POINT_LIGHTS", maxNumPointLightsStr.c_str()),
		D3DShaderMacro("MAX_NUM_SPOT_LIGHTS", maxNumSpotLightsStr.c_str()),
		D3DShaderMacro()
	};
	D3DShader computeShader(L"Shaders//TiledLightCullingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	std::vector<D3D12_DESCRIPTOR_RANGE> srvDescriptorRanges;
	srvDescriptorRanges.reserve(6);

	srvDescriptorRanges.push_back(D3DCBVRange(1, 0));
	srvDescriptorRanges.push_back(D3DSRVRange(1, 0));

	if (m_CullPointLights)
	{
		srvDescriptorRanges.push_back(D3DSRVRange(3, 1));
		srvDescriptorRanges.push_back(D3DUAVRange(3, 0));
	}
	if (m_CullSpotLights)
	{
		srvDescriptorRanges.push_back(D3DSRVRange(3, 4));
		srvDescriptorRanges.push_back(D3DUAVRange(3, 3));
	}

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = D3DRootDescriptorTableParameter(srvDescriptorRanges.size(), srvDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_ALL);

	D3DRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new D3DRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"TiledLightCullingPass::m_pRootSignature");

	D3DComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new D3DPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"TiledLightCullingPass::m_pPipelineState");
}

TiledLightCullingPass::~TiledLightCullingPass()
{
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}

void TiledLightCullingPass::Record(RenderParams* pParams)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	D3DCommandList* pCommandList = pParams->m_pCommandList;
	D3DResourceList* pResources = pParams->m_pResources;
	
	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetResourceTransitions(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	const UINT numClearValue[4] = {0, 0, 0, 0};
	if (m_CullPointLights)
	{
		D3DBuffer* pNumLightsPerTileBuffer = pParams->m_pNumPointLightsPerTileBuffer;
		assert(pNumLightsPerTileBuffer != nullptr);

		D3DDescriptorHandle numLightsPerTileUAVHandle(pResources->m_SRVHeapStart, 5);
		pCommandList->ClearUnorderedAccessView(numLightsPerTileUAVHandle, pNumLightsPerTileBuffer->GetUAVHandle(), pNumLightsPerTileBuffer, numClearValue);
	}
	if (m_CullSpotLights)
	{
		D3DBuffer* pNumLightsPerTileBuffer = pParams->m_pNumSpotLightsPerTileBuffer;
		assert(pNumLightsPerTileBuffer != nullptr);

		D3DDescriptorHandle numLightsPerTileUAVHandle(pResources->m_SRVHeapStart, m_CullPointLights ? 11 : 5);
		pCommandList->ClearUnorderedAccessView(numLightsPerTileUAVHandle, pNumLightsPerTileBuffer->GetUAVHandle(), pNumLightsPerTileBuffer, numClearValue);
	}
	
	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);
	pCommandList->Close();
}