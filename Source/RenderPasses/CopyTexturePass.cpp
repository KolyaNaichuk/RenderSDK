#include "RenderPasses/CopyTexturePass.h"
#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DRenderEnv.h"
#include "D3DWrapper/D3DPipelineState.h"
#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DUtils.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

CopyTexturePass::CopyTexturePass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	D3DShader vertexShader(L"Shaders//FullScreenTriangleVS.hlsl", "Main", "vs_4_0");
	D3DShader pixelShader(L"Shaders//CopyTexturePS.hlsl", "Main", "ps_4_0");

	D3DSRVRange srvRange(1, 0);
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = D3DRootDescriptorTableParameter(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

	D3DStaticSamplerDesc samplerDesc(D3DStaticSamplerDesc::Linear, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	
	D3DRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new D3DRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"CopyTexturePass::m_pRootSignature");
	
	D3DGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SetRenderTargetFormat(pParams->m_RTVFormat);

	m_pPipelineState = new D3DPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CopyTexturePass::m_pPipelineState");
}

CopyTexturePass::~CopyTexturePass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void CopyTexturePass::Record(RenderParams* pParams)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	D3DCommandList* pCommandList = pParams->m_pCommandList;
	D3DResourceList* pResources = pParams->m_pResources;
	
	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	pCommandList->SetResourceTransitions(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = pResources->m_RTVHeapStart;
	pCommandList->OMSetRenderTargets(1, &rtvHeapStart);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	
	pCommandList->IASetVertexBuffers(0, 1, nullptr);
	pCommandList->IASetIndexBuffer(nullptr);

	pCommandList->RSSetViewports(1, pParams->m_pViewport);

	D3DRect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->DrawInstanced(3, 1, 0, 0);
	pCommandList->Close();
}
