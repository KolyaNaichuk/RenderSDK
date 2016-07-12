#include "CommandRecorders/CalcTextureLuminanceRecorder.h"
#include "D3DWrapper/D3DPipelineState.h"
#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DResource.h"
#include "D3DWrapper/D3DUtils.h"

enum RootParams
{
	kSRVRootParam = 0,
	kSamplerRootParam,
	kNumRootParams
};

CalcTextureLuminanceRecorder::CalcTextureLuminanceRecorder(D3DDevice* pDevice, DXGI_FORMAT rtvFormat, bool logLuminance)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	D3DShader vertexShader(L"Shaders//FullScreenTriangleVS.hlsl", "Main", "vs_4_0");

	// Kolya: fix me
	/*
	const D3DShaderMacro pixelShaderDefines[] =
	{
		D3DShaderMacro("LOG_LUMINANCE", logLuminance ? "1" : "0"),
		D3DShaderMacro()
	};
	D3DShader pixelShader(L"Shaders//CalcTextureLuminancePS.hlsl", "Main", "ps_4_0", pixelShaderDefines);

	D3DSRVRange srvRange(1, 0);
	D3DSamplerRange samplerRange(1, 0);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = D3DRootDescriptorTableParameter(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kSamplerRootParam] = D3DRootDescriptorTableParameter(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL);

	D3DRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new D3DRootSignature(pDevice, &rootSignatureDesc, L"CalcTextureLuminanceRecorder::m_pRootSignature");

	D3DGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SetRenderTargetFormat(rtvFormat);

	m_pPipelineState = new D3DPipelineState(pDevice, &pipelineStateDesc, L"CalcTextureLuminanceRecorder::m_pPipelineState");
	*/
}

CalcTextureLuminanceRecorder::~CalcTextureLuminanceRecorder()
{
 	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void CalcTextureLuminanceRecorder::Record(D3DCommandList* pCommandList, D3DCommandAllocator* pCommandAllocator,
	D3DResource* pRTVTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor,
	D3DDescriptorHeap* pSRVDescriptorHeap, D3DResource* pSRVTexture, D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor,
	D3DDescriptorHeap* pSamplerDescriptorHeap, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor,
	const D3D12_RESOURCE_STATES* pRTVEndState, const D3D12_RESOURCE_STATES* pSRVEndState)
{
	// Kolya: fix me
	/*
	pCommandList->Reset(pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	if (pRTVTexture->GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
		pCommandList->TransitionBarrier(pRTVTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (pSRVTexture->GetState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		pCommandList->TransitionBarrier(pSRVTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	pCommandList->OMSetRenderTargets(1, &rtvDescriptor);
	pCommandList->SetDescriptorHeaps(pSRVDescriptorHeap, pSamplerDescriptorHeap);
	
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParam, srvDescriptor);
	pCommandList->SetGraphicsRootDescriptorTable(kSamplerRootParam, samplerDescriptor);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3DVertexBufferView* pVertexBufferView = nullptr;
	pCommandList->IASetVertexBuffers(0, 1, pVertexBufferView);

	D3DIndexBufferView* pIndexBufferView = nullptr;
	pCommandList->IASetIndexBuffer(pIndexBufferView);

	D3DViewport viewport(0.0f, 0.0f, FLOAT(pRTVTexture->GetWidth()), FLOAT(pRTVTexture->GetHeight()));
	pCommandList->RSSetViewports(1, &viewport);

	D3DRect scissorRect(ExtractRect(viewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->DrawInstanced(3, 1, 0, 0);

	if (pRTVEndState != nullptr)
		pCommandList->TransitionBarrier(pRTVTexture, *pRTVEndState);
	if (pSRVEndState != nullptr)
		pCommandList->TransitionBarrier(pSRVTexture, *pSRVEndState);

	pCommandList->Close();
	*/
}
