#include "RenderPasses/CreateVoxelGridPass.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/RenderEnv.h"
#include "Common/MeshRenderResources.h"

enum RootParams
{
	kCBVRootParamVS = 0,
	kCBVRootParamGS,
	kSRVRootParamPS,
	kConstant32BitRootParamPS,
	kNumRootParams
};

CreateVoxelGridPass::CreateVoxelGridPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pCommandSignature(nullptr)
{
	assert(false);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	Shader vertexShader(L"Shaders//CreateVoxelGridVS.hlsl", "Main", "vs_4_0");
	Shader geometryShader(L"Shaders//CreateVoxelGridGS.hlsl", "Main", "gs_4_0");
	Shader pixelShader(L"Shaders//CreateVoxelGridPS.hlsl", "Main", "ps_5_1");
	
	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {CBVDescriptorRange(1, 0)};
	D3D12_DESCRIPTOR_RANGE descriptorRangesGS[] = {CBVDescriptorRange(1, 0)};
	D3D12_DESCRIPTOR_RANGE descriptorRangesPS[] = {CBVDescriptorRange(1, 0), SRVDescriptorRange(1, 0), UAVDescriptorRange(1, 0)};
		
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kCBVRootParamVS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), &descriptorRangesVS[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[kCBVRootParamGS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesGS), &descriptorRangesGS[0], D3D12_SHADER_VISIBILITY_GEOMETRY);
	rootParams[kSRVRootParamPS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesPS), &descriptorRangesPS[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kConstant32BitRootParamPS] = Root32BitConstantsParameter(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);
		
	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"CreateVoxelGridPass::m_pRootSignature");
		
	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetGeometryShader(&geometryShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.RasterizerState = RasterizerDesc(RasterizerDesc::CullNoneConservative);
	//pipelineStateDesc.InputLayout = *pMeshBatch->GetInputLayout(); // Kolya. Fix me
	//pipelineStateDesc.PrimitiveTopologyType = pMeshBatch->GetPrimitiveTopologyType(); // Kolya. Fix me
	
	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CreateVoxelGridPass::m_pPipelineState");

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] =
	{
		Constant32BitArgument(kConstant32BitRootParamPS, 0, 1),
		DrawIndexedArgument()
	};
	//CommandSignatureDesc commandSignatureDesc(sizeof(DrawMeshCommand), ARRAYSIZE(argumentDescs), &argumentDescs[0]);
	//m_pCommandSignature = new CommandSignature(pRenderEnv->m_pDevice, m_pRootSignature, &commandSignatureDesc, L"RenderGBufferPass::m_pCommandSignature");
}

CreateVoxelGridPass::~CreateVoxelGridPass()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void CreateVoxelGridPass::Record(RenderParams* pParams)
{
	assert(false);
	/*
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	ResourceList* pResources = pParams->m_pResources;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
			
	pCommandList->OMSetRenderTargets(0, nullptr);
	
	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamVS, pResources->m_SRVHeapStart);
	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamGS, DescriptorHandle(pResources->m_SRVHeapStart, 1));
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParamPS, DescriptorHandle(pResources->m_SRVHeapStart, 2));
	pCommandList->IASetPrimitiveTopology(pMeshBatch->GetPrimitiveTopology());
	pCommandList->IASetVertexBuffers(0, 1, pMeshBatch->GetVertexBuffer()->GetVBView());
	pCommandList->IASetIndexBuffer(pMeshBatch->GetIndexBuffer()->GetIBView());
	
	pCommandList->RSSetViewports(1, pParams->m_pViewport);
	
	Rect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);
	
	pCommandList->ExecuteIndirect(m_pCommandSignature, pMeshBatch->GetNumMeshes(), pParams->m_pDrawMeshCommandBuffer, 0, pParams->m_pNumDrawMeshesBuffer, 0);
	pCommandList->End();
	*/
}
