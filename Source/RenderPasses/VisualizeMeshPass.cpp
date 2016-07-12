#include "RenderPasses/VisualizeMeshPass.h"
#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DPipelineState.h"
#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DUtils.h"
#include "D3DWrapper/D3DResource.h"
#include "D3DWrapper/D3DRenderEnv.h"
#include "Common/MeshData.h"

enum RootParams
{
	kCBVRootParam = 0,
	kNumRootParams
};

VisualizeMeshPass::VisualizeMeshPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	assert(false);
	/*
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	u8 inputElementFlags = VertexElementFlag_Position;
	D3DShaderMacro shaderDefines[2];

	if (pParams->m_MeshDataElement == MeshDataElement_Color)
	{
		shaderDefines[0] = D3DShaderMacro("HAS_COLOR", "1");
		inputElementFlags |= VertexElementFlag_Color;
	}
	else if (pParams->m_MeshDataElement == MeshDataElement_Normal)
	{
		shaderDefines[0] = D3DShaderMacro("HAS_NORMAL", "1");
		inputElementFlags |= VertexElementFlag_Normal;
	}
	else if (pParams->m_MeshDataElement == MeshDataElement_TexCoords)
	{
		shaderDefines[0] = D3DShaderMacro("HAS_TEXCOORD", "1");
		inputElementFlags |= VertexElementFlag_TexCoords;
	}
	else
	{
		assert(false);
	}

	shaderDefines[1] = D3DShaderMacro();

	D3DShader vertexShader(L"Shaders//VisualizeMeshVS.hlsl", "Main", "vs_4_0", shaderDefines);
	D3DShader pixelShader(L"Shaders//VisualizeMeshPS.hlsl", "Main", "ps_4_0", shaderDefines);

	D3DCBVRange cbvRange(1, 0);
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kCBVRootParam] = D3DRootDescriptorTableParameter(1, &cbvRange, D3D12_SHADER_VISIBILITY_VERTEX);

	D3DRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new D3DRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"VisualizeMeshPass::m_pRootSignature");

	std::vector<D3DInputElementDesc> inputElementDescs;
	GenerateInputElements(inputElementDescs, inputElementFlags, pParams->m_VertexElementFlags);

	D3DGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.SetInputLayout(inputElementDescs.size(), &inputElementDescs[0]);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.DepthStencilState = D3DDepthStencilDesc(D3DDepthStencilDesc::Enabled);
	pipelineStateDesc.SetRenderTargetFormat(pParams->m_RTVFormat, pParams->m_DSVFormat);

	m_pPipelineState = new D3DPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"VisualizeMeshPass::m_pPipelineState");
	*/
}

VisualizeMeshPass::~VisualizeMeshPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void VisualizeMeshPass::Record(RenderParams* pParams)
{
	assert(false);
	/*
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	D3DCommandList* pCommandList = pParams->m_pCommandList;
	D3DResourceList* pResources = pParams->m_pResources;
	Mesh* pMesh = pParams->m_pMeshBatch;
		
	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	
	pCommandList->SetResourceTransitions(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParam, pResources->m_SRVHeapStart);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = pResources->m_RTVHeapStart;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = pResources->m_DSVHeapStart;
	pCommandList->OMSetRenderTargets(1, &rtvHeapStart, TRUE, &dsvHeapStart);
			
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, pMesh->GetVertexBuffer()->GetVBView());
	pCommandList->IASetIndexBuffer(pMesh->GetIndexBuffer()->GetIBView());

	pCommandList->RSSetViewports(1, pParams->m_pViewport);

	D3DRect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);
	
	assert(pMesh->GetNumSubMeshes() == 1);
	const SubMeshData* pSubMeshData = pMesh->GetSubMeshes();
	pCommandList->DrawIndexedInstanced(pSubMeshData->m_NumIndices, 1, pSubMeshData->m_IndexStart, 0, 0);

	pCommandList->Close();
	*/
}
