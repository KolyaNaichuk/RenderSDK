#include "CommandRecorders/TiledShadingRecorder.h"
#include "CommandRecorders/FillGBufferRecorder.h"
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

	const u16 tileSize = 16;
	std::string tileSizeStr = std::to_string(tileSize);
	std::string shadingModeStr = std::to_string(pParams->m_ShadingMode);
	std::string numPointLightsStr = std::to_string(pParams->m_NumPointLights);
	std::string numSpotLightsStr = std::to_string(pParams->m_NumSpotLights);
	std::string useDirectionalLightStr = std::to_string(pParams->m_UseDirectionalLight ? 1 : 0);
	
	const DXShaderMacro shaderDefines[] =
	{
		DXShaderMacro("TILE_SIZE", tileSizeStr.c_str()),
		DXShaderMacro("SHADING_MODE", shadingModeStr.c_str()),
		DXShaderMacro("NUM_POINT_LIGHTS", numPointLightsStr.c_str()),
		DXShaderMacro("NUM_SPOT_LIGHTS", numSpotLightsStr.c_str()),
		DXShaderMacro("USE_DIRECTIONAL_LIGHT", useDirectionalLightStr.c_str()),
		DXShaderMacro()
	};
	DXShader computeShader(L"Shaders//TiledShadingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	std::vector<D3D12_DESCRIPTOR_RANGE> srvDescriptorRanges;
	srvDescriptorRanges.reserve(5);

	srvDescriptorRanges.push_back(DXUAVRange(1, 0));
	srvDescriptorRanges.push_back(DXCBVRange(1, 0));
	srvDescriptorRanges.push_back(DXSRVRange(4, 0));
	
	if (pParams->m_NumPointLights > 0)
		srvDescriptorRanges.push_back(DXSRVRange(2, 4));
	if (pParams->m_NumSpotLights > 0)
		srvDescriptorRanges.push_back(DXSRVRange(2, 6));
	
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
