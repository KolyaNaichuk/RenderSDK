#include "RenderPasses/CreateExpShadowMapPass.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/RenderEnv.h"
#include "Profiler/GPUProfiler.h"
#include "Math/Math.h"

namespace
{
	enum RootParams
	{
		kRoot32BitConstantsParam,
		kRootSRVTableParam,
		kNumRootParams
	};
}

CreateExpShadowMapPass::CreateExpShadowMapPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

CreateExpShadowMapPass::~CreateExpShadowMapPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void CreateExpShadowMapPass::Record(RenderParams* pParams)
{
	const ResourceTransitionBarrier resourceBarriers[] =
	{
		ResourceTransitionBarrier(pParams->m_pStandardShadowMaps,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			m_OutputResourceStates.m_StandardShadowMapsState,
			pParams->m_StandardShadowMapIndex),

		ResourceTransitionBarrier(pParams->m_pExpShadowMaps,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			m_OutputResourceStates.m_ExpShadowMapsState,
			pParams->m_ExpShadowMapIndex)
	};

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_PROFILING

	const UINT constants32Bit[] = {pParams->m_StandardShadowMapIndex, pParams->m_ExpShadowMapIndex};
	
	pCommandList->SetPipelineState(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->ResourceBarrier(ARRAYSIZE(resourceBarriers), resourceBarriers);
	pCommandList->SetComputeRoot32BitConstants(kRoot32BitConstantsParam, ARRAYSIZE(constants32Bit), constants32Bit, 0);
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStart);
	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
}

void CreateExpShadowMapPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(pParams->m_InputResourceStates.m_StandardShadowMapsState == D3D12_RESOURCE_STATE_DEPTH_WRITE);
	assert(pParams->m_InputResourceStates.m_CreateExpShadowMapParamsBufferState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	assert(pParams->m_InputResourceStates.m_ExpShadowMapsState == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	
	m_OutputResourceStates.m_StandardShadowMapsState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_CreateExpShadowMapParamsBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_ExpShadowMapsState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	
	assert(!m_SRVHeapStart.IsValid());
	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();

	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		pParams->m_pStandardShadowMaps->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pCreateExpShadowMapParamsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pExpShadowMaps->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CreateExpShadowMapPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRoot32BitConstantsParam] = Root32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_ALL, 2);

	const D3D12_DESCRIPTOR_RANGE descriptorRanges[] =
	{
		SRVDescriptorRange(2, 0),
		UAVDescriptorRange(1, 0)
	};
	rootParams[kRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);
	
	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"CreateExpShadowMapPass::m_pRootSignature");
}

void CreateExpShadowMapPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pPipelineState == nullptr);
	assert(m_pRootSignature != nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	DepthTexture* pStandardShadowMaps = pParams->m_pStandardShadowMaps;
	ColorTexture* pExpShadowMaps = pParams->m_pExpShadowMaps;

	assert(pStandardShadowMaps->GetWidth() == pStandardShadowMaps->GetHeight());
	assert(pExpShadowMaps->GetWidth() == pExpShadowMaps->GetHeight());
	assert(pStandardShadowMaps->GetWidth() == pExpShadowMaps->GetWidth());
		
	const u32 numThreads = 8;
	m_NumThreadGroupsX = (u32)Ceil((f32)pExpShadowMaps->GetWidth() / (f32)numThreads);
	m_NumThreadGroupsY = m_NumThreadGroupsX;

	std::string numThreadsStr = std::to_string(numThreads);
	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("NUM_THREADS", numThreadsStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//CreateExpShadowMapCS.hlsl", "Main", "cs_5_0", shaderDefines);

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CreateExpShadowMapPass::m_pPipelineState");
}
