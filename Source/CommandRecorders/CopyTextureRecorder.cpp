#include "CommandRecorders/CopyTextureRecorder.h"
#include "DX/DXCommandList.h"
#include "DX/DXResource.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXUtils.h"

enum RootParams
{
	kSRVRootParam = 0,
	kSamplerRootParam,
	kNumRootParams
};

CopyTextureRecorder::CopyTextureRecorder(DXDevice* pDevice, DXGI_FORMAT rtvFormat)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	DXShader vertexShader(L"Shaders//FullScreenTriangleVS.hlsl", "Main", "vs_4_0");
	DXShader pixelShader(L"Shaders//CopyTexturePS.hlsl", "Main", "ps_4_0");

	DXSRVRange srvRange(1, 0);
	DXSamplerRange samplerRange(1, 0);
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = DXRootDescriptorTableParameter(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kSamplerRootParam] = DXRootDescriptorTableParameter(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(pDevice, &rootSignatureDesc, L"CopyTextureRecorder::m_pRootSignature");
	
	DXGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SetRenderTargetFormat(rtvFormat);

	m_pPipelineState = new DXPipelineState(pDevice, &pipelineStateDesc, L"CopyTextureRecorder::m_pPipelineState");
}

CopyTextureRecorder::~CopyTextureRecorder()
{
	delete m_pPipelineState;
	delete m_pRootSignature;
}

void CopyTextureRecorder::Record(DXCommandList* pCommandList, DXCommandAllocator* pCommandAllocator,
	DXResource* pRTVTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor,
	ID3D12DescriptorHeap* pSRVDescriptorHeap, DXResource* pSRVTexture, D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor,
	ID3D12DescriptorHeap* pSamplerDescriptorHeap, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor,
	const D3D12_RESOURCE_STATES* pRTVEndState,
	const D3D12_RESOURCE_STATES* pSRVEndState)
{
	pCommandList->Reset(pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	
	if (pRTVTexture->GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
		pCommandList->TransitionBarrier(pRTVTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (pSRVTexture->GetState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		pCommandList->TransitionBarrier(pSRVTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	pCommandList->OMSetRenderTargets(1, &rtvDescriptor);
	pCommandList->SetDescriptorHeaps(1, &pSRVDescriptorHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParam, srvDescriptor);
	pCommandList->SetDescriptorHeaps(1, &pSamplerDescriptorHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kSamplerRootParam, samplerDescriptor);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	
	
	DXVertexBufferView* pVertexBufferView = nullptr;
	pCommandList->IASetVertexBuffers(0, 1, pVertexBufferView);
	
	DXIndexBufferView* pIndexBufferView = nullptr;
	pCommandList->IASetIndexBuffer(pIndexBufferView);

	DXViewport viewport(0.0f, 0.0f, FLOAT(pRTVTexture->GetWidth()), FLOAT(pRTVTexture->GetHeight()));
	pCommandList->RSSetViewports(1, &viewport);

	DXRect scissorRect(ExtractRect(viewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->DrawInstanced(3, 1, 0, 0);

	if (pRTVEndState != nullptr)
		pCommandList->TransitionBarrier(pRTVTexture, *pRTVEndState);
	if (pSRVEndState != nullptr)
		pCommandList->TransitionBarrier(pSRVTexture, *pSRVEndState);

	pCommandList->Close();
}
