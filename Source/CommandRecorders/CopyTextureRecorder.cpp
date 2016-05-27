#include "CommandRecorders/CopyTextureRecorder.h"
#include "DX/DXCommandList.h"
#include "DX/DXRenderEnvironment.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXUtils.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

CopyTextureRecorder::CopyTextureRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	DXRenderEnvironment* pEnv = pParams->m_pEnv;

	DXShader vertexShader(L"Shaders//FullScreenTriangleVS.hlsl", "Main", "vs_4_0");
	DXShader pixelShader(L"Shaders//CopyTexturePS.hlsl", "Main", "ps_4_0");

	DXSRVRange srvRange(1, 0);
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = DXRootDescriptorTableParameter(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

	DXStaticSamplerDesc samplerDesc(DXStaticSamplerDesc::Linear, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	
	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(pEnv->m_pDevice, &rootSignatureDesc, L"CopyTextureRecorder::m_pRootSignature");
	
	DXGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SetRenderTargetFormat(pParams->m_RTVFormat);

	m_pPipelineState = new DXPipelineState(pEnv->m_pDevice, &pipelineStateDesc, L"CopyTextureRecorder::m_pPipelineState");
}

CopyTextureRecorder::~CopyTextureRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void CopyTextureRecorder::Record(RenderPassParams* pParams)
{
	DXRenderEnvironment* pEnv = pParams->m_pEnv;
	DXCommandList* pCommandList = pParams->m_pCommandList;
	DXBindingResourceList* pResources = pParams->m_pResources;
	
	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	pCommandList->SetResourceTransitions(&pResources->m_ResourceTransitions);
	pCommandList->SetDescriptorHeaps(pEnv->m_pShaderVisibleSRVHeap);
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
