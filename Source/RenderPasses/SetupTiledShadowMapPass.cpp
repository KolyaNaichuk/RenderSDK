#include "RenderPasses/SetupTiledShadowMapPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/RenderEnv.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

SetupTiledShadowMapPass::SetupTiledShadowMapPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	std::string numTilesXStr = std::to_string(pParams->m_NumTilesX);
	std::string numTilesYStr = std::to_string(pParams->m_NumTilesY);

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("NUM_TILES_X", numTilesXStr.c_str()),
		ShaderMacro("NUM_TILES_Y", numTilesYStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//SetupTiledShadowMapCS.hlsl", "Main", "cs_5_0", shaderDefines);

	D3D12_DESCRIPTOR_RANGE srvDescriptorRanges[] =
	{
		CBVDescriptorRange(1, 0),
		SRVDescriptorRange(3, 0),
		UAVDescriptorRange(2, 0)
	};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = RootDescriptorTableParameter(ARRAYSIZE(srvDescriptorRanges), &srvDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"SetupTiledShadowMapPass::m_pRootSignature");

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"SetupTiledShadowMapPass::m_pPipelineState");
}

SetupTiledShadowMapPass::~SetupTiledShadowMapPass()
{
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}

void SetupTiledShadowMapPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	BindingResourceList* pResources = pParams->m_pResources;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);
	pCommandList->Dispatch(1, 1, 1);
	pCommandList->End();
}
