#include "RenderPasses/DownscaleAndReprojectDepthPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Math/Math.h"

namespace
{
	enum ReprojectRootParams
	{
		kReprojectRootCBVParam = 0,
		kReprojectRootSRVTableParam,
		kReprojectNumRootParams
	};
	enum CopyRootParams
	{
		kCopyRootSRVTableParam = 0,
		kCopyNumRootParams
	};
}

DownscaleAndReprojectDepthPass::DownscaleAndReprojectDepthPass(InitParams* pParams)
	: m_pReprojectRootSignature(nullptr)
	, m_pReprojectPipelineState(nullptr)
	, m_pReprojectionColorTexture(nullptr)
	, m_NumThreadGroupsX(0)
	, m_NumThreadGroupsY(0)
	, m_pCopyRootSignature(nullptr)
	, m_pCopyPipelineState(nullptr)
	, m_pReprojectionDepthTexture(nullptr)
	, m_pCopyViewport(nullptr)
{
	const UINT64 reprojectedDepthTextureWidth = pParams->m_pPrevDepthTexture->GetWidth() / 4;
	const UINT reprojectedDepthTextureHeight = pParams->m_pPrevDepthTexture->GetHeight() / 4;

	InitReprojectResources(pParams, reprojectedDepthTextureWidth, reprojectedDepthTextureHeight);
	InitReprojectRootSignature(pParams);
	InitReprojectPipelineState(pParams, reprojectedDepthTextureWidth, reprojectedDepthTextureHeight);

	InitCopyResources(pParams, reprojectedDepthTextureWidth, reprojectedDepthTextureHeight);
	InitCopyRootSignature(pParams);
	InitCopyPipelineState(pParams);
}

DownscaleAndReprojectDepthPass::~DownscaleAndReprojectDepthPass()
{
	SafeDelete(m_pCopyViewport);
	SafeDelete(m_pReprojectionDepthTexture);
	SafeDelete(m_pCopyRootSignature);
	SafeDelete(m_pCopyPipelineState);
	
	SafeDelete(m_pReprojectionColorTexture);
	SafeDelete(m_pReprojectRootSignature);
	SafeDelete(m_pReprojectPipelineState);
}

void DownscaleAndReprojectDepthPass::InitReprojectResources(InitParams* pParams, UINT64 reprojectedDepthTextureWidth, UINT reprojectedDepthTextureHeight)
{
	assert(m_ReprojectResourceBarriers.empty());
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
			
	ColorTexture2DDesc reprojectionColorTextureDesc(DXGI_FORMAT_R32_UINT, reprojectedDepthTextureWidth, reprojectedDepthTextureHeight, false, true, true);
	m_pReprojectionColorTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &reprojectionColorTextureDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, L"DownscaleAndReprojectDepthPass::m_pReprojectionColorTexture");

	m_OutputResourceStates.m_PrevDepthTextureState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_ReprojectedDepthTextureState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	
	CreateReprojectResourceBarrierIfRequired(pParams->m_pPrevDepthTexture,
		pParams->m_InputResourceStates.m_PrevDepthTextureState,
		m_OutputResourceStates.m_PrevDepthTextureState);

	CreateReprojectResourceBarrierIfRequired(m_pReprojectionColorTexture,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	m_ReprojectSRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_ReprojectSRVHeapStart,
		pParams->m_pPrevDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pReprojectionColorTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DownscaleAndReprojectDepthPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;

	pCommandList->Begin();
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	{
		pCommandList->SetPipelineState(m_pReprojectPipelineState);
		pCommandList->SetComputeRootSignature(m_pReprojectRootSignature);

		if (!m_ReprojectResourceBarriers.empty())
			pCommandList->ResourceBarrier(m_ReprojectResourceBarriers.size(), m_ReprojectResourceBarriers.data());

		const UINT clearDepthValue[4] = {0, 0, 0, 0};
		DescriptorHandle reprojectedDepthTextureHandle(m_ReprojectSRVHeapStart, 1);
		pCommandList->ClearUnorderedAccessView(reprojectedDepthTextureHandle, m_pReprojectionColorTexture->GetUAVHandle(), m_pReprojectionColorTexture, clearDepthValue);
		pCommandList->SetComputeRootConstantBufferView(kReprojectRootCBVParam, pParams->m_pAppDataBuffer);
		pCommandList->SetComputeRootDescriptorTable(kReprojectRootSRVTableParam, m_ReprojectSRVHeapStart);

		pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);
	}
	{
		pCommandList->SetPipelineState(m_pCopyPipelineState);
		pCommandList->SetGraphicsRootSignature(m_pCopyRootSignature);

		if (!m_CopyResourceBarriers.empty())
			pCommandList->ResourceBarrier(m_CopyResourceBarriers.size(), m_CopyResourceBarriers.data());

		pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
		pCommandList->SetGraphicsRootDescriptorTable(kCopyRootSRVTableParam, m_CopySRVHeapStart);

		D3D12_CPU_DESCRIPTOR_HANDLE copyDSVHeapStart = m_CopyDSVHeapStart;
		pCommandList->OMSetRenderTargets(0, nullptr, FALSE, &copyDSVHeapStart);

		pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pCommandList->IASetVertexBuffers(0, 1, nullptr);
		pCommandList->IASetIndexBuffer(nullptr);

		Rect copyScissorRect(ExtractRect(m_pCopyViewport));
		pCommandList->RSSetViewports(1, m_pCopyViewport);
		pCommandList->RSSetScissorRects(1, &copyScissorRect);
		
		pCommandList->DrawInstanced(3, 1, 0, 0);
	}

	pCommandList->End();
}

void DownscaleAndReprojectDepthPass::InitReprojectRootSignature(InitParams* pParams)
{
	assert(m_pReprojectRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kReprojectNumRootParams];
	rootParams[kReprojectRootCBVParam] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = { SRVDescriptorRange(1, 0), UAVDescriptorRange(1, 0) };
	rootParams[kReprojectRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	StaticSamplerDesc samplerDesc(StaticSamplerDesc::MaxPoint, 0, D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kReprojectNumRootParams, rootParams, 1, &samplerDesc);
	m_pReprojectRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"DownscaleAndReprojectDepthPass::m_pRootSignature");
}

void DownscaleAndReprojectDepthPass::InitReprojectPipelineState(InitParams* pParams, UINT64 reprojectedDepthTextureWidth, UINT reprojectedDepthTextureHeight)
{
	assert(m_pReprojectRootSignature != nullptr);
	assert(m_pReprojectPipelineState == nullptr);

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
	pipelineStateDesc.SetRootSignature(m_pReprojectRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pReprojectPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"DownscaleAndReprojectDepthPass::m_pPipelineState");
}

void DownscaleAndReprojectDepthPass::CreateReprojectResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ReprojectResourceBarriers.emplace_back(pResource, currState, requiredState);
}

void DownscaleAndReprojectDepthPass::InitCopyResources(InitParams* pParams, UINT64 reprojectedDepthTextureWidth, UINT reprojectedDepthTextureHeight)
{
	assert(m_CopyResourceBarriers.empty());
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	DepthStencilValue optimizedClearDepth(1.0f);
	DepthTexture2DDesc reprojectionDepthTextureDesc(DXGI_FORMAT_R32_TYPELESS, reprojectedDepthTextureWidth, reprojectedDepthTextureHeight, true, true);
	m_pReprojectionDepthTexture = new DepthTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &reprojectionDepthTextureDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearDepth, L"DownscaleAndReprojectDepthPass::m_pReprojectionDepthTexture");

	CreateCopyResourceBarrierIfRequired(m_pReprojectionDepthTexture,
		pParams->m_InputResourceStates.m_ReprojectedDepthTextureState,
		m_OutputResourceStates.m_ReprojectedDepthTextureState);

	CreateCopyResourceBarrierIfRequired(m_pReprojectionColorTexture,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_CopySRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_CopySRVHeapStart,
		m_pReprojectionColorTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_CopyDSVHeapStart = m_pReprojectionDepthTexture->GetDSVHandle();
}

void DownscaleAndReprojectDepthPass::InitCopyRootSignature(InitParams* pParams)
{
	assert(m_pCopyRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kCopyNumRootParams];

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = { SRVDescriptorRange(1, 0) };
	rootParams[kCopyRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), &descriptorRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);

	RootSignatureDesc rootSignatureDesc(kCopyNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pCopyRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"DownscaleAndReprojectDepthPass::m_pCopyRootSignature");
}

void DownscaleAndReprojectDepthPass::InitCopyPipelineState(InitParams* pParams)
{
	assert(m_pCopyRootSignature != nullptr);
	assert(m_pCopyPipelineState == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	Shader vertexShader(L"Shaders//FullScreenTriangleVS.hlsl", "Main", "vs_4_0");
	Shader pixelShader(L"Shaders//CopyReprojectedDepthPS.hlsl", "Main", "ps_4_0");

	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pCopyRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.DepthStencilState = DepthStencilDesc(DepthStencilDesc::Always);
	pipelineStateDesc.SetRenderTargetFormats(0, nullptr, m_pReprojectionDepthTexture->GetFormat());

	m_pCopyPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"DownscaleAndReprojectDepthPass::m_pCopyPipelineState");
}

void DownscaleAndReprojectDepthPass::CreateCopyResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_CopyResourceBarriers.emplace_back(pResource, currState, requiredState);
}
