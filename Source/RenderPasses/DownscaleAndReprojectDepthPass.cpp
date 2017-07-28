#include "RenderPasses/DownscaleAndReprojectDepthPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "Math/Math.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

DownscaleAndReprojectDepthPass::DownscaleAndReprojectDepthPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_NumThreadGroupsX(0)
	, m_NumThreadGroupsY(0)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	
	const u16 numThreads = 8;
	m_NumThreadGroupsX = (u16)Ceil((f32)pParams->m_ReprojectedDepthTextureWidth / (f32)numThreads);
	m_NumThreadGroupsY = (u16)Ceil((f32)pParams->m_ReprojectedDepthTextureHeight / (f32)numThreads);

	std::string numThreadsStr = std::to_string(numThreads);
	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("NUM_THREADS_X", numThreadsStr.c_str()),
		ShaderMacro("NUM_THREADS_Y", numThreadsStr.c_str()),
		ShaderMacro()
	};

	Shader computeShader(L"Shaders//DownscaleAndReprojectDepthCS.hlsl", "Main", "cs_5_0", shaderDefines);
	D3D12_DESCRIPTOR_RANGE srvDescriptorRanges[] = {CBVDescriptorRange(1, 0), SRVDescriptorRange(1, 0), UAVDescriptorRange(1, 0)};

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = RootDescriptorTableParameter(ARRAYSIZE(srvDescriptorRanges), srvDescriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	StaticSamplerDesc samplerDesc(StaticSamplerDesc::Max, 0, D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, 1, &samplerDesc);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"DownscaleAndReprojectDepthPass::m_pRootSignature");

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"DownscaleAndReprojectDepthPass::m_pPipelineState");
}

DownscaleAndReprojectDepthPass::~DownscaleAndReprojectDepthPass()
{
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}

void DownscaleAndReprojectDepthPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	ResourceList* pResources = pParams->m_pResources;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);
	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);
	pCommandList->End();
}
