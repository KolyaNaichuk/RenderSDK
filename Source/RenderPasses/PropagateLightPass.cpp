#include "RenderPasses/PropagateLightPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RenderEnv.h"
#include "Math/Math.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

PropagateLightPass::PropagateLightPass(InitPrams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const u16 numThreadsPerGroupX = 8;
	const u16 numThreadsPerGroupY = 8;
	const u16 numThreadsPerGroupZ = 8;

	m_NumThreadGroupsX = (u16)Ceil((f32)pParams->m_NumGridCellsX / (f32)numThreadsPerGroupX);
	m_NumThreadGroupsY = (u16)Ceil((f32)pParams->m_NumGridCellsY / (f32)numThreadsPerGroupY);
	m_NumThreadGroupsZ = (u16)Ceil((f32)pParams->m_NumGridCellsZ / (f32)numThreadsPerGroupZ);

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
	Shader computeShader(L"Shaders//PropagateLightCS.hlsl", "Main", "cs_5_0", shaderDefines);
	
	const D3D12_DESCRIPTOR_RANGE SRVDescriptorRanges[] =
	{
		CBVDescriptorRange(1, 0),
		SRVDescriptorRange(3, 0),
		UAVDescriptorRange(3, 0)
	};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = RootDescriptorTableParameter(ARRAYSIZE(SRVDescriptorRanges), &SRVDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"PropagateLightPass::m_pRootSignature");

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"PropagateLightPass::m_pPipelineState");
}

PropagateLightPass::~PropagateLightPass()
{
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}

void PropagateLightPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	BindingResourceList** ppResources = pParams->m_ppResources;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	std::vector<ResourceTransitionBarrier> resourceBarriers[2];
	for (u8 barrierIndex = 0; barrierIndex < 2; ++barrierIndex)
	{
		BindingResourceList* pCurrResources = ppResources[barrierIndex];
		BindingResourceList* pPrevResources = ppResources[(barrierIndex + 1) % 2];
				
		assert(pCurrResources->m_RequiredResourceStates.size() == 6);
		resourceBarriers[barrierIndex].reserve(6);

		for (u8 resourceIndex = 0; resourceIndex < 6; ++resourceIndex)
		{
			RequiredResourceState& prevResourceState = pPrevResources->m_RequiredResourceStates[(resourceIndex + 3) % 6];
			RequiredResourceState& currResourceState = pCurrResources->m_RequiredResourceStates[resourceIndex];
			assert(prevResourceState.m_pResource == currResourceState.m_pResource);
			
			resourceBarriers[barrierIndex].emplace_back(currResourceState.m_pResource, prevResourceState.m_RequiredState, currResourceState.m_RequiredState);
		}
	}
	{
		BindingResourceList* pResources = ppResources[0];

		pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
		pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);
		pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, m_NumThreadGroupsZ);
	}
	for (u16 it = 1; it < pParams->m_NumIterations; ++it)
	{
		const u16 resourceIndex = it % 2;
		BindingResourceList* pResources = ppResources[resourceIndex];

		pCommandList->ResourceBarrier(resourceBarriers[resourceIndex].size(), resourceBarriers[resourceIndex].data());
		pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);
		pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, m_NumThreadGroupsZ);
	}
	if ((pParams->m_NumIterations % 2) == 0)
		pCommandList->ResourceBarrier(resourceBarriers[0].size(), resourceBarriers[0].data());
					
	pCommandList->End();
}
