#include "CommandRecorders/VisualizeMeshRecorder.h"
#include "DX/DXRootSignature.h"
#include "DX/DXPipelineState.h"
#include "DX/DXCommandList.h"
#include "DX/DXResource.h"
#include "DX/DXUtils.h"
#include "Common/MeshData.h"

enum RootParams
{
	kObjectTransformCBVRootParam = 0,
	kNumRootParams
};

VisualizeMeshRecorder::VisualizeMeshRecorder(DXDevice* pDevice, DXGI_FORMAT rtvFormat, MeshDataElement meshDataElement, u8 vertexElementFlags)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	u8 inputElementFlags = 0;
	DXShaderMacro shaderDefines[2];	

	if (meshDataElement == MeshDataElement_Color)
	{
		shaderDefines[0] = DXShaderMacro("HAS_COLOR", "1");
		inputElementFlags = VertexElementFlag_Color;
	}
	else if (meshDataElement == MeshDataElement_Normal)
	{
		shaderDefines[0] = DXShaderMacro("HAS_NORMAL", "1");
		inputElementFlags = VertexElementFlag_Normal;
	}
	else if (meshDataElement == MeshDataElement_TexCoords)
	{
		shaderDefines[0] = DXShaderMacro("HAS_TEXCOORD", "1");
		inputElementFlags = VertexElementFlag_TexCoords;
	}
	else
	{
		assert(false);
	}

	shaderDefines[1] = DXShaderMacro();

	DXShader vertexShader(L"Shaders//VisualizeMeshVS.hlsl", "Main", "vs_4_0", shaderDefines);
	DXShader pixelShader(L"Shaders//VisualizeMeshPS.hlsl", "Main", "ps_4_0", shaderDefines);

	DXCBVRange objectTransformCBVRange(1, 0);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kObjectTransformCBVRootParam] = DXRootDescriptorTableParameter(1, &objectTransformCBVRange, D3D12_SHADER_VISIBILITY_VERTEX);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(pDevice, &rootSignatureDesc, L"VisualizeMeshRecorder::m_pRootSignature");

	std::vector<DXInputElementDesc> inputElementDescs;
	GenerateInputElements(inputElementDescs, inputElementFlags, vertexElementFlags);

	DXGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.SetInputLayout(inputElementDescs.size(), &inputElementDescs[0]);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SetRenderTargetFormat(rtvFormat);

	m_pPipelineState = new DXPipelineState(pDevice, &pipelineStateDesc, L"VisualizeMeshRecorder::m_pPipelineState");
}

VisualizeMeshRecorder::~VisualizeMeshRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void VisualizeMeshRecorder::Record(DXCommandList* pCommandList, DXCommandAllocator* pCommandAllocator,
	DXResource* pRTVTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor,
	UINT numDXDescriptorHeaps, ID3D12DescriptorHeap* pDXFirstDescriptorHeap,
	D3D12_GPU_DESCRIPTOR_HANDLE objectTransformCBVDescriptor,
	Mesh* pMesh, const D3D12_RESOURCE_STATES* pRTVEndState)
{
	pCommandList->Reset(pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	if (pRTVTexture->GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
		pCommandList->TransitionBarrier(pRTVTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);

	pCommandList->SetDescriptorHeaps(numDXDescriptorHeaps, &pDXFirstDescriptorHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kObjectTransformCBVRootParam, objectTransformCBVDescriptor);

	pCommandList->OMSetRenderTargets(1, &rtvDescriptor);
			
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, pMesh->GetVertexBufferView());
	pCommandList->IASetIndexBuffer(pMesh->GetIndexBufferView());

	DXViewport viewport(0.0f, 0.0f, FLOAT(pRTVTexture->GetWidth()), FLOAT(pRTVTexture->GetHeight()));
	pCommandList->RSSetViewports(1, &viewport);

	DXRect scissorRect(0, 0, LONG(pRTVTexture->GetWidth()), LONG(pRTVTexture->GetHeight()));
	pCommandList->RSSetScissorRects(1, &scissorRect);

	assert(pMesh->GetNumSubMeshes() == 1);
	const SubMeshData* pSubMeshData = pMesh->GetSubMeshes();
	pCommandList->DrawIndexedInstanced(pSubMeshData->m_NumIndices, 1, pSubMeshData->m_IndexStart, 0, 0);

	if (pRTVEndState != nullptr)
		pCommandList->TransitionBarrier(pRTVTexture, *pRTVEndState);

	pCommandList->Close();
}
