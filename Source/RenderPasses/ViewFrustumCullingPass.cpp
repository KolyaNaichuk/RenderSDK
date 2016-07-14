#include "RenderPasses/ViewFrustumCullingPass.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/CommandList.h"
#include "Math/Math.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

ViewFrustumCullingPass::ViewFrustumCullingPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const u16 threadGroupSize = 64;
	m_NumThreadGroupsX = (u16)Ceil((f32)pParams->m_NumObjects / (f32)threadGroupSize);

	std::string threadGroupSizeStr = std::to_string(threadGroupSize);
	std::string objectBoundsTypeStr = std::to_string(pParams->m_ObjectBoundsType);

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("THREAD_GROUP_SIZE", threadGroupSizeStr.c_str()),
		ShaderMacro("OBJECT_BOUNDS_TYPE", objectBoundsTypeStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//ViewFrustumCullingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	D3D12_DESCRIPTOR_RANGE srvDescriptorRanges[] = {UAVDescriptorRange(2, 0), SRVDescriptorRange(1, 0), CBVDescriptorRange(1, 0)};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = RootDescriptorTableParameter(ARRAYSIZE(srvDescriptorRanges), &srvDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"ViewFrustumCullingPass::m_pRootSignature");

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"ViewFrustumCullingPass::m_pPipelineState");
}

ViewFrustumCullingPass::~ViewFrustumCullingPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void ViewFrustumCullingPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	BindingResourceList* pResources = pParams->m_pResources;
	Buffer* pNumVisibleObjectsBuffer = pParams->m_pNumVisibleObjectsBuffer;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	const UINT numObjectsClearValue[4] = {0, 0, 0, 0};
	pCommandList->ClearUnorderedAccessView(pResources->m_SRVHeapStart, pNumVisibleObjectsBuffer->GetUAVHandle(), pNumVisibleObjectsBuffer, numObjectsClearValue);

	pCommandList->Dispatch(m_NumThreadGroupsX, 1, 1);
	pCommandList->Close();
}
