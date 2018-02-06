#include "RenderPasses/VisualizeVoxelReflectancePass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/GPUProfiler.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"

namespace
{
	enum RootParams
	{
		kRootCBVParamPS = 0,
		kRootSRVTableParamPS,
		kNumRootParams
	};
}

VisualizeVoxelReflectancePass::VisualizeVoxelReflectancePass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

VisualizeVoxelReflectancePass::~VisualizeVoxelReflectancePass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void VisualizeVoxelReflectancePass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;
	
	pCommandList->Begin(m_pPipelineState);
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_PROFILING

	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());

	pCommandList->SetGraphicsRootConstantBufferView(kRootCBVParamPS, pParams->m_pAppDataBuffer);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParamPS, m_SRVHeapStartPS);
	
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

void VisualizeVoxelReflectancePass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	m_OutputResourceStates.m_BackBufferState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	m_OutputResourceStates.m_DepthTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_VoxelReflectanceTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	assert(m_ResourceBarriers.empty());
	AddResourceBarrierIfRequired(pParams->m_pBackBuffer,
		pParams->m_InputResourceStates.m_BackBufferState,
		m_OutputResourceStates.m_BackBufferState);

	AddResourceBarrierIfRequired(pParams->m_pDepthTexture,
		pParams->m_InputResourceStates.m_DepthTextureState,
		m_OutputResourceStates.m_DepthTextureState);

	AddResourceBarrierIfRequired(pParams->m_pVoxelReflectanceTexture,
		pParams->m_InputResourceStates.m_VoxelReflectanceTextureState,
		m_OutputResourceStates.m_VoxelReflectanceTextureState);

	m_SRVHeapStartPS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartPS,
		pParams->m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pVoxelReflectanceTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_RTVHeapStart = pParams->m_pBackBuffer->GetRTVHandle();
}

void VisualizeVoxelReflectancePass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRootCBVParamPS] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
	
	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {SRVDescriptorRange(2, 0)};
	rootParams[kRootSRVTableParamPS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_PIXEL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"VisualizeVoxelReflectancePass::m_pRootSignature");
}

void VisualizeVoxelReflectancePass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	Shader vertexShader(L"Shaders//FullScreenTriangleVS.hlsl", "Main", "vs_4_0");
	Shader pixelShader(L"Shaders//VisualizeVoxelReflectancePS.hlsl", "Main", "ps_4_0");

	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SetRenderTargetFormat(GetRenderTargetViewFormat(pParams->m_pBackBuffer->GetFormat()));

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"VisualizeVoxelReflectancePass::m_pPipelineState");
}

void VisualizeVoxelReflectancePass::AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
