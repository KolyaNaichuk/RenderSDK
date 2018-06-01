#include "RenderPasses/FilterExpShadowMapPass.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/RenderEnv.h"
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
	: m_Name(pParams->m_pName)
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
	assert(false);
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
	const std::string numThreadsStr = std::to_string(numThreads);

	m_NumThreadGroupsX = (u32)Ceil((f32)pExpShadowMaps->GetWidth() / (f32)numThreads);
	m_NumThreadGroupsY = m_NumThreadGroupsX;

	{
		assert(m_pPipelineStateX == nullptr);
		const ShaderMacro shaderDefines[] =
		{
			ShaderMacro("FILTER_X", "1"),
			ShaderMacro("NUM_THREADS", numThreadsStr.c_str()),
			ShaderMacro()
		};
		Shader computeShader(L"Shaders//FilterExpShadowMapCS.hlsl", "Main", "cs_5_0", shaderDefines);

		ComputePipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pRootSignature);
		pipelineStateDesc.SetComputeShader(&computeShader);

		m_pPipelineStateX = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"FilterExpShadowMapPass::m_pPipelineStateX");
	}
	{
		assert(m_pPipelineStateY == nullptr);
		const ShaderMacro shaderDefines[] =
		{
			ShaderMacro("FILTER_Y", "1"),
			ShaderMacro("NUM_THREADS", numThreadsStr.c_str()),
			ShaderMacro()
		};
		Shader computeShader(L"Shaders//FilterExpShadowMapCS.hlsl", "Main", "cs_5_0", shaderDefines);

		ComputePipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pRootSignature);
		pipelineStateDesc.SetComputeShader(&computeShader);

		m_pPipelineStateY = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"FilterExpShadowMapPass::m_pPipelineStateY");
	}
}
