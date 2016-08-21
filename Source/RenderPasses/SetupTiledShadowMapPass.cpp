#include "RenderPasses/SetupTiledShadowMapPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "Math/Math.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

SetupTiledShadowMapPass::SetupTiledShadowMapPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_NumThreadGroupsX(0)
	, m_NumThreadGroupsY(0)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const u16 threadGroupSize = 64;	
	m_NumThreadGroupsX = (u16)Ceil((f32)pParams->m_MaxNumLights / (f32)threadGroupSize);

	if (pParams->m_LightType == LightType_Point)
		m_NumThreadGroupsY = kNumCubeMapFaces;
	else if (pParams->m_LightType == LightType_Spot)
		m_NumThreadGroupsY = 1;
	else
		assert(false);

	std::string threadGroupSizeStr = std::to_string(threadGroupSize);
	std::string lightTypeStr = std::to_string(pParams->m_LightType);

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("THREAD_GROUP_SIZE", threadGroupSizeStr.c_str()),
		ShaderMacro("LIGHT_TYPE", lightTypeStr.c_str()),
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
	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);
	pCommandList->End();
}
