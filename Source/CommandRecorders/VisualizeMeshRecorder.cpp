#include "CommandRecorders/VisualizeMeshRecorder.h"
#include "DX/DXRootSignature.h"
#include "DX/DXPipelineState.h"
#include "DX/DXCommandList.h"
#include "DX/DXUtils.h"
#include "DX/DXResource.h"
#include "DX/DXRenderEnvironment.h"
#include "Common/MeshData.h"

enum RootParams
{
	kCBVRootParam = 0,
	kNumRootParams
};

VisualizeMeshRecorder::VisualizeMeshRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	assert(false);
	/*
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;

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
	m_pRootSignature = new DXRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"VisualizeMeshRecorder::m_pRootSignature");

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

	m_pPipelineState = new DXPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"VisualizeMeshRecorder::m_pPipelineState");
	*/
}

VisualizeMeshRecorder::~VisualizeMeshRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void VisualizeMeshRecorder::Record(RenderPassParams* pParams)
{
	assert(false);
	/*
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;
	DXCommandList* pCommandList = pParams->m_pCommandList;
	DXBindingResourceList* pResources = pParams->m_pResources;
	Mesh* pMesh = pParams->m_pMeshBatch;
		
	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	
	pCommandList->SetResourceTransitions(&pResources->m_ResourceTransitions);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParam, pResources->m_SRVHeapStart);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = pResources->m_RTVHeapStart;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = pResources->m_DSVHeapStart;
	pCommandList->OMSetRenderTargets(1, &rtvHeapStart, TRUE, &dsvHeapStart);
			
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, pMesh->GetVertexBuffer()->GetVBView());
	pCommandList->IASetIndexBuffer(pMesh->GetIndexBuffer()->GetIBView());

	pCommandList->RSSetViewports(1, pParams->m_pViewport);

	DXRect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);
	
	assert(pMesh->GetNumSubMeshes() == 1);
	const SubMeshData* pSubMeshData = pMesh->GetSubMeshes();
	pCommandList->DrawIndexedInstanced(pSubMeshData->m_NumIndices, 1, pSubMeshData->m_IndexStart, 0, 0);

	pCommandList->Close();
	*/
}
