#include "RenderPasses/InjectVPLsIntoVoxelGridPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/GraphicsResource.h"
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

	CBVDescriptorRange cbvRange(1, 0);
	SRVDescriptorRange gridBufferSRVRange(1, 0);
	SRVDescriptorRange omniLightsBufferSRVRange(1, 1);
	SRVDescriptorRange spotLightsBufferSRVRange(1, 2);
	UAVDescriptorRange uavRange(1, 0);
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kGridConfigCBVRootParam] = RootDescriptorTableParameter(1, &cbvRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kGridBufferSRVRootParam] = RootDescriptorTableParameter(1, &gridBufferSRVRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kOmniLightsBufferSRVRootParam] = RootDescriptorTableParameter(1, &omniLightsBufferSRVRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kSpotLightsBufferSRVRootParam] = RootDescriptorTableParameter(1, &spotLightsBufferSRVRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kVPLBufferUAVRootParam] = RootDescriptorTableParameter(1, &uavRange, D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pDevice, &rootSignatureDesc, L"InjectVPLsIntoVoxelGridPass::m_pRootSignature");

	std::string numThreadsPerGroupXStr = std::to_string(numThreadsPerGroupX);
	std::string numThreadsPerGroupYStr = std::to_string(numThreadsPerGroupY);
	std::string numThreadsPerGroupZStr = std::to_string(numThreadsPerGroupZ);

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("NUM_THREADS_X", numThreadsPerGroupXStr.c_str()),
		ShaderMacro("NUM_THREADS_Y", numThreadsPerGroupYStr.c_str()),
		ShaderMacro("NUM_THREADS_Z", numThreadsPerGroupZStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//InjectVPLsIntoVoxelGridCS.hlsl", "Main", "cs_5_0", shaderDefines);

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pParams->m_pDevice, &pipelineStateDesc, L"InjectVPLsIntoVoxelGridPass::m_pPipelineState");
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
