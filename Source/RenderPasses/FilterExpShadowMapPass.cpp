#include "RenderPasses/FilterExpShadowMapPass.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/RenderEnv.h"
#include "Profiler/GPUProfiler.h"
#include "Math/Math.h"

namespace
{
	enum RootParams
	{
		kRoot32BitConstantsParam = 0,
		kRootSRVTableParam,
		kNumRootParams
	};
}

FilterExpShadowMapPass::FilterExpShadowMapPass(InitParams* pParams)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

FilterExpShadowMapPass::~FilterExpShadowMapPass()
{
	SafeDelete(m_pPipelineStateX);
	SafeDelete(m_pPipelineStateY);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pIntermediateResults);
}

void FilterExpShadowMapPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;
	
	const UINT constants32Bit[] = {pParams->m_ExpShadowMapIndex, pParams->m_IntermediateResultIndex};
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetComputeRoot32BitConstants(kRoot32BitConstantsParam, ARRAYSIZE(constants32Bit), constants32Bit, 0);

	const ResourceTransitionBarrier resourceBarriersX[] =
	{
		ResourceTransitionBarrier(m_pIntermediateResults,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			pParams->m_IntermediateResultIndex),

		ResourceTransitionBarrier(pParams->m_pExpShadowMaps,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			pParams->m_ExpShadowMapIndex)
	};

#ifdef ENABLE_PROFILING
	u32 profileIndex1 = pGPUProfiler->StartProfile(pCommandList, "FilterExpShadowMapPass: Blur X");
#endif // ENABLE_PROFILING

	pCommandList->SetPipelineState(m_pPipelineStateX);
	pCommandList->ResourceBarrier(ARRAYSIZE(resourceBarriersX), resourceBarriersX);
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStartX);
	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex1);
#endif // ENABLE_PROFILING

	const ResourceTransitionBarrier resourceBarriersY[] =
	{
		ResourceTransitionBarrier(m_pIntermediateResults,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			pParams->m_IntermediateResultIndex),

		ResourceTransitionBarrier(pParams->m_pExpShadowMaps,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			pParams->m_ExpShadowMapIndex)
	};

#ifdef ENABLE_PROFILING
	u32 profileIndex2 = pGPUProfiler->StartProfile(pCommandList, "FilterExpShadowMapPass: Blur Y");
#endif // ENABLE_PROFILING

	pCommandList->SetPipelineState(m_pPipelineStateY);
	pCommandList->ResourceBarrier(ARRAYSIZE(resourceBarriersY), resourceBarriersY);
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStartY);
	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex2);
#endif // ENABLE_PROFILING
}

void FilterExpShadowMapPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(pParams->m_InputResourceStates.m_ExpShadowMapsState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_OutputResourceStates.m_ExpShadowMapsState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	
	assert(m_pIntermediateResults == nullptr);
	ColorTexture2DDesc intermediateResultsDesc(pParams->m_pExpShadowMaps->GetFormat(), pParams->m_pExpShadowMaps->GetWidth(), pParams->m_pExpShadowMaps->GetHeight(),
		false/*createRTV*/, true/*createSRV*/, true/*createUAV*/, 1/*mipLevels*/, pParams->m_MaxNumActiveExpShadowMaps/*arraySize*/);
	m_pIntermediateResults = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &intermediateResultsDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr/*optimizedClearColor*/, L"FilterExpShadowMapPass::m_pIntermediateResults");

	assert(!m_SRVHeapStartX.IsValid());
	m_SRVHeapStartX = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();

	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartX,
		pParams->m_pExpShadowMaps->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pIntermediateResults->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	assert(!m_SRVHeapStartY.IsValid());
	m_SRVHeapStartY = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();

	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartY,
		m_pIntermediateResults->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pExpShadowMaps->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void FilterExpShadowMapPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRoot32BitConstantsParam] = Root32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_ALL, 2);

	const D3D12_DESCRIPTOR_RANGE descriptorRanges[] =
	{
		SRVDescriptorRange(1, 0),
		UAVDescriptorRange(1, 0)
	};
	rootParams[kRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	StaticSamplerDesc staticSamplerDesc(StaticSamplerDesc::Point, 0, D3D12_SHADER_VISIBILITY_ALL);
	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, 1, &staticSamplerDesc);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"FilterExpShadowMapPass::m_pRootSignature");
}

void FilterExpShadowMapPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
		
	ColorTexture* pExpShadowMaps = pParams->m_pExpShadowMaps;
	assert(pExpShadowMaps->GetWidth() == pExpShadowMaps->GetHeight());

	const u32 numThreads = 8;
	const std::wstring numThreadsStr = std::to_wstring(numThreads);
	const std::wstring shadowMapSizeStr = std::to_wstring(pExpShadowMaps->GetWidth());

	m_NumThreadGroupsX = (u32)Ceil((f32)pExpShadowMaps->GetWidth() / (f32)numThreads);
	m_NumThreadGroupsY = m_NumThreadGroupsX;

	{
		assert(m_pPipelineStateX == nullptr);
		const ShaderDefine shaderDefines[] =
		{
			ShaderDefine(L"SHADOW_MAP_SIZE", shadowMapSizeStr.c_str()),
			ShaderDefine(L"FILTER_X", L"1"),
			ShaderDefine(L"NUM_THREADS", numThreadsStr.c_str())
		};
		Shader computeShader(L"Shaders//FilterExpShadowMapCS.hlsl", L"Main", L"cs_6_1", shaderDefines, ARRAYSIZE(shaderDefines));

		ComputePipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pRootSignature);
		pipelineStateDesc.SetComputeShader(&computeShader);

		m_pPipelineStateX = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"FilterExpShadowMapPass::m_pPipelineStateX");
	}
	{
		assert(m_pPipelineStateY == nullptr);
		const ShaderDefine shaderDefines[] =
		{
			ShaderDefine(L"SHADOW_MAP_SIZE", shadowMapSizeStr.c_str()),
			ShaderDefine(L"FILTER_Y", L"1"),
			ShaderDefine(L"NUM_THREADS", numThreadsStr.c_str())
		};
		Shader computeShader(L"Shaders//FilterExpShadowMapCS.hlsl", L"Main", L"cs_6_1", shaderDefines, ARRAYSIZE(shaderDefines));

		ComputePipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pRootSignature);
		pipelineStateDesc.SetComputeShader(&computeShader);

		m_pPipelineStateY = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"FilterExpShadowMapPass::m_pPipelineStateY");
	}
}
