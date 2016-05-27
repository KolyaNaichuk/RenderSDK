#include "CommandRecorders/ViewFrustumCullingRecorder.h"
#include "Common/MeshBatch.h"
#include "DX/DXRootSignature.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRenderEnvironment.h"
#include "DX/DXResource.h"
#include "DX/DXCommandList.h"
#include "Math/Math.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

ViewFrustumCullingRecorder::ViewFrustumCullingRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	DXRenderEnvironment* pEnv = pParams->m_pEnv;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	const u16 threadGroupSize = 64;
	m_NumThreadGroupsX = (u16)Ceil((f32)pMeshBatch->GetNumMeshes() / (f32)threadGroupSize);

	std::string threadGroupSizeStr = std::to_string(threadGroupSize);
	const DXShaderMacro shaderDefines[] =
	{
		DXShaderMacro("THREAD_GROUP_SIZE", threadGroupSizeStr.c_str()),
		DXShaderMacro()
	};
	DXShader computeShader(L"Shaders//ViewFrustumCullingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {DXUAVRange(2, 0), DXSRVRange(2, 0), DXCBVRange(1, 0)};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = DXRootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), &descriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new DXRootSignature(pEnv->m_pDevice, &rootSignatureDesc, L"ViewFrustumCullingRecorder::m_pRootSignature");

	DXComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new DXPipelineState(pEnv->m_pDevice, &pipelineStateDesc, L"ViewFrustumCullingRecorder::m_pPipelineState");
}

ViewFrustumCullingRecorder::~ViewFrustumCullingRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void ViewFrustumCullingRecorder::Record(RenderPassParams* pParams)
{
	DXRenderEnvironment* pEnv = pParams->m_pEnv;
	DXCommandList* pCommandList = pParams->m_pCommandList;
	DXBindingResourceList* pResources = pParams->m_pResources;
	DXBuffer* pNumDrawsBuffer = pParams->m_pNumDrawsBuffer;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);

	const UINT numDrawsClearValue[4] = {0, 0, 0, 0};
	pCommandList->ClearUnorderedAccessView(pResources->m_SRVHeapStart, pNumDrawsBuffer->GetUAVHandle(), pNumDrawsBuffer, numDrawsClearValue);

	pCommandList->SetResourceTransitions(&pResources->m_ResourceTransitions);
	pCommandList->SetDescriptorHeaps(pEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	pCommandList->Dispatch(m_NumThreadGroupsX, 1, 1);
	pCommandList->Close();
}
