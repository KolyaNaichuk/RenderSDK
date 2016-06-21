#include "CommandRecorders/VisualizeVoxelGridRecorder.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXCommandList.h"
#include "DX/DXResource.h"
#include "DX/DXDescriptorHeap.h"
#include "DX/DXRenderEnvironment.h"
#include "DX/DXUtils.h"

enum RootParams
{
	kSRVRootParam,
	kNumRootParams
};

VisualizeVoxelGridRecorder::VisualizeVoxelGridRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;

	DXShader vertexShader(L"Shaders//FullScreenTriangleVS.hlsl", "Main", "vs_4_0");
	DXShader pixelShader(L"Shaders//VisualizeVoxelGridPS.hlsl", "Main", "ps_4_0");

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {DXCBVRange(2, 0), DXSRVRange(2, 0)};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = DXRootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), &descriptorRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);
	
	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"VisualizeVoxelGridRecorder::m_pRootSignature");

	DXGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SetRenderTargetFormat(pParams->m_RTVFormat);

	m_pPipelineState = new DXPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"VisualizeVoxelGridRecorder::m_pPipelineState");
}

VisualizeVoxelGridRecorder::~VisualizeVoxelGridRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void VisualizeVoxelGridRecorder::Record(RenderPassParams* pParams)
{
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;
	DXCommandList* pCommandList = pParams->m_pCommandList;
	DXBindingResourceList* pResources = pParams->m_pResources;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	pCommandList->SetResourceTransitions(&pResources->m_ResourceTransitions);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = pResources->m_RTVHeapStart;
	pCommandList->OMSetRenderTargets(1, &rtvHeapStart);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, nullptr);
	pCommandList->IASetIndexBuffer(nullptr);

	pCommandList->RSSetViewports(1, pParams->m_pViewport);

	DXRect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->DrawInstanced(3, 1, 0, 0);
	pCommandList->Close();
}
