#include "RenderPasses/CreateRenderGBufferCommandsPass.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/CommandList.h"
#include "Math/Math.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

CreateRenderGBufferCommandsPass::CreateRenderGBufferCommandsPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const u16 threadGroupSize = 64;
	m_NumThreadGroupsX = (u16)Ceil((f32)pParams->m_NumMeshesInBatch / (f32)threadGroupSize);

	std::string threadGroupSizeStr = std::to_string(threadGroupSize);
	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("THREAD_GROUP_SIZE", threadGroupSizeStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//CreateRenderGBufferCommandsCS.hlsl", "Main", "cs_5_0", shaderDefines);

	D3D12_DESCRIPTOR_RANGE srvDescriptorRanges[] = {SRVDescriptorRange(3, 0), UAVDescriptorRange(1, 0)};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = RootDescriptorTableParameter(ARRAYSIZE(srvDescriptorRanges), &srvDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"CreateRenderGBufferCommandsPass::m_pRootSignature");

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CreateRenderGBufferCommandsPass::m_pPipelineState");
}

CreateRenderGBufferCommandsPass::~CreateRenderGBufferCommandsPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void CreateRenderGBufferCommandsPass::Record(RenderParams* pParams)
{
	assert(false);
	/*
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	ResourceList* pResources = pParams->m_pResources;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);

	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	pCommandList->Dispatch(m_NumThreadGroupsX, 1, 1);
	pCommandList->End();
	*/
}
