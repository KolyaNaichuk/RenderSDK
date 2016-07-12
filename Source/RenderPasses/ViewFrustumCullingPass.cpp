#include "RenderPasses/ViewFrustumCullingPass.h"
#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DPipelineState.h"
#include "D3DWrapper/D3DRenderEnv.h"
#include "D3DWrapper/D3DResource.h"
#include "D3DWrapper/D3DCommandList.h"
#include "Math/Math.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

ViewFrustumCullingPass::ViewFrustumCullingPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const u16 threadGroupSize = 64;
	m_NumThreadGroupsX = (u16)Ceil((f32)pParams->m_NumObjects / (f32)threadGroupSize);

	std::string threadGroupSizeStr = std::to_string(threadGroupSize);
	std::string objectBoundsTypeStr = std::to_string(pParams->m_ObjectBoundsType);

	const D3DShaderMacro shaderDefines[] =
	{
		D3DShaderMacro("THREAD_GROUP_SIZE", threadGroupSizeStr.c_str()),
		D3DShaderMacro("OBJECT_BOUNDS_TYPE", objectBoundsTypeStr.c_str()),
		D3DShaderMacro()
	};
	D3DShader computeShader(L"Shaders//ViewFrustumCullingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	D3D12_DESCRIPTOR_RANGE srvDescriptorRanges[] = {D3DUAVRange(2, 0), D3DSRVRange(1, 0), D3DCBVRange(1, 0)};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = D3DRootDescriptorTableParameter(ARRAYSIZE(srvDescriptorRanges), &srvDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	D3DRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new D3DRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"ViewFrustumCullingPass::m_pRootSignature");

	D3DComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new D3DPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"ViewFrustumCullingPass::m_pPipelineState");
}

ViewFrustumCullingPass::~ViewFrustumCullingPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void ViewFrustumCullingPass::Record(RenderParams* pParams)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	D3DCommandList* pCommandList = pParams->m_pCommandList;
	D3DResourceList* pResources = pParams->m_pResources;
	D3DBuffer* pNumVisibleObjectsBuffer = pParams->m_pNumVisibleObjectsBuffer;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetResourceTransitions(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	const UINT numObjectsClearValue[4] = {0, 0, 0, 0};
	pCommandList->ClearUnorderedAccessView(pResources->m_SRVHeapStart, pNumVisibleObjectsBuffer->GetUAVHandle(), pNumVisibleObjectsBuffer, numObjectsClearValue);

	pCommandList->Dispatch(m_NumThreadGroupsX, 1, 1);
	pCommandList->Close();
}
