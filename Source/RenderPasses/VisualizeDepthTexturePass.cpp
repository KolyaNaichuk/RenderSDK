#include "RenderPasses/VisualizeDepthTexturePass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "Profiler/GPUProfiler.h"

namespace
{
	enum RootParams
	{
		kRoot32BitConstantsParam = 0,
		kRootSRVTableParam,
		kNumRootParams
	};
}

VisualizeDepthTexturePass::VisualizeDepthTexturePass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

VisualizeDepthTexturePass::~VisualizeDepthTexturePass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void VisualizeDepthTexturePass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;

	pCommandList->Begin(m_pPipelineState);
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_PROFILING

	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetGraphicsRoot32BitConstants(kRoot32BitConstantsParam, sizeof(CameraSettings) / sizeof(f32), &pParams->m_CameraSettings, 0);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStart);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = m_RTVHeapStart;
	pCommandList->OMSetRenderTargets(1, &rtvHeapStart);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, nullptr);
	pCommandList->IASetIndexBuffer(nullptr);

	Rect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetViewports(1, pParams->m_pViewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->DrawInstanced(3, 1, 0, 0);

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();
}

void VisualizeDepthTexturePass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	m_OutputResourceStates.m_InputTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_BackBufferState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	assert(m_ResourceBarriers.empty());
	AddResourceBarrierIfRequired(pParams->m_pInputTexture,
		pParams->m_InputResourceStates.m_InputTextureState,
		m_OutputResourceStates.m_InputTextureState);

	AddResourceBarrierIfRequired(pParams->m_pBackBuffer,
		pParams->m_InputResourceStates.m_BackBufferState,
		m_OutputResourceStates.m_BackBufferState);

	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		pParams->m_InputTextureSRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_RTVHeapStart = pParams->m_pBackBuffer->GetRTVHandle();
}

void VisualizeDepthTexturePass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRoot32BitConstantsParam] = Root32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_PIXEL, sizeof(CameraSettings) / sizeof(f32));
	
	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {SRVDescriptorRange(1, 0)};
	rootParams[kRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_PIXEL);

	StaticSamplerDesc samplerDesc(StaticSamplerDesc::Point, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"VisualizeDepthTexturePass::m_pRootSignature");
}

void VisualizeDepthTexturePass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);
	
	Shader vertexShader(L"Shaders//FullScreenTriangleVS.hlsl", L"Main", L"vs_6_1");
	Shader pixelShader(L"Shaders//VisualizeDepthTexturePS.hlsl", L"Main", L"ps_6_1");

	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SetRenderTargetFormat(GetRenderTargetViewFormat(pParams->m_pBackBuffer->GetFormat()));

	m_pPipelineState = new PipelineState(pParams->m_pRenderEnv->m_pDevice, &pipelineStateDesc, L"VisualizeDepthTexturePass::m_pPipelineState");
}

void VisualizeDepthTexturePass::AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
