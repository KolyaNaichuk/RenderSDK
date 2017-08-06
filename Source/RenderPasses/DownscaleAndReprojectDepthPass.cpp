#include "RenderPasses/DownscaleAndReprojectDepthPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Math/Math.h"

namespace
{
	enum RootParams
	{
		kRootCBVParam = 0,
		kRootSRVTableParam,
		kNumRootParams
	};
}

DownscaleAndReprojectDepthPass::DownscaleAndReprojectDepthPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pReprojectedDepthTexture(nullptr)
	, m_NumThreadGroupsX(0)
	, m_NumThreadGroupsY(0)
{
	const UINT64 reprojectedDepthTextureWidth = pParams->m_pPrevDepthTexture->GetWidth() / 4;
	const UINT reprojectedDepthTextureHeight = pParams->m_pPrevDepthTexture->GetHeight() / 4;

	InitResources(pParams, reprojectedDepthTextureWidth, reprojectedDepthTextureHeight);
	InitRootSignature(pParams);
	InitPipelineState(pParams, reprojectedDepthTextureWidth, reprojectedDepthTextureHeight);
}

DownscaleAndReprojectDepthPass::~DownscaleAndReprojectDepthPass()
{
	SafeDelete(m_pReprojectedDepthTexture);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}

void DownscaleAndReprojectDepthPass::InitResources(InitParams* pParams, UINT64 reprojectedDepthTextureWidth, UINT reprojectedDepthTextureHeight)
{
	assert(m_ResourceBarriers.empty());
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
			
	ColorTexture2DDesc reprojectedDepthTextureDesc(DXGI_FORMAT_R32_UINT, reprojectedDepthTextureWidth, reprojectedDepthTextureHeight, false, true, true);
	m_pReprojectedDepthTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &reprojectedDepthTextureDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, L"DownscaleAndReprojectDepthPass::m_pReprojectedDepthTexture");

	m_OutputResourceStates.m_PrevDepthTextureState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_ReprojectedDepthTextureState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	CreateResourceBarrierIfRequired(pParams->m_pPrevDepthTexture,
		pParams->m_InputResourceStates.m_PrevDepthTextureState,
		m_OutputResourceStates.m_PrevDepthTextureState);

	CreateResourceBarrierIfRequired(m_pReprojectedDepthTexture,
		pParams->m_InputResourceStates.m_ReprojectedDepthTextureState,
		m_OutputResourceStates.m_ReprojectedDepthTextureState);

	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		pParams->m_pPrevDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pReprojectedDepthTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DownscaleAndReprojectDepthPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);

	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier(m_ResourceBarriers.size(), m_ResourceBarriers.data());

	const UINT clearValues[4] = {0, 0, 0, 0};
	DescriptorHandle reprojectedDepthTextureHandle(m_SRVHeapStart, 1);
	pCommandList->ClearUnorderedAccessView(reprojectedDepthTextureHandle, m_pReprojectedDepthTexture->GetUAVHandle(), m_pReprojectedDepthTexture, clearValues);
	
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootConstantBufferView(kRootCBVParam, pParams->m_pAppDataBuffer);
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStart);
	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);
	pCommandList->End();
}

void DownscaleAndReprojectDepthPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRootCBVParam] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = { SRVDescriptorRange(1, 0), UAVDescriptorRange(1, 0) };
	rootParams[kRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	StaticSamplerDesc samplerDesc(StaticSamplerDesc::MaxPoint, 0, D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, 1, &samplerDesc);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"DownscaleAndReprojectDepthPass::m_pRootSignature");
}

void DownscaleAndReprojectDepthPass::InitPipelineState(InitParams* pParams, UINT64 reprojectedDepthTextureWidth, UINT reprojectedDepthTextureHeight)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const u32 numThreads = 8;
	m_NumThreadGroupsX = (u32)Ceil((f32)reprojectedDepthTextureWidth / (f32)numThreads);
	m_NumThreadGroupsY = (u32)Ceil((f32)reprojectedDepthTextureHeight / (f32)numThreads);

	std::string numThreadsStr = std::to_string(numThreads);
	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("NUM_THREADS_X", numThreadsStr.c_str()),
		ShaderMacro("NUM_THREADS_Y", numThreadsStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//DownscaleAndReprojectDepthCS.hlsl", "Main", "cs_5_0", shaderDefines);

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"DownscaleAndReprojectDepthPass::m_pPipelineState");
}

void DownscaleAndReprojectDepthPass::CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
