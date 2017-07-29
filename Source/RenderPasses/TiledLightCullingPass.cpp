#include "RenderPasses/TiledLightCullingPass.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/RenderEnv.h"

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
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	std::string tileSizeStr = std::to_string(pParams->m_TileSize);
	std::string numTilesXStr = std::to_string(pParams->m_NumTilesX);
	std::string maxNumPointLightsStr = std::to_string(pParams->m_MaxNumPointLights);
	std::string maxNumSpotLightsStr = std::to_string(pParams->m_MaxNumSpotLights);

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("TILE_SIZE", tileSizeStr.c_str()),
		ShaderMacro("NUM_TILES_X", numTilesXStr.c_str()),
		ShaderMacro("MAX_NUM_POINT_LIGHTS", maxNumPointLightsStr.c_str()),
		ShaderMacro("MAX_NUM_SPOT_LIGHTS", maxNumSpotLightsStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//TiledLightCullingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	std::vector<D3D12_DESCRIPTOR_RANGE> srvDescriptorRanges;
	srvDescriptorRanges.reserve(6);

	srvDescriptorRanges.push_back(CBVDescriptorRange(1, 0));
	srvDescriptorRanges.push_back(SRVDescriptorRange(1, 0));

	if (m_CullPointLights)
	{
		srvDescriptorRanges.push_back(SRVDescriptorRange(3, 1));
		srvDescriptorRanges.push_back(UAVDescriptorRange(3, 0));
	}
	if (m_CullSpotLights)
	{
		srvDescriptorRanges.push_back(SRVDescriptorRange(3, 4));
		srvDescriptorRanges.push_back(UAVDescriptorRange(3, 3));
	}

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = RootDescriptorTableParameter(srvDescriptorRanges.size(), srvDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"TiledLightCullingPass::m_pRootSignature");

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"TiledLightCullingPass::m_pPipelineState");
}

TiledLightCullingPass::~TiledLightCullingPass()
{
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}

void TiledLightCullingPass::Record(RenderParams* pParams)
{
	assert(false && "Kolya. Fix me");
	/*
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	ResourceList* pResources = pParams->m_pResources;
	
	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	const UINT numClearValue[4] = {0, 0, 0, 0};
	if (m_CullPointLights)
	{
		Buffer* pNumLightsPerTileBuffer = pParams->m_pNumPointLightsPerTileBuffer;
		assert(pNumLightsPerTileBuffer != nullptr);

		DescriptorHandle numLightsPerTileUAVHandle(pResources->m_SRVHeapStart, 5);
		pCommandList->ClearUnorderedAccessView(numLightsPerTileUAVHandle, pNumLightsPerTileBuffer->GetUAVHandle(), pNumLightsPerTileBuffer, numClearValue);
	}
	if (m_CullSpotLights)
	{
		Buffer* pNumLightsPerTileBuffer = pParams->m_pNumSpotLightsPerTileBuffer;
		assert(pNumLightsPerTileBuffer != nullptr);

		DescriptorHandle numLightsPerTileUAVHandle(pResources->m_SRVHeapStart, m_CullPointLights ? 11 : 5);
		pCommandList->ClearUnorderedAccessView(numLightsPerTileUAVHandle, pNumLightsPerTileBuffer->GetUAVHandle(), pNumLightsPerTileBuffer, numClearValue);
	}
	
	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);
	pCommandList->End();
	*/
}