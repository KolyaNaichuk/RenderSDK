#include "CommandRecorders/ClearVoxelGridRecorder.h"
#include "DX/DXCommandList.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXResource.h"

enum RootParams
{
	kGridUAVRootParam = 0,
	kGridConfigCBVRootParam,
	kNumRootParams
};

ClearVoxelGridRecorder::ClearVoxelGridRecorder(DXDevice* pDevice, UINT numGridCellsX, UINT numGridCellsY, UINT numGridCellsZ)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_NumThreadGroupsX(0)
	, m_NumThreadGroupsY(0)
	, m_NumThreadGroupsZ(0)
{
	const UINT numThreadsPerGroupX = 8;
	const UINT numThreadsPerGroupY = 8;
	const UINT numThreadsPerGroupZ = 8;

	m_NumThreadGroupsX = numGridCellsX / numThreadsPerGroupX;
	m_NumThreadGroupsY = numGridCellsY / numThreadsPerGroupY;
	m_NumThreadGroupsZ = numGridCellsZ / numThreadsPerGroupZ;

	DXCBVRange cbvRange(1, 0);
	DXUAVRange uavRange(1, 0);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kGridUAVRootParam] = DXRootDescriptorTableParameter(1, &uavRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kGridConfigCBVRootParam] = DXRootDescriptorTableParameter(1, &cbvRange, D3D12_SHADER_VISIBILITY_ALL);
		
	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new DXRootSignature(pDevice, &rootSignatureDesc, L"ClearVoxelGridRecorder::m_pRootSignature");

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

	m_pPipelineState = new DXPipelineState(pDevice, &pipelineStateDesc, L"ClearVoxelGridRecorder::m_pPipelineState");
}

ClearVoxelGridRecorder::~ClearVoxelGridRecorder()
{
	delete m_pPipelineState;
	delete m_pRootSignature;
}

void ClearVoxelGridRecorder::Record(DXCommandList* pCommandList, DXCommandAllocator* pCommandAllocator,
	UINT numDXDescriptorHeaps, ID3D12DescriptorHeap* pDXFirstDescriptorHeap,
	DXResource* pGridBuffer, D3D12_GPU_DESCRIPTOR_HANDLE gridUAVDescriptor,
	D3D12_GPU_DESCRIPTOR_HANDLE gridConfigCBVDescriptor,
	const D3D12_RESOURCE_STATES* pGridBufferEndState)
{
	pCommandList->Reset(pCommandAllocator, m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);

	if (pGridBuffer->GetState() != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		pCommandList->TransitionBarrier(pGridBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	pCommandList->SetDescriptorHeaps(numDXDescriptorHeaps, &pDXFirstDescriptorHeap);
	pCommandList->SetComputeRootDescriptorTable(kGridUAVRootParam, gridUAVDescriptor);
	pCommandList->SetComputeRootDescriptorTable(kGridConfigCBVRootParam, gridConfigCBVDescriptor);

	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, m_NumThreadGroupsZ);

	if (pGridBufferEndState != nullptr)
		pCommandList->TransitionBarrier(pGridBuffer, *pGridBufferEndState);

	pCommandList->Close();
}