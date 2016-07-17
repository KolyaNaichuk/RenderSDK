#include "RenderPasses/CalcTextureLuminancePass.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/GraphicsUtils.h"

enum RootParams
{
	kSRVRootParam = 0,
	kSamplerRootParam,
	kNumRootParams
};

CalcTextureLuminancePass::CalcTextureLuminancePass(GraphicsDevice* pDevice, DXGI_FORMAT rtvFormat, bool logLuminance)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	Shader vertexShader(L"Shaders//FullScreenTriangleVS.hlsl", "Main", "vs_4_0");

	// Kolya: fix me
	/*
	const ShaderMacro pixelShaderDefines[] =
	{
		ShaderMacro("LOG_LUMINANCE", logLuminance ? "1" : "0"),
		ShaderMacro()
	};
	Shader pixelShader(L"Shaders//CalcTextureLuminancePS.hlsl", "Main", "ps_4_0", pixelShaderDefines);

	SRVDescriptorRange srvRange(1, 0);
	SamplerRange samplerRange(1, 0);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = RootDescriptorTableParameter(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kSamplerRootParam] = RootDescriptorTableParameter(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new RootSignature(pDevice, &rootSignatureDesc, L"CalcTextureLuminancePass::m_pRootSignature");

	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SetRenderTargetFormat(rtvFormat);

	m_pPipelineState = new PipelineState(pDevice, &pipelineStateDesc, L"CalcTextureLuminancePass::m_pPipelineState");
	*/
}

CalcTextureLuminancePass::~CalcTextureLuminancePass()
{
 	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void CalcTextureLuminancePass::Record(CommandList* pCommandList,
	GraphicsResource* pRTVTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor,
	DescriptorHeap* pSRVDescriptorHeap, GraphicsResource* pSRVTexture, D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor,
	DescriptorHeap* pSamplerDescriptorHeap, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor,
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

	VertexBufferView* pVertexBufferView = nullptr;
	pCommandList->IASetVertexBuffers(0, 1, pVertexBufferView);

	IndexBufferView* pIndexBufferView = nullptr;
	pCommandList->IASetIndexBuffer(pIndexBufferView);

	Viewport viewport(0.0f, 0.0f, FLOAT(pRTVTexture->GetWidth()), FLOAT(pRTVTexture->GetHeight()));
	pCommandList->RSSetViewports(1, &viewport);

	Rect scissorRect(ExtractRect(viewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->DrawInstanced(3, 1, 0, 0);

	if (pRTVEndState != nullptr)
		pCommandList->TransitionBarrier(pRTVTexture, *pRTVEndState);
	if (pSRVEndState != nullptr)
		pCommandList->TransitionBarrier(pSRVTexture, *pSRVEndState);

	pCommandList->Close();
	*/
}
