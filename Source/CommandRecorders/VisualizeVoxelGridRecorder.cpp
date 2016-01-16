#include "CommandRecorders/VisualizeVoxelGridRecorder.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXCommandList.h"
#include "DX/DXResource.h"
#include "DX/DXDescriptorHeap.h"
#include "DX/DXUtils.h"

enum RootParams
{
	kGridConfigCBVRootParam = 0,
	kTransformCBVHandleRootParam,
	kDepthSRVRootParam,
	kGridBufferSRVRootParam,
	kNumRootParams
};

VisualizeVoxelGridRecorder::VisualizeVoxelGridRecorder(VisualizeVoxelGridInitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	DXShader vertexShader(L"Shaders//FullScreenTriangleVS.hlsl", "Main", "vs_4_0");
	DXShader pixelShader(L"Shaders//VisualizeVoxelGridPS.hlsl", "Main", "ps_4_0");

	DXCBVRange gridConfigCBVRange(1, 0);
	DXCBVRange transformCBVRange(1, 1);
	DXSRVRange depthSRVRange(1, 0);
	DXSRVRange gridBufferSRVRange(1, 1);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kGridConfigCBVRootParam] = DXRootDescriptorTableParameter(1, &gridConfigCBVRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kTransformCBVHandleRootParam] = DXRootDescriptorTableParameter(1, &transformCBVRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kDepthSRVRootParam] = DXRootDescriptorTableParameter(1, &depthSRVRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kGridBufferSRVRootParam] = DXRootDescriptorTableParameter(1, &gridBufferSRVRange, D3D12_SHADER_VISIBILITY_PIXEL);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(pParams->m_pDevice, &rootSignatureDesc, L"VisualizeVoxelGridRecorder::m_pRootSignature");

	DXGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SetRenderTargetFormat(pParams->m_RTVFormat);

	m_pPipelineState = new DXPipelineState(pParams->m_pDevice, &pipelineStateDesc, L"VisualizeVoxelGridRecorder::m_pPipelineState");
}

VisualizeVoxelGridRecorder::~VisualizeVoxelGridRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void VisualizeVoxelGridRecorder::Record(VisualizeVoxelGridRecordParams* pParams)
{
	pParams->m_pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pParams->m_pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	if (pParams->m_pRenderTarget->GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
		pParams->m_pCommandList->TransitionBarrier(pParams->m_pRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (pParams->m_pDepthTexture->GetState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		pParams->m_pCommandList->TransitionBarrier(pParams->m_pDepthTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	pParams->m_pCommandList->SetDescriptorHeaps(pParams->m_pCBVSRVUAVDescriptorHeap);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kGridConfigCBVRootParam, pParams->m_GridConfigCBVHandle);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kTransformCBVHandleRootParam, pParams->m_TransformCBVHandle);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kDepthSRVRootParam, pParams->m_DepthSRVHandle);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kGridBufferSRVRootParam, pParams->m_GridBufferSRVHandle);

	pParams->m_pCommandList->OMSetRenderTargets(1, &pParams->m_RTVHandle);
	pParams->m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	DXVertexBufferView* pVertexBufferView = nullptr;
	pParams->m_pCommandList->IASetVertexBuffers(0, 1, pVertexBufferView);

	DXIndexBufferView* pIndexBufferView = nullptr;
	pParams->m_pCommandList->IASetIndexBuffer(pIndexBufferView);

	DXViewport viewport(0.0f, 0.0f, FLOAT(pParams->m_pDepthTexture->GetWidth()), FLOAT(pParams->m_pDepthTexture->GetHeight()));
	pParams->m_pCommandList->RSSetViewports(1, &viewport);

	DXRect scissorRect(ExtractRect(viewport));
	pParams->m_pCommandList->RSSetScissorRects(1, &scissorRect);

	pParams->m_pCommandList->DrawInstanced(3, 1, 0, 0);

	if (pParams->m_pRenderTargetEndState != nullptr)
		pParams->m_pCommandList->TransitionBarrier(pParams->m_pRenderTarget, *pParams->m_pRenderTargetEndState);

	D3D12_GPU_DESCRIPTOR_HANDLE nullHandle = pParams->m_pCBVSRVUAVDescriptorHeap->GetGPUDescriptor(0);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kDepthSRVRootParam, nullHandle);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kGridBufferSRVRootParam, nullHandle);

	pParams->m_pCommandList->Close();
}
