#include "CommandRecorders/InjectVPLsIntoVoxelGridRecorder.h"
#include "DX/DXCommandList.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXDescriptorHeap.h"
#include "DX/DXResource.h"
#include "Math/Math.h"

enum RootParams
{
	kGridConfigCBVRootParam,
	kGridBufferSRVRootParam,
	kOmniLightsBufferSRVRootParam,
	kSpotLightsBufferSRVRootParam,
	kVPLBufferUAVRootParam,
	kNumRootParams
};

InjectVPLsIntoVoxelGridRecorder::InjectVPLsIntoVoxelGridRecorder(InitPrams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	// Kolya: fix me
	assert(false);
	/*
	const u16 numThreadsPerGroupX = 8;
	const u16 numThreadsPerGroupY = 8;
	const u16 numThreadsPerGroupZ = 8;

	m_NumThreadGroupsX = (u16)Ceil((f32)pParams->m_NumGridCellsX / (f32)numThreadsPerGroupX);
	m_NumThreadGroupsY = (u16)Ceil((f32)pParams->m_NumGridCellsY / (f32)numThreadsPerGroupY);
	m_NumThreadGroupsZ = (u16)Ceil((f32)pParams->m_NumGridCellsZ / (f32)numThreadsPerGroupZ);

	DXCBVRange cbvRange(1, 0);
	DXSRVRange gridBufferSRVRange(1, 0);
	DXSRVRange omniLightsBufferSRVRange(1, 1);
	DXSRVRange spotLightsBufferSRVRange(1, 2);
	DXUAVRange uavRange(1, 0);
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kGridConfigCBVRootParam] = DXRootDescriptorTableParameter(1, &cbvRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kGridBufferSRVRootParam] = DXRootDescriptorTableParameter(1, &gridBufferSRVRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kOmniLightsBufferSRVRootParam] = DXRootDescriptorTableParameter(1, &omniLightsBufferSRVRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kSpotLightsBufferSRVRootParam] = DXRootDescriptorTableParameter(1, &spotLightsBufferSRVRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kVPLBufferUAVRootParam] = DXRootDescriptorTableParameter(1, &uavRange, D3D12_SHADER_VISIBILITY_ALL);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new DXRootSignature(pParams->m_pDevice, &rootSignatureDesc, L"InjectVPLsIntoVoxelGridRecorder::m_pRootSignature");

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
	DXShader computeShader(L"Shaders//InjectVPLsIntoVoxelGridCS.hlsl", "Main", "cs_5_0", shaderDefines);

	DXComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new DXPipelineState(pParams->m_pDevice, &pipelineStateDesc, L"InjectVPLsIntoVoxelGridRecorder::m_pPipelineState");
	*/
}

InjectVPLsIntoVoxelGridRecorder::~InjectVPLsIntoVoxelGridRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void InjectVPLsIntoVoxelGridRecorder::Record(RenderPassParams* pParams)
{
}
