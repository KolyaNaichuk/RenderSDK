#include "CommandRecorders/FillGBufferRecorder.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXCommandList.h"
#include "DX/DXResource.h"
#include "DX/DXUtils.h"
#include "Common/MeshData.h"
#include "Common/Mesh.h"

enum RootParams
{
	kTransformCBVRootParam = 0,
	kMaterialCBVRootParam,
	kNumRootParams
};

FillGBufferRecorder::FillGBufferRecorder(FillGBufferInitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	DXShader vertexShader(L"Shaders//FillGBufferVS.hlsl", "Main", "vs_4_0");

	std::vector<DXShaderMacro> pixelShaderDefines;
	
	if (pParams->m_MaterialElementFlags & MaterialElementFlag_DiffuseMap)
		pixelShaderDefines.push_back(DXShaderMacro("DIFFUSE_MAP", "1"));
	
	if (pParams->m_MaterialElementFlags & MaterialElementFlag_NormalMap)
		pixelShaderDefines.push_back(DXShaderMacro("NORMAL_MAP", "1"));
	
	if (pParams->m_MaterialElementFlags & MaterialElementFlag_SpecularMap)
		pixelShaderDefines.push_back(DXShaderMacro("SPECULAR_MAP", "1"));

	pixelShaderDefines.push_back(DXShaderMacro());

	DXShader pixelShader(L"Shaders//FillGBufferPS.hlsl", "Main", "ps_4_0", &pixelShaderDefines[0]);
		
	DXCBVRange transformCBVRange(1, 0);
	DXCBVRange materialCBVRange(1, 0);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kTransformCBVRootParam] = DXRootDescriptorTableParameter(1, &transformCBVRange, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[kMaterialCBVRootParam] = DXRootDescriptorTableParameter(1, &materialCBVRange, D3D12_SHADER_VISIBILITY_PIXEL);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(pParams->m_pDevice, &rootSignatureDesc, L"FillGBufferRecorder::m_pRootSignature");

	std::vector<DXInputElementDesc> inputElementDescs;
	GenerateInputElements(inputElementDescs, pParams->m_VertexElementFlags, pParams->m_VertexElementFlags);

	DXGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.SetInputLayout(inputElementDescs.size(), &inputElementDescs[0]);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.DepthStencilState = DXDepthStencilDesc(DXDepthStencilDesc::Enabled);
	
	const DXGI_FORMAT rtvFormats[] = {pParams->m_DiffuseRTVFormat, pParams->m_NormalRTVFormat, pParams->m_SpecularRTVFormat};
	pipelineStateDesc.SetRenderTargetFormats(ARRAYSIZE(rtvFormats), rtvFormats, pParams->m_DSVFormat);

	m_pPipelineState = new DXPipelineState(pParams->m_pDevice, &pipelineStateDesc, L"FillGBufferRecorder::m_pPipelineState");
}

FillGBufferRecorder::~FillGBufferRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void FillGBufferRecorder::Record(FillGBufferRecordParams* pParams)
{
	pParams->m_pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);

	// Kolya: Has to force clear state - otherwise VS Graphics Debugger will fail to make capture
	pParams->m_pCommandList->GetDXObject()->ClearState(m_pPipelineState->GetDXObject());
	pParams->m_pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	
	if (pParams->m_pDiffuseTexture->GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
		pParams->m_pCommandList->TransitionBarrier(pParams->m_pDiffuseTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
	
	if (pParams->m_pNormalTexture->GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
		pParams->m_pCommandList->TransitionBarrier(pParams->m_pNormalTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
	
	if (pParams->m_pDepthTexture->GetState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
		pParams->m_pCommandList->TransitionBarrier(pParams->m_pDepthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	pParams->m_pCommandList->SetDescriptorHeaps(pParams->m_pCBVDescriptorHeap);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kTransformCBVRootParam, pParams->m_TransformCBVHandle);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = {pParams->m_DiffuseRTVHandle, pParams->m_NormalRTVHandle};
	pParams->m_pCommandList->OMSetRenderTargets(ARRAYSIZE(rtvHandles), rtvHandles, TRUE, &pParams->m_DSVHandle);

	pParams->m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pParams->m_pCommandList->IASetVertexBuffers(0, 1, pParams->m_pMesh->GetVertexBufferView());
	pParams->m_pCommandList->IASetIndexBuffer(pParams->m_pMesh->GetIndexBufferView());

	DXViewport viewport(0.0f, 0.0f, FLOAT(pParams->m_pDiffuseTexture->GetWidth()), FLOAT(pParams->m_pDiffuseTexture->GetHeight()));
	pParams->m_pCommandList->RSSetViewports(1, &viewport);

	DXRect scissorRect(ExtractRect(viewport));
	pParams->m_pCommandList->RSSetScissorRects(1, &scissorRect);

	assert(pParams->m_pMesh->GetNumSubMeshes() == 1);
	const SubMeshData* pSubMeshData = pParams->m_pMesh->GetSubMeshes();
	pParams->m_pCommandList->DrawIndexedInstanced(pSubMeshData->m_NumIndices, 1, pSubMeshData->m_IndexStart, 0, 0);

	pParams->m_pCommandList->Close();
}
