#include "CommandRecorders/DetectVisibleMeshesRecorder.h"
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

DetectVisibleMeshesRecorder::DetectVisibleMeshesRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;

	const u16 threadGroupSize = 64;
	m_NumThreadGroupsX = (u16)Ceil((f32)pParams->m_NumMeshesInBatch / (f32)threadGroupSize);

	std::string threadGroupSizeStr = std::to_string(threadGroupSize);
	const DXShaderMacro shaderDefines[] =
	{
		DXShaderMacro("THREAD_GROUP_SIZE", threadGroupSizeStr.c_str()),
		DXShaderMacro()
	};
	DXShader computeShader(L"Shaders//DetectVisibleMeshesCS.hlsl", "Main", "cs_5_0", shaderDefines);

	D3D12_DESCRIPTOR_RANGE srvDescriptorRanges[] = {DXUAVRange(2, 0), DXSRVRange(1, 0), DXCBVRange(1, 0)};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = DXRootDescriptorTableParameter(ARRAYSIZE(srvDescriptorRanges), &srvDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new DXRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"DetectVisibleMeshesRecorder::m_pRootSignature");

	DXComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new DXPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"DetectVisibleMeshesRecorder::m_pPipelineState");
}

DetectVisibleMeshesRecorder::~DetectVisibleMeshesRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void DetectVisibleMeshesRecorder::Record(RenderPassParams* pParams)
{
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;
	DXCommandList* pCommandList = pParams->m_pCommandList;
	DXBindingResourceList* pResources = pParams->m_pResources;
	DXBuffer* pNumVisibleMeshesBuffer = pParams->m_pNumVisibleMeshesBuffer;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetResourceTransitions(&pResources->m_ResourceTransitions);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	const UINT numMeshesClearValue[4] = {0, 0, 0, 0};
	pCommandList->ClearUnorderedAccessView(pResources->m_SRVHeapStart, pNumVisibleMeshesBuffer->GetUAVHandle(), pNumVisibleMeshesBuffer, numMeshesClearValue);

	pCommandList->Dispatch(m_NumThreadGroupsX, 1, 1);
	pCommandList->Close();
}
