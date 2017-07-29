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

PropagateLightPass::PropagateLightPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pWithoutOcclusionTestState(nullptr)
	, m_pWithOcclusionTestState(nullptr)
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
	
	const D3D12_DESCRIPTOR_RANGE srvDescriptorRanges[] =
	{
		CBVDescriptorRange(1, 0),
		SRVDescriptorRange(4, 0),
		UAVDescriptorRange(6, 0)
	};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = RootDescriptorTableParameter(ARRAYSIZE(srvDescriptorRanges), &srvDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"PropagateLightPass::m_pRootSignature");

	{
		const ShaderMacro shaderDefines[] =
		{
			ShaderMacro("NUM_THREADS_X", numThreadsPerGroupXStr.c_str()),
			ShaderMacro("NUM_THREADS_Y", numThreadsPerGroupYStr.c_str()),
			ShaderMacro("NUM_THREADS_Z", numThreadsPerGroupZStr.c_str()),
			ShaderMacro("TEST_OCCLUSION", "0"),
			ShaderMacro()
		};
		Shader computeShader(L"Shaders//PropagateLightCS.hlsl", "Main", "cs_5_0", shaderDefines);

		ComputePipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pRootSignature);
		pipelineStateDesc.SetComputeShader(&computeShader);

		m_pWithoutOcclusionTestState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"PropagateLightPass::m_pWithoutOcclusionTestState");
	}
	{
		const ShaderMacro shaderDefines[] =
		{
			ShaderMacro("NUM_THREADS_X", numThreadsPerGroupXStr.c_str()),
			ShaderMacro("NUM_THREADS_Y", numThreadsPerGroupYStr.c_str()),
			ShaderMacro("NUM_THREADS_Z", numThreadsPerGroupZStr.c_str()),
			ShaderMacro("TEST_OCCLUSION", "1"),
			ShaderMacro()
		};
		Shader computeShader(L"Shaders//PropagateLightCS.hlsl", "Main", "cs_5_0", shaderDefines);

		ComputePipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pRootSignature);
		pipelineStateDesc.SetComputeShader(&computeShader);

		m_pWithOcclusionTestState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"PropagateLightPass::m_pWithOcclusionTestState");
	}
}

PropagateLightPass::~PropagateLightPass()
{
	SafeDelete(m_pWithoutOcclusionTestState);
	SafeDelete(m_pWithOcclusionTestState);
	SafeDelete(m_pRootSignature);
}

void PropagateLightPass::Record(RenderParams* pParams)
{
	assert(false && "Kolya. Fix Me");
	/*
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	ResourceList** ppResources = pParams->m_ppResources;
			
	std::vector<ResourceBarrier> resourceBarriers[2];
	for (u8 it = 0; it < 2; ++it)
	{
		ResourceStateList requiredResourceStates = ppResources[it]->m_RequiredResourceStates;

		resourceBarriers[it].reserve(6);
		for (u8 resourceIndex = 1; resourceIndex < 4; ++resourceIndex)
		{
			GraphicsResource* pResource = requiredResourceStates[resourceIndex].m_pResource;
			resourceBarriers[it].emplace_back(pResource, pResource->GetWriteState(), pResource->GetReadState());
		}
		for (u8 resourceIndex = 4; resourceIndex < 7; ++resourceIndex)
		{
			GraphicsResource* pResource = requiredResourceStates[resourceIndex].m_pResource;
			resourceBarriers[it].emplace_back(pResource, pResource->GetReadState(), pResource->GetWriteState());
		}
	}
	
	pCommandList->Begin();
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	{
		ResourceList* pResources = ppResources[0];
			
		pCommandList->SetPipelineState(m_pWithoutOcclusionTestState);
		pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
		pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);
		pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, m_NumThreadGroupsZ);
	}

	if (pParams->m_NumIterations > 1)
	{
		pCommandList->SetPipelineState(m_pWithOcclusionTestState);
		for (u16 it = 1; it < pParams->m_NumIterations; ++it)
		{
			const u16 resourceIndex = it % 2;
			ResourceList* pResources = ppResources[resourceIndex];

			pCommandList->ResourceBarrier(resourceBarriers[resourceIndex].size(), resourceBarriers[resourceIndex].data());
			pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);
			pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, m_NumThreadGroupsZ);
		}
	}

	if ((pParams->m_NumIterations % 2) == 0)
		pCommandList->ResourceBarrier(resourceBarriers[0].size(), resourceBarriers[0].data());

	pCommandList->End();
	*/
}
