#include "CommandRecorders/RenderShadowMapCommandsRecorder.h"
#include "DX/DXRootSignature.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRenderEnvironment.h"
#include "DX/DXCommandList.h"
#include "DX/DXCommandSignature.h"
#include "DX/DXResource.h"
#include "Math/Math.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

RenderShadowMapCommandsRecorder::RenderShadowMapCommandsRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pCommandSignature(nullptr)
	, m_pIndirectArgumentBuffer(nullptr)
	, m_EnablePointLights(pParams->m_EnablePointLights)
	, m_EnableSpotLights(pParams->m_EnableSpotLights)
{
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;

	const u16 threadGroupSize = 64;
	std::string threadGroupSizeStr = std::to_string(threadGroupSize);
	std::string enablePointLightsStr = std::to_string(pParams->m_EnablePointLights ? 1 : 0);
	std::string maxNumPointLightsPerShadowCasterStr = std::to_string(pParams->m_MaxNumPointLightsPerShadowCaster);
	std::string enableSpotLightsStr = std::to_string(pParams->m_EnableSpotLights ? 1 : 0);
	std::string maxNumSpotLightsPerShadowCasterStr = std::to_string(pParams->m_MaxNumSpotLightsPerShadowCaster);

	const DXShaderMacro shaderDefines[] =
	{
		DXShaderMacro("THREAD_GROUP_SIZE", threadGroupSizeStr.c_str()),
		DXShaderMacro("ENABLE_POINT_LIGHTS", enablePointLightsStr.c_str()),
		DXShaderMacro("MAX_NUM_POINT_LIGHTS_PER_SHADOW_CASTER", maxNumPointLightsPerShadowCasterStr.c_str()),
		DXShaderMacro("ENABLE_SPOT_LIGHTS", enableSpotLightsStr.c_str()),
		DXShaderMacro("MAX_NUM_SPOT_LIGHTS_PER_SHADOW_CASTER", maxNumSpotLightsPerShadowCasterStr.c_str()),
		DXShaderMacro()
	};
	DXShader computeShader(L"Shaders//CreateRenderShadowMapCommandsCS.hlsl", "Main", "cs_5_0", shaderDefines);

	std::vector<D3D12_DESCRIPTOR_RANGE> srvDescriptorRanges;
	srvDescriptorRanges.push_back(DXSRVRange(3, 0));

	if (m_EnablePointLights)
	{
		srvDescriptorRanges.push_back(DXSRVRange(2, 3));
		srvDescriptorRanges.push_back(DXUAVRange(4, 0));
	}
	if (m_EnableSpotLights)
	{
		srvDescriptorRanges.push_back(DXSRVRange(2, 5));
		srvDescriptorRanges.push_back(DXUAVRange(4, 4));
	}
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = DXRootDescriptorTableParameter(srvDescriptorRanges.size(), &srvDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new DXRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"RenderShadowMapCommandsRecorder::m_pRootSignature");

	DXComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new DXPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RenderShadowMapCommandsRecorder::m_pPipelineState");

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] = {DXDispatchArgument()};
	DXCommandSignatureDesc commandSignatureDesc(sizeof(D3D12_DISPATCH_ARGUMENTS), ARRAYSIZE(argumentDescs), &argumentDescs[0]);
	m_pCommandSignature = new DXCommandSignature(pRenderEnv->m_pDevice, nullptr, &commandSignatureDesc, L"RenderShadowMapCommandsRecorder::m_pCommandSignature");
}

RenderShadowMapCommandsRecorder::~RenderShadowMapCommandsRecorder()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pIndirectArgumentBuffer);
}

void RenderShadowMapCommandsRecorder::Record(RenderPassParams* pParams)
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
	if (m_EnablePointLights)
	{
		DXBuffer* pNumShadowCastingLightsBuffer = pParams->m_pNumShadowCastingPointLightsBuffer;
		DXBuffer* pNumShadowCastersBuffer = pParams->m_pNumDrawPointLightShadowCastersBuffer;

		DXDescriptorHandle numShadowCastingLightsUAVHandle(pResources->m_SRVHeapStart, 6);
		pCommandList->ClearUnorderedAccessView(numShadowCastingLightsUAVHandle, pNumShadowCastingLightsBuffer->GetUAVHandle(), pNumShadowCastingLightsBuffer, numClearValue);

		DXDescriptorHandle numShadowCastersUAVHandle(pResources->m_SRVHeapStart, 8);
		pCommandList->ClearUnorderedAccessView(numShadowCastersUAVHandle, pNumShadowCastersBuffer->GetUAVHandle(), pNumShadowCastersBuffer, numClearValue);
	}
	if (m_EnableSpotLights)
	{
		DXBuffer* pNumShadowCastingLightsBuffer = pParams->m_pNumShadowCastingSpotLightsBuffer;
		DXBuffer* pNumShadowCastersBuffer = pParams->m_pNumDrawSpotLightShadowCastersBuffer;

		DXDescriptorHandle numShadowCastingLightsUAVHandle(pResources->m_SRVHeapStart, m_EnablePointLights ? 12 : 6);
		pCommandList->ClearUnorderedAccessView(numShadowCastingLightsUAVHandle, pNumShadowCastingLightsBuffer->GetUAVHandle(), pNumShadowCastingLightsBuffer, numClearValue);

		DXDescriptorHandle numShadowCastersUAVHandle(pResources->m_SRVHeapStart, m_EnablePointLights ? 14 : 8);
		pCommandList->ClearUnorderedAccessView(numShadowCastersUAVHandle, pNumShadowCastersBuffer->GetUAVHandle(), pNumShadowCastersBuffer, numClearValue);
	}

	assert(false && "Invalid pArgumentBuffer");
	pCommandList->ExecuteIndirect(m_pCommandSignature, 1, /*pArgumentBuffer*/nullptr, 0, nullptr, 0);
	pCommandList->Close();
}
