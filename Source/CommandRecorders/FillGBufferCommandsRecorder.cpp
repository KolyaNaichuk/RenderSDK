#include "CommandRecorders/FillGBufferCommandsRecorder.h"
#include "DX/DXRootSignature.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRenderEnvironment.h"
#include "DX/DXCommandList.h"
#include "Math/Math.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

FillGBufferCommandsRecorder::FillGBufferCommandsRecorder(InitParams* pParams)
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
	DXShader computeShader(L"Shaders//CreateFillGBufferCommandsCS.hlsl", "Main", "cs_5_0", shaderDefines);

	D3D12_DESCRIPTOR_RANGE srvDescriptorRanges[] = {DXSRVRange(3, 0), DXUAVRange(1, 0)};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = DXRootDescriptorTableParameter(ARRAYSIZE(srvDescriptorRanges), &srvDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new DXRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"FillGBufferCommandsRecorder::m_pRootSignature");

	DXComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new DXPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"FillGBufferCommandsRecorder::m_pPipelineState");
}

FillGBufferCommandsRecorder::~FillGBufferCommandsRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void FillGBufferCommandsRecorder::Record(RenderPassParams* pParams)
{
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;
	DXCommandList* pCommandList = pParams->m_pCommandList;
	DXBindingResourceList* pResources = pParams->m_pResources;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);

	pCommandList->SetResourceTransitions(&pResources->m_ResourceTransitions);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	pCommandList->Dispatch(m_NumThreadGroupsX, 1, 1);
	pCommandList->Close();
}
