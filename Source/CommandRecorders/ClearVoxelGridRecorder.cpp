#include "CommandRecorders/ClearVoxelGridRecorder.h"
#include "DX/DXCommandList.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXResource.h"
#include "Math/Math.h"

enum RootParams
{
	kGridBufferUAVRootParam = 0,
	kGridConfigCBVRootParam,
	kNumRootParams
};

ClearVoxelGridRecorder::ClearVoxelGridRecorder(ClearVoxelGridInitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_NumThreadGroupsX(0)
	, m_NumThreadGroupsY(0)
	, m_NumThreadGroupsZ(0)
{
	const u16 numThreadsPerGroupX = 8;
	const u16 numThreadsPerGroupY = 8;
	const u16 numThreadsPerGroupZ = 8;

	m_NumThreadGroupsX = (u16)Ceil((f32)pParams->m_NumGridCellsX / (f32)numThreadsPerGroupX);
	m_NumThreadGroupsY = (u16)Ceil((f32)pParams->m_NumGridCellsY / (f32)numThreadsPerGroupY);
	m_NumThreadGroupsZ = (u16)Ceil((f32)pParams->m_NumGridCellsZ / (f32)numThreadsPerGroupZ);

	DXCBVRange cbvRange(1, 0);
	DXUAVRange uavRange(1, 0);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kGridBufferUAVRootParam] = DXRootDescriptorTableParameter(1, &uavRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kGridConfigCBVRootParam] = DXRootDescriptorTableParameter(1, &cbvRange, D3D12_SHADER_VISIBILITY_ALL);
		
	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new DXRootSignature(pParams->m_pDevice, &rootSignatureDesc, L"ClearVoxelGridRecorder::m_pRootSignature");

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

	m_pPipelineState = new DXPipelineState(pParams->m_pDevice, &pipelineStateDesc, L"ClearVoxelGridRecorder::m_pPipelineState");
}

ClearVoxelGridRecorder::~ClearVoxelGridRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void ClearVoxelGridRecorder::Record(ClearVoxelGridRecordParams* pParams)
{
	pParams->m_pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pParams->m_pCommandList->SetComputeRootSignature(m_pRootSignature);

	if (pParams->m_pGridBuffer->GetState() != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		pParams->m_pCommandList->TransitionBarrier(pParams->m_pGridBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	pParams->m_pCommandList->SetDescriptorHeaps(pParams->m_NumDXDescriptorHeaps, &pParams->m_pDXFirstDescriptorHeap);
	pParams->m_pCommandList->SetComputeRootDescriptorTable(kGridBufferUAVRootParam, pParams->m_GridBufferUAVHandle);
	pParams->m_pCommandList->SetComputeRootDescriptorTable(kGridConfigCBVRootParam, pParams->m_GridConfigCBVHandle);

	pParams->m_pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, m_NumThreadGroupsZ);
	pParams->m_pCommandList->Close();
}