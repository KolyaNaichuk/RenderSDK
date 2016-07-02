#include "CommandRecorders/TiledLightCullingRecorder.h"
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

TiledLightCullingRecorder::TiledLightCullingRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_NumThreadGroupsX(pParams->m_NumTilesX)
	, m_NumThreadGroupsY(pParams->m_NumTilesY)
	, m_CullPointLights(pParams->m_MaxNumPointLights > 0)
	, m_CullSpotLights(pParams->m_MaxNumSpotLights > 0)
{
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;

	std::string tileSizeStr = std::to_string(pParams->m_TileSize);
	std::string numTilesXStr = std::to_string(pParams->m_NumTilesX);
	std::string maxNumPointLightsStr = std::to_string(pParams->m_MaxNumPointLights);
	std::string maxNumSpotLightsStr = std::to_string(pParams->m_MaxNumSpotLights);

	const DXShaderMacro shaderDefines[] =
	{
		DXShaderMacro("TILE_SIZE", tileSizeStr.c_str()),
		DXShaderMacro("NUM_TILES_X", numTilesXStr.c_str()),
		DXShaderMacro("MAX_NUM_POINT_LIGHTS", maxNumPointLightsStr.c_str()),
		DXShaderMacro("MAX_NUM_SPOT_LIGHTS", maxNumSpotLightsStr.c_str()),
		DXShaderMacro()
	};
	DXShader computeShader(L"Shaders//TiledLightCullingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	std::vector<D3D12_DESCRIPTOR_RANGE> srvDescriptorRanges;
	srvDescriptorRanges.reserve(6);

	srvDescriptorRanges.push_back(DXCBVRange(1, 0));
	srvDescriptorRanges.push_back(DXSRVRange(1, 0));

	if (m_CullPointLights)
	{
		srvDescriptorRanges.push_back(DXSRVRange(3, 1));
		srvDescriptorRanges.push_back(DXUAVRange(3, 0));
	}
	if (m_CullSpotLights)
	{
		srvDescriptorRanges.push_back(DXSRVRange(3, 4));
		srvDescriptorRanges.push_back(DXUAVRange(3, 3));
	}

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = DXRootDescriptorTableParameter(srvDescriptorRanges.size(), srvDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_ALL);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new DXRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"TiledLightCullingRecorder::m_pRootSignature");

	DXComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new DXPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"TiledLightCullingRecorder::m_pPipelineState");
}

TiledLightCullingRecorder::~TiledLightCullingRecorder()
{
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}

void TiledLightCullingRecorder::Record(RenderPassParams* pParams)
{
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;
	DXCommandList* pCommandList = pParams->m_pCommandList;
	DXBindingResourceList* pResources = pParams->m_pResources;
	
	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetResourceTransitions(&pResources->m_ResourceTransitions);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	const UINT numClearValue[4] = {0, 0, 0, 0};
	if (m_CullPointLights)
	{
		DXBuffer* pNumLightsPerTileBuffer = pParams->m_pNumPointLightsPerTileBuffer;
		assert(pNumLightsPerTileBuffer != nullptr);

		DXDescriptorHandle numLightsPerTileUAVHandle(pResources->m_SRVHeapStart, 5);
		pCommandList->ClearUnorderedAccessView(numLightsPerTileUAVHandle, pNumLightsPerTileBuffer->GetUAVHandle(), pNumLightsPerTileBuffer, numClearValue);
	}
	if (m_CullSpotLights)
	{
		DXBuffer* pNumLightsPerTileBuffer = pParams->m_pNumSpotLightsPerTileBuffer;
		assert(pNumLightsPerTileBuffer != nullptr);

		DXDescriptorHandle numLightsPerTileUAVHandle(pResources->m_SRVHeapStart, m_CullPointLights ? 11 : 5);
		pCommandList->ClearUnorderedAccessView(numLightsPerTileUAVHandle, pNumLightsPerTileBuffer->GetUAVHandle(), pNumLightsPerTileBuffer, numClearValue);
	}
	
	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);
	pCommandList->Close();
}