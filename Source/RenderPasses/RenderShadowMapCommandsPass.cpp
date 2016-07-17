#include "RenderPasses/RenderShadowMapCommandsPass.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/GraphicsResource.h"
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
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const u16 threadGroupSize = 64;
	std::string threadGroupSizeStr = std::to_string(threadGroupSize);
	std::string enablePointLightsStr = std::to_string(pParams->m_EnablePointLights ? 1 : 0);
	std::string maxNumPointLightsPerShadowCasterStr = std::to_string(pParams->m_MaxNumPointLightsPerShadowCaster);
	std::string enableSpotLightsStr = std::to_string(pParams->m_EnableSpotLights ? 1 : 0);
	std::string maxNumSpotLightsPerShadowCasterStr = std::to_string(pParams->m_MaxNumSpotLightsPerShadowCaster);

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("THREAD_GROUP_SIZE", threadGroupSizeStr.c_str()),
		ShaderMacro("ENABLE_POINT_LIGHTS", enablePointLightsStr.c_str()),
		ShaderMacro("MAX_NUM_POINT_LIGHTS_PER_SHADOW_CASTER", maxNumPointLightsPerShadowCasterStr.c_str()),
		ShaderMacro("ENABLE_SPOT_LIGHTS", enableSpotLightsStr.c_str()),
		ShaderMacro("MAX_NUM_SPOT_LIGHTS_PER_SHADOW_CASTER", maxNumSpotLightsPerShadowCasterStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//RenderShadowMapCommandsCS.hlsl", "Main", "cs_5_0", shaderDefines);

	std::vector<D3D12_DESCRIPTOR_RANGE> srvDescriptorRanges;
	srvDescriptorRanges.push_back(SRVDescriptorRange(3, 0));

	if (m_EnablePointLights)
	{
		srvDescriptorRanges.push_back(SRVDescriptorRange(3, 3));
		srvDescriptorRanges.push_back(UAVDescriptorRange(4, 0));
	}
	if (m_EnableSpotLights)
	{
		srvDescriptorRanges.push_back(SRVDescriptorRange(3, 6));
		srvDescriptorRanges.push_back(UAVDescriptorRange(4, 4));
	}
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = RootDescriptorTableParameter(srvDescriptorRanges.size(), &srvDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"RenderShadowMapCommandsPass::m_pRootSignature");

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RenderShadowMapCommandsPass::m_pPipelineState");

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] = {DispatchArgument()};
	CommandSignatureDesc commandSignatureDesc(sizeof(D3D12_DISPATCH_ARGUMENTS), ARRAYSIZE(argumentDescs), &argumentDescs[0]);
	m_pCommandSignature = new CommandSignature(pRenderEnv->m_pDevice, nullptr, &commandSignatureDesc, L"RenderShadowMapCommandsPass::m_pCommandSignature");
}

RenderShadowMapCommandsPass::~RenderShadowMapCommandsPass()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}

void RenderShadowMapCommandsPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	BindingResourceList* pResources = pParams->m_pResources;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	const UINT numClearValue[4] = {0, 0, 0, 0};
	if (m_EnablePointLights)
	{
		Buffer* pNumShadowCastingLightsBuffer = pParams->m_pNumShadowCastingPointLightsBuffer;
		Buffer* pNumShadowCastersBuffer = pParams->m_pNumDrawPointLightShadowCastersBuffer;

		assert(pNumShadowCastingLightsBuffer != nullptr);
		assert(pNumShadowCastersBuffer != nullptr);

		DescriptorHandle numShadowCastingLightsUAVHandle(pResources->m_SRVHeapStart, 7);
		pCommandList->ClearUnorderedAccessView(numShadowCastingLightsUAVHandle, pNumShadowCastingLightsBuffer->GetUAVHandle(), pNumShadowCastingLightsBuffer, numClearValue);

		DescriptorHandle numShadowCastersUAVHandle(pResources->m_SRVHeapStart, 9);
		pCommandList->ClearUnorderedAccessView(numShadowCastersUAVHandle, pNumShadowCastersBuffer->GetUAVHandle(), pNumShadowCastersBuffer, numClearValue);
	}

	if (m_EnableSpotLights)
	{
		Buffer* pNumShadowCastingLightsBuffer = pParams->m_pNumShadowCastingSpotLightsBuffer;
		Buffer* pNumShadowCastersBuffer = pParams->m_pNumDrawSpotLightShadowCastersBuffer;

		assert(pNumShadowCastingLightsBuffer != nullptr);
		assert(pNumShadowCastersBuffer != nullptr);

		DescriptorHandle numShadowCastingLightsUAVHandle(pResources->m_SRVHeapStart, m_EnablePointLights ? 14 : 7);
		pCommandList->ClearUnorderedAccessView(numShadowCastingLightsUAVHandle, pNumShadowCastingLightsBuffer->GetUAVHandle(), pNumShadowCastingLightsBuffer, numClearValue);

		DescriptorHandle numShadowCastersUAVHandle(pResources->m_SRVHeapStart, m_EnablePointLights ? 16 : 9);
		pCommandList->ClearUnorderedAccessView(numShadowCastersUAVHandle, pNumShadowCastersBuffer->GetUAVHandle(), pNumShadowCastersBuffer, numClearValue);
	}

	pCommandList->ExecuteIndirect(m_pCommandSignature, 1, pParams->m_pIndirectArgumentBuffer, 0, nullptr, 0);
	pCommandList->End();
}
