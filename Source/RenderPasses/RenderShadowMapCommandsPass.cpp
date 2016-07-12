#include "RenderPasses/RenderShadowMapCommandsPass.h"
#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DPipelineState.h"
#include "D3DWrapper/D3DRenderEnv.h"
#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DCommandSignature.h"
#include "D3DWrapper/D3DResource.h"
#include "Math/Math.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

RenderShadowMapCommandsPass::RenderShadowMapCommandsPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pCommandSignature(nullptr)
	, m_EnablePointLights(pParams->m_EnablePointLights)
	, m_EnableSpotLights(pParams->m_EnableSpotLights)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const u16 threadGroupSize = 64;
	std::string threadGroupSizeStr = std::to_string(threadGroupSize);
	std::string enablePointLightsStr = std::to_string(pParams->m_EnablePointLights ? 1 : 0);
	std::string maxNumPointLightsPerShadowCasterStr = std::to_string(pParams->m_MaxNumPointLightsPerShadowCaster);
	std::string enableSpotLightsStr = std::to_string(pParams->m_EnableSpotLights ? 1 : 0);
	std::string maxNumSpotLightsPerShadowCasterStr = std::to_string(pParams->m_MaxNumSpotLightsPerShadowCaster);

	const D3DShaderMacro shaderDefines[] =
	{
		D3DShaderMacro("THREAD_GROUP_SIZE", threadGroupSizeStr.c_str()),
		D3DShaderMacro("ENABLE_POINT_LIGHTS", enablePointLightsStr.c_str()),
		D3DShaderMacro("MAX_NUM_POINT_LIGHTS_PER_SHADOW_CASTER", maxNumPointLightsPerShadowCasterStr.c_str()),
		D3DShaderMacro("ENABLE_SPOT_LIGHTS", enableSpotLightsStr.c_str()),
		D3DShaderMacro("MAX_NUM_SPOT_LIGHTS_PER_SHADOW_CASTER", maxNumSpotLightsPerShadowCasterStr.c_str()),
		D3DShaderMacro()
	};
	D3DShader computeShader(L"Shaders//RenderShadowMapCommandsCS.hlsl", "Main", "cs_5_0", shaderDefines);

	std::vector<D3D12_DESCRIPTOR_RANGE> srvDescriptorRanges;
	srvDescriptorRanges.push_back(D3DSRVRange(3, 0));

	if (m_EnablePointLights)
	{
		srvDescriptorRanges.push_back(D3DSRVRange(3, 3));
		srvDescriptorRanges.push_back(D3DUAVRange(4, 0));
	}
	if (m_EnableSpotLights)
	{
		srvDescriptorRanges.push_back(D3DSRVRange(3, 6));
		srvDescriptorRanges.push_back(D3DUAVRange(4, 4));
	}
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = D3DRootDescriptorTableParameter(srvDescriptorRanges.size(), &srvDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	D3DRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new D3DRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"RenderShadowMapCommandsPass::m_pRootSignature");

	D3DComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new D3DPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RenderShadowMapCommandsPass::m_pPipelineState");

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] = {D3DDispatchArgument()};
	D3DCommandSignatureDesc commandSignatureDesc(sizeof(D3D12_DISPATCH_ARGUMENTS), ARRAYSIZE(argumentDescs), &argumentDescs[0]);
	m_pCommandSignature = new D3DCommandSignature(pRenderEnv->m_pDevice, nullptr, &commandSignatureDesc, L"RenderShadowMapCommandsPass::m_pCommandSignature");
}

RenderShadowMapCommandsPass::~RenderShadowMapCommandsPass()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}

void RenderShadowMapCommandsPass::Record(RenderParams* pParams)
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
	if (m_EnablePointLights)
	{
		D3DBuffer* pNumShadowCastingLightsBuffer = pParams->m_pNumShadowCastingPointLightsBuffer;
		D3DBuffer* pNumShadowCastersBuffer = pParams->m_pNumDrawPointLightShadowCastersBuffer;

		assert(pNumShadowCastingLightsBuffer != nullptr);
		assert(pNumShadowCastersBuffer != nullptr);

		D3DDescriptorHandle numShadowCastingLightsUAVHandle(pResources->m_SRVHeapStart, 7);
		pCommandList->ClearUnorderedAccessView(numShadowCastingLightsUAVHandle, pNumShadowCastingLightsBuffer->GetUAVHandle(), pNumShadowCastingLightsBuffer, numClearValue);

		D3DDescriptorHandle numShadowCastersUAVHandle(pResources->m_SRVHeapStart, 9);
		pCommandList->ClearUnorderedAccessView(numShadowCastersUAVHandle, pNumShadowCastersBuffer->GetUAVHandle(), pNumShadowCastersBuffer, numClearValue);
	}

	if (m_EnableSpotLights)
	{
		D3DBuffer* pNumShadowCastingLightsBuffer = pParams->m_pNumShadowCastingSpotLightsBuffer;
		D3DBuffer* pNumShadowCastersBuffer = pParams->m_pNumDrawSpotLightShadowCastersBuffer;

		assert(pNumShadowCastingLightsBuffer != nullptr);
		assert(pNumShadowCastersBuffer != nullptr);

		D3DDescriptorHandle numShadowCastingLightsUAVHandle(pResources->m_SRVHeapStart, m_EnablePointLights ? 14 : 7);
		pCommandList->ClearUnorderedAccessView(numShadowCastingLightsUAVHandle, pNumShadowCastingLightsBuffer->GetUAVHandle(), pNumShadowCastingLightsBuffer, numClearValue);

		D3DDescriptorHandle numShadowCastersUAVHandle(pResources->m_SRVHeapStart, m_EnablePointLights ? 16 : 9);
		pCommandList->ClearUnorderedAccessView(numShadowCastersUAVHandle, pNumShadowCastersBuffer->GetUAVHandle(), pNumShadowCastersBuffer, numClearValue);
	}

	pCommandList->ExecuteIndirect(m_pCommandSignature, 1, pParams->m_pIndirectArgumentBuffer, 0, nullptr, 0);
	pCommandList->Close();
}
