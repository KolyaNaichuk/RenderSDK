#include "CommandRecorders/ClearVoxelGridRecorder.h"
#include "DX/DXCommandList.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXDescriptorHeap.h"
#include "DX/DXResourceList.h"
#include "DX/DXRenderEnvironment.h"
#include "Math/Math.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

ClearVoxelGridRecorder::ClearVoxelGridRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;

	const u16 numThreadsPerGroupX = 8;
	const u16 numThreadsPerGroupY = 8;
	const u16 numThreadsPerGroupZ = 8;

	m_NumThreadGroupsX = (u16)Ceil((f32)pParams->m_NumGridCellsX / (f32)numThreadsPerGroupX);
	m_NumThreadGroupsY = (u16)Ceil((f32)pParams->m_NumGridCellsY / (f32)numThreadsPerGroupY);
	m_NumThreadGroupsZ = (u16)Ceil((f32)pParams->m_NumGridCellsZ / (f32)numThreadsPerGroupZ);
	
	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {DXCBVRange(1, 0), DXUAVRange(1, 0)};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = DXRootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), &descriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);
		
	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new DXRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"ClearVoxelGridRecorder::m_pRootSignature");

	std::string numThreadsPerGroupXStr = std::to_string(numThreadsPerGroupX);
	std::string numThreadsPerGroupYStr = std::to_string(numThreadsPerGroupY);
	std::string numThreadsPerGroupZStr = std::to_string(numThreadsPerGroupZ);

	const DXShaderMacro shaderDefines[] =
	{
		DXShaderMacro("NUM_THREADS_X", numThreadsPerGroupXStr.c_str()),
		DXShaderMacro("NUM_THREADS_Y", numThreadsPerGroupYStr.c_str()),
		DXShaderMacro("NUM_THREADS_Z", numThreadsPerGroupZStr.c_str()),
		DXShaderMacro()
	};
	DXShader computeShader(L"Shaders//ClearVoxelGridCS.hlsl", "Main", "cs_5_0", shaderDefines);

	DXComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new DXPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"ClearVoxelGridRecorder::m_pPipelineState");
}

ClearVoxelGridRecorder::~ClearVoxelGridRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void ClearVoxelGridRecorder::Record(RenderPassParams* pParams)
{
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;
	DXCommandList* pCommandList = pParams->m_pCommandList;
	DXBindingResourceList* pResources = pParams->m_pResources;
	
	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	
	pCommandList->SetResourceTransitions(&pResources->m_ResourceTransitions);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);
	
	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, m_NumThreadGroupsZ);
	pCommandList->Close();
}