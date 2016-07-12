#include "RenderPasses/InjectVPLsIntoVoxelGridPass.h"
#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DPipelineState.h"
#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DDescriptorHeap.h"
#include "D3DWrapper/D3DResource.h"
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

InjectVPLsIntoVoxelGridPass::InjectVPLsIntoVoxelGridPass(InitPrams* pParams)
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

	D3DCBVRange cbvRange(1, 0);
	D3DSRVRange gridBufferSRVRange(1, 0);
	D3DSRVRange omniLightsBufferSRVRange(1, 1);
	D3DSRVRange spotLightsBufferSRVRange(1, 2);
	D3DUAVRange uavRange(1, 0);
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kGridConfigCBVRootParam] = D3DRootDescriptorTableParameter(1, &cbvRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kGridBufferSRVRootParam] = D3DRootDescriptorTableParameter(1, &gridBufferSRVRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kOmniLightsBufferSRVRootParam] = D3DRootDescriptorTableParameter(1, &omniLightsBufferSRVRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kSpotLightsBufferSRVRootParam] = D3DRootDescriptorTableParameter(1, &spotLightsBufferSRVRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kVPLBufferUAVRootParam] = D3DRootDescriptorTableParameter(1, &uavRange, D3D12_SHADER_VISIBILITY_ALL);

	D3DRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new D3DRootSignature(pParams->m_pDevice, &rootSignatureDesc, L"InjectVPLsIntoVoxelGridPass::m_pRootSignature");

	std::string numThreadsPerGroupXStr = std::to_string(numThreadsPerGroupX);
	std::string numThreadsPerGroupYStr = std::to_string(numThreadsPerGroupY);
	std::string numThreadsPerGroupZStr = std::to_string(numThreadsPerGroupZ);

	const D3DShaderMacro shaderDefines[] =
	{
		D3DShaderMacro("NUM_THREADS_X", numThreadsPerGroupXStr.c_str()),
		D3DShaderMacro("NUM_THREADS_Y", numThreadsPerGroupYStr.c_str()),
		D3DShaderMacro("NUM_THREADS_Z", numThreadsPerGroupZStr.c_str()),
		D3DShaderMacro()
	};
	D3DShader computeShader(L"Shaders//InjectVPLsIntoVoxelGridCS.hlsl", "Main", "cs_5_0", shaderDefines);

	D3DComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new D3DPipelineState(pParams->m_pDevice, &pipelineStateDesc, L"InjectVPLsIntoVoxelGridPass::m_pPipelineState");
	*/
}

InjectVPLsIntoVoxelGridPass::~InjectVPLsIntoVoxelGridPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void InjectVPLsIntoVoxelGridPass::Record(RenderParams* pParams)
{
}
