#include "RenderPasses/FillMeshTypeDepthBufferPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/PipelineState.h"

namespace
{
	enum RootParams
	{
		kRoot32BitConstantParam = 0,
		kRootSRVTableParam,
		kNumRootParams
	};
}

FillMeshTypeDepthBufferPass::FillMeshTypeDepthBufferPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pMeshTypeDepthTexture(nullptr)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

FillMeshTypeDepthBufferPass::~FillMeshTypeDepthBufferPass()
{
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pMeshTypeDepthTexture);
}

void FillMeshTypeDepthBufferPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	
	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetGraphicsRoot32BitConstant(kRoot32BitConstantParam, pParams->m_NumMeshTypes, 0);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStart);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = m_DSVHeapStart;
	pCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHeapStart);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, nullptr);
	pCommandList->IASetIndexBuffer(nullptr);

	Rect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetViewports(1, pParams->m_pViewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->DrawInstanced(3, 1, 0, 0);
	pCommandList->End();
}

void FillMeshTypeDepthBufferPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	m_OutputResourceStates.m_MaterialIDTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_MeshTypePerMaterialIDBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_MeshTypeDepthTextureState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	DepthStencilValue optimizedClearDepth(1.0f);

	assert(m_pMeshTypeDepthTexture == nullptr);
	DepthTexture2DDesc depthTextureDesc(DXGI_FORMAT_R16_TYPELESS, pParams->m_pMaterialIDTexture->GetWidth(), pParams->m_pMaterialIDTexture->GetHeight(), true, true);
	m_pMeshTypeDepthTexture = new DepthTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &depthTextureDesc,
		pParams->m_InputResourceStates.m_MeshTypeDepthTextureState, &optimizedClearDepth, L"FillMeshTypeDepthBufferPass::m_pMeshTypeDepthTexture");

	assert(m_ResourceBarriers.empty());
	AddResourceBarrierIfRequired(pParams->m_pMaterialIDTexture,
		pParams->m_InputResourceStates.m_MaterialIDTextureState,
		m_OutputResourceStates.m_MaterialIDTextureState);

	AddResourceBarrierIfRequired(pParams->m_pMeshTypePerMaterialIDBuffer,
		pParams->m_InputResourceStates.m_MeshTypePerMaterialIDBufferState,
		m_OutputResourceStates.m_MeshTypePerMaterialIDBufferState);

	AddResourceBarrierIfRequired(m_pMeshTypeDepthTexture,
		pParams->m_InputResourceStates.m_MeshTypeDepthTextureState,
		m_OutputResourceStates.m_MeshTypeDepthTextureState);

	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		pParams->m_pMaterialIDTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pMeshTypePerMaterialIDBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_DSVHeapStart = m_pMeshTypeDepthTexture->GetDSVHandle();
}

void FillMeshTypeDepthBufferPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRoot32BitConstantParam] = Root32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_PIXEL, 1);

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {SRVDescriptorRange(2, 0)};
	rootParams[kRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_PIXEL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"FillMeshTypeDepthBufferPass::m_pRootSignature");
}

void FillMeshTypeDepthBufferPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	Shader vertexShader(L"Shaders//FullScreenTriangleVS.hlsl", "Main", "vs_4_0");
	Shader pixelShader(L"Shaders//FillMeshTypeDepthBufferPS.hlsl", "Main", "ps_4_0");

	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.DepthStencilState = DepthStencilDesc(DepthStencilDesc::Always);
	pipelineStateDesc.SetRenderTargetFormats(0, nullptr, GetDepthStencilViewFormat(m_pMeshTypeDepthTexture->GetFormat()));

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"FillMeshTypeDepthBufferPass::m_pPipelineState");
}

void FillMeshTypeDepthBufferPass::AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
