#include "CommandRecorders/VisualizeMeshRecorder.h"
#include "DX/DXRootSignature.h"
#include "DX/DXPipelineState.h"
#include "DX/DXCommandList.h"
#include "DX/DXResource.h"
#include "DX/DXUtils.h"
#include "Common/MeshData.h"

enum RootParams
{
	kCBVRootParam = 0,
	kNumRootParams
};

VisualizeMeshRecorder::VisualizeMeshRecorder(VisualizeMeshInitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	// Kolya: fix me
	assert(false);
	/*
	u8 inputElementFlags = VertexElementFlag_Position;
	DXShaderMacro shaderDefines[2];

	if (pParams->m_MeshDataElement == MeshDataElement_Color)
	{
		shaderDefines[0] = DXShaderMacro("HAS_COLOR", "1");
		inputElementFlags |= VertexElementFlag_Color;
	}
	else if (pParams->m_MeshDataElement == MeshDataElement_Normal)
	{
		shaderDefines[0] = DXShaderMacro("HAS_NORMAL", "1");
		inputElementFlags |= VertexElementFlag_Normal;
	}
	else if (pParams->m_MeshDataElement == MeshDataElement_TexCoords)
	{
		shaderDefines[0] = DXShaderMacro("HAS_TEXCOORD", "1");
		inputElementFlags |= VertexElementFlag_TexCoords;
	}
	else
	{
		assert(false);
	}

	shaderDefines[1] = DXShaderMacro();

	DXShader vertexShader(L"Shaders//VisualizeMeshVS.hlsl", "Main", "vs_4_0", shaderDefines);
	DXShader pixelShader(L"Shaders//VisualizeMeshPS.hlsl", "Main", "ps_4_0", shaderDefines);

	DXCBVRange cbvRange(1, 0);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kCBVRootParam] = DXRootDescriptorTableParameter(1, &cbvRange, D3D12_SHADER_VISIBILITY_VERTEX);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(pParams->m_pDevice, &rootSignatureDesc, L"VisualizeMeshRecorder::m_pRootSignature");

	std::vector<DXInputElementDesc> inputElementDescs;
	GenerateInputElements(inputElementDescs, inputElementFlags, pParams->m_VertexElementFlags);

	DXGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.SetInputLayout(inputElementDescs.size(), &inputElementDescs[0]);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.DepthStencilState = DXDepthStencilDesc(DXDepthStencilDesc::Enabled);
	pipelineStateDesc.SetRenderTargetFormat(pParams->m_RTVFormat, pParams->m_DSVFormat);

	m_pPipelineState = new DXPipelineState(pParams->m_pDevice, &pipelineStateDesc, L"VisualizeMeshRecorder::m_pPipelineState");
	*/
}

VisualizeMeshRecorder::~VisualizeMeshRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void VisualizeMeshRecorder::Record(VisualizeMeshRecordParams* pParams)
{
	// Kolya: fix me
	assert(false);
	/*
	pParams->m_pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	
	// Kolya: Has to force clear state - otherwise VS Graphics Debugger will fail to make capture
	pParams->m_pCommandList->GetDXObject()->ClearState(m_pPipelineState->GetDXObject());

	pParams->m_pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	if (pParams->m_pRTVTexture->GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
		pParams->m_pCommandList->TransitionBarrier(pParams->m_pRTVTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (pParams->m_pDSVTexture->GetState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
		pParams->m_pCommandList->TransitionBarrier(pParams->m_pDSVTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	pParams->m_pCommandList->SetDescriptorHeaps(pParams->m_pCBVSRVUAVDescriptorHeap);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParam, pParams->m_CBVHandle);

	pParams->m_pCommandList->OMSetRenderTargets(1, &pParams->m_RTVHandle, TRUE, &pParams->m_DSVHandle);
			
	pParams->m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pParams->m_pCommandList->IASetVertexBuffers(0, 1, pParams->m_pMesh->GetVertexBufferView());
	pParams->m_pCommandList->IASetIndexBuffer(pParams->m_pMesh->GetIndexBufferView());

	DXViewport viewport(0.0f, 0.0f, FLOAT(pParams->m_pRTVTexture->GetWidth()), FLOAT(pParams->m_pRTVTexture->GetHeight()));
	pParams->m_pCommandList->RSSetViewports(1, &viewport);

	DXRect scissorRect(ExtractRect(viewport));
	pParams->m_pCommandList->RSSetScissorRects(1, &scissorRect);

	assert(pParams->m_pMesh->GetNumSubMeshes() == 1);
	const SubMeshData* pSubMeshData = pParams->m_pMesh->GetSubMeshes();
	pParams->m_pCommandList->DrawIndexedInstanced(pSubMeshData->m_NumIndices, 1, pSubMeshData->m_IndexStart, 0, 0);

	if (pParams->m_pRTVEndState != nullptr)
		pParams->m_pCommandList->TransitionBarrier(pParams->m_pRTVTexture, *pParams->m_pRTVEndState);

	pParams->m_pCommandList->Close();
	*/
}
